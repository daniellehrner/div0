#include "div0/t8n/alloc.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

#include <string.h>
#include <yyjson.h>

// ============================================================================
// Parsing Implementation
// ============================================================================

json_result_t t8n_parse_alloc_value(yyjson_val_t *root, div0_arena_t *arena,
                                    state_snapshot_t *out) {
  if (root == nullptr || !json_is_obj(root)) {
    return json_err(JSON_ERR_INVALID_TYPE, "alloc must be an object");
  }

  // Count accounts
  size_t count = json_obj_size(root);
  if (count == 0) {
    out->accounts = nullptr;
    out->account_count = 0;
    return json_ok();
  }

  // Allocate account array
  out->accounts = div0_arena_alloc(arena, count * sizeof(account_snapshot_t));
  if (out->accounts == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate accounts");
  }
  out->account_count = count;

  // Parse each account
  json_obj_iter_t iter = json_obj_iter(root);
  const char *addr_hex;
  yyjson_val_t *account_val;
  size_t idx = 0;

  while (json_obj_iter_next(&iter, &addr_hex, &account_val)) {
    account_snapshot_t *account = &out->accounts[idx];
    __builtin___memset_chk(account, 0, sizeof(*account), __builtin_object_size(account, 0));

    // Parse address from key
    if (!hex_decode(addr_hex, account->address.bytes, ADDRESS_SIZE)) {
      return json_err(JSON_ERR_INVALID_HEX, "invalid address key");
    }

    if (!json_is_obj(account_val)) {
      return json_err(JSON_ERR_INVALID_TYPE, "account must be an object");
    }

    // Parse balance (required)
    if (!json_get_hex_uint256(account_val, "balance", &account->balance)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing balance field");
    }

    // Parse nonce (optional, defaults to 0)
    json_get_hex_u64(account_val, "nonce", &account->nonce);

    // Parse code (optional)
    json_get_hex_bytes(account_val, "code", arena, &account->code);

    // Parse storage (optional)
    yyjson_val_t *storage_val = json_obj_get(account_val, "storage");
    if (storage_val != nullptr && json_is_obj(storage_val)) {
      size_t storage_count = json_obj_size(storage_val);
      if (storage_count > 0) {
        account->storage = div0_arena_alloc(arena, storage_count * sizeof(storage_entry_t));
        if (account->storage == nullptr) {
          return json_err(JSON_ERR_ALLOC, "failed to allocate storage");
        }

        json_obj_iter_t storage_iter = json_obj_iter(storage_val);
        const char *slot_hex;
        yyjson_val_t *value_val;
        size_t storage_idx = 0;

        while (json_obj_iter_next(&storage_iter, &slot_hex, &value_val)) {
          storage_entry_t *entry = &account->storage[storage_idx];

          if (!hex_decode_uint256(slot_hex, &entry->slot)) {
            return json_err(JSON_ERR_INVALID_HEX, "invalid storage slot key");
          }

          if (!json_val_hex_uint256(value_val, &entry->value)) {
            return json_err(JSON_ERR_INVALID_HEX, "invalid storage value");
          }

          // Skip zero values (they represent deleted slots)
          if (!uint256_is_zero(entry->value)) {
            storage_idx++;
          }
        }
        account->storage_count = storage_idx;
      }
    }

    idx++;
  }

  return json_ok();
}

json_result_t t8n_parse_alloc(const char *json, size_t len, div0_arena_t *arena,
                              state_snapshot_t *out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse(json, len, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *root = json_doc_root(&doc);
  result = t8n_parse_alloc_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

json_result_t t8n_parse_alloc_file(const char *path, div0_arena_t *arena, state_snapshot_t *out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse_file(path, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *root = json_doc_root(&doc);
  result = t8n_parse_alloc_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

// ============================================================================
// Serialization Implementation
// ============================================================================

yyjson_mut_val_t *t8n_write_alloc_account(const account_snapshot_t *account, json_writer_t *w) {
  yyjson_mut_val_t *obj = json_write_obj(w);
  if (obj == nullptr) {
    return nullptr;
  }

  // Balance (always present)
  json_obj_add_hex_uint256(w, obj, "balance", &account->balance);

  // Nonce (only if non-zero)
  if (account->nonce != 0) {
    json_obj_add_hex_u64(w, obj, "nonce", account->nonce);
  }

  // Code (only if non-empty)
  if (account->code.size > 0) {
    json_obj_add_hex_bytes(w, obj, "code", account->code.data, account->code.size);
  }

  // Storage (only if non-empty)
  if (account->storage_count > 0) {
    yyjson_mut_val_t *storage = json_write_obj(w);
    for (size_t i = 0; i < account->storage_count; i++) {
      const storage_entry_t *entry = &account->storage[i];

      // Skip zero values
      if (uint256_is_zero(entry->value)) {
        continue;
      }

      char slot_hex[67];
      hex_encode_uint256_padded(&entry->slot, slot_hex);

      yyjson_mut_val_t *value = json_write_hex_uint256_padded(w, &entry->value);
      json_obj_add(w, storage, slot_hex, value);
    }
    json_obj_add(w, obj, "storage", storage);
  }

  return obj;
}

yyjson_mut_val_t *t8n_write_alloc(const state_snapshot_t *snapshot, json_writer_t *w) {
  yyjson_mut_val_t *root = json_write_obj(w);
  if (root == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < snapshot->account_count; i++) {
    const account_snapshot_t *account = &snapshot->accounts[i];

    char addr_hex[43];
    hex_encode(account->address.bytes, ADDRESS_SIZE, addr_hex);

    yyjson_mut_val_t *account_obj = t8n_write_alloc_account(account, w);
    if (account_obj == nullptr) {
      return nullptr;
    }

    json_obj_add(w, root, addr_hex, account_obj);
  }

  return root;
}

#endif // DIV0_FREESTANDING
