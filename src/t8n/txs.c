#include "div0/t8n/txs.h"

#ifndef DIV0_FREESTANDING

// ============================================================================
// Access List Parsing
// ============================================================================

static json_result_t parse_access_list(yyjson_val_t *arr, div0_arena_t *arena, access_list_t *out) {
  access_list_init(out);

  if (arr == nullptr || !json_is_arr(arr)) {
    return json_ok();
  }

  size_t count = json_arr_len(arr);
  if (count == 0) {
    return json_ok();
  }

  if (!access_list_alloc_entries(out, count, arena)) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate access list");
  }

  json_arr_iter_t iter = json_arr_iter(arr);
  yyjson_val_t *entry_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &entry_val)) {
    if (!json_is_obj(entry_val)) {
      return json_err(JSON_ERR_INVALID_TYPE, "access list entry must be an object");
    }

    access_list_entry_t *entry = &out->entries[idx];

    if (!json_get_hex_address(entry_val, "address", &entry->address)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing access list entry address");
    }

    yyjson_val_t *keys_val = json_obj_get(entry_val, "storageKeys");
    if (keys_val != nullptr && json_is_arr(keys_val)) {
      size_t key_count = json_arr_len(keys_val);
      if (!access_list_entry_alloc_keys(entry, key_count, arena)) {
        return json_err(JSON_ERR_ALLOC, "failed to allocate storage keys");
      }

      json_arr_iter_t key_iter = json_arr_iter(keys_val);
      yyjson_val_t *key_val;
      size_t key_idx = 0;

      while (json_arr_iter_next(&key_iter, &key_val)) {
        if (!json_val_hex_uint256(key_val, &entry->storage_keys[key_idx])) {
          return json_err(JSON_ERR_INVALID_HEX, "invalid storage key");
        }
        key_idx++;
      }
    } else {
      entry->storage_keys = nullptr;
      entry->storage_keys_count = 0;
    }

    idx++;
  }

  return json_ok();
}

// ============================================================================
// Authorization List Parsing (EIP-7702)
// ============================================================================

static json_result_t parse_authorization_list(yyjson_val_t *arr, div0_arena_t *arena,
                                              authorization_list_t *out) {
  authorization_list_init(out);

  if (arr == nullptr || !json_is_arr(arr)) {
    return json_ok();
  }

  size_t count = json_arr_len(arr);
  if (count == 0) {
    return json_ok();
  }

  if (!authorization_list_alloc(out, count, arena)) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate authorization list");
  }

  json_arr_iter_t iter = json_arr_iter(arr);
  yyjson_val_t *auth_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &auth_val)) {
    if (!json_is_obj(auth_val)) {
      return json_err(JSON_ERR_INVALID_TYPE, "authorization must be an object");
    }

    authorization_t *auth = &out->entries[idx];

    if (!json_get_hex_u64(auth_val, "chainId", &auth->chain_id)) {
      auth->chain_id = 0; // 0 = valid on any chain
    }
    if (!json_get_hex_address(auth_val, "address", &auth->address)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing authorization address");
    }
    if (!json_get_hex_u64(auth_val, "nonce", &auth->nonce)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing authorization nonce");
    }

    // Signature fields
    uint64_t y;
    if (json_get_hex_u64(auth_val, "yParity", &y)) {
      auth->y_parity = (uint8_t)y;
    } else if (json_get_hex_u64(auth_val, "v", &y)) {
      auth->y_parity = (uint8_t)(y & 1);
    } else {
      auth->y_parity = 0;
    }

    if (!json_get_hex_uint256(auth_val, "r", &auth->r)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing authorization r");
    }
    if (!json_get_hex_uint256(auth_val, "s", &auth->s)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing authorization s");
    }

    idx++;
  }

  return json_ok();
}

// ============================================================================
// Blob Hash Parsing (EIP-4844)
// ============================================================================

static json_result_t parse_blob_hashes(yyjson_val_t *arr, div0_arena_t *arena, hash_t **out,
                                       size_t *out_count) {
  *out = nullptr;
  *out_count = 0;

  if (arr == nullptr || !json_is_arr(arr)) {
    return json_ok();
  }

  size_t count = json_arr_len(arr);
  if (count == 0) {
    return json_ok();
  }

  *out = div0_arena_alloc(arena, count * sizeof(hash_t));
  if (*out == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate blob hashes");
  }

  json_arr_iter_t iter = json_arr_iter(arr);
  yyjson_val_t *hash_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &hash_val)) {
    if (!json_val_hex_hash(hash_val, &(*out)[idx])) {
      return json_err(JSON_ERR_INVALID_HEX, "invalid blob hash");
    }
    idx++;
  }

  *out_count = idx;
  return json_ok();
}

// ============================================================================
// Transaction Parsing
// ============================================================================

json_result_t t8n_parse_tx(yyjson_val_t *obj, div0_arena_t *arena, transaction_t *out) {
  if (obj == nullptr || !json_is_obj(obj)) {
    return json_err(JSON_ERR_INVALID_TYPE, "transaction must be an object");
  }

  // Determine transaction type
  uint64_t tx_type = 0;
  json_get_hex_u64(obj, "type", &tx_type);

  // Check for protected field (only for type detection)
  yyjson_val_t *protected_val = json_obj_get(obj, "protected");
  bool is_protected = (protected_val != nullptr && json_get_bool(protected_val));

  json_result_t result;

  switch (tx_type) {
  case 0: {
    // Legacy transaction
    out->type = TX_TYPE_LEGACY;
    legacy_tx_t *tx = &out->legacy;
    legacy_tx_init(tx);

    json_get_hex_u64(obj, "nonce", &tx->nonce);
    json_get_hex_uint256(obj, "gasPrice", &tx->gas_price);
    json_get_hex_u64(obj, "gas", &tx->gas_limit);
    if (!json_get_hex_u64(obj, "gas", &tx->gas_limit)) {
      json_get_hex_u64(obj, "gasLimit", &tx->gas_limit);
    }
    json_get_hex_uint256(obj, "value", &tx->value);
    json_get_hex_bytes(obj, "input", arena, &tx->data);
    if (tx->data.size == 0) {
      json_get_hex_bytes(obj, "data", arena, &tx->data);
    }

    // To address (optional for contract creation)
    yyjson_val_t *to_val = json_obj_get(obj, "to");
    if (to_val != nullptr && !json_is_null(to_val)) {
      tx->to = div0_arena_alloc(arena, sizeof(address_t));
      if (tx->to == nullptr) {
        return json_err(JSON_ERR_ALLOC, "failed to allocate to address");
      }
      if (!json_val_hex_address(to_val, tx->to)) {
        return json_err(JSON_ERR_INVALID_HEX, "invalid to address");
      }
    }

    // Signature
    json_get_hex_u64(obj, "v", &tx->v);
    json_get_hex_uint256(obj, "r", &tx->r);
    json_get_hex_uint256(obj, "s", &tx->s);

    break;
  }

  case 1: {
    // EIP-2930 transaction
    out->type = TX_TYPE_EIP2930;
    eip2930_tx_t *tx = &out->eip2930;
    eip2930_tx_init(tx);

    json_get_hex_u64(obj, "chainId", &tx->chain_id);
    json_get_hex_u64(obj, "nonce", &tx->nonce);
    json_get_hex_uint256(obj, "gasPrice", &tx->gas_price);
    json_get_hex_u64(obj, "gas", &tx->gas_limit);
    if (!json_get_hex_u64(obj, "gas", &tx->gas_limit)) {
      json_get_hex_u64(obj, "gasLimit", &tx->gas_limit);
    }
    json_get_hex_uint256(obj, "value", &tx->value);
    json_get_hex_bytes(obj, "input", arena, &tx->data);
    if (tx->data.size == 0) {
      json_get_hex_bytes(obj, "data", arena, &tx->data);
    }

    // To address
    yyjson_val_t *to_val = json_obj_get(obj, "to");
    if (to_val != nullptr && !json_is_null(to_val)) {
      tx->to = div0_arena_alloc(arena, sizeof(address_t));
      if (tx->to == nullptr) {
        return json_err(JSON_ERR_ALLOC, "failed to allocate to address");
      }
      if (!json_val_hex_address(to_val, tx->to)) {
        return json_err(JSON_ERR_INVALID_HEX, "invalid to address");
      }
    }

    // Access list
    result = parse_access_list(json_obj_get(obj, "accessList"), arena, &tx->access_list);
    if (json_is_err(result)) {
      return result;
    }

    // Signature
    uint64_t y;
    if (json_get_hex_u64(obj, "yParity", &y)) {
      tx->y_parity = (uint8_t)y;
    } else if (json_get_hex_u64(obj, "v", &y)) {
      tx->y_parity = (uint8_t)(y & 1);
    }
    json_get_hex_uint256(obj, "r", &tx->r);
    json_get_hex_uint256(obj, "s", &tx->s);

    break;
  }

  case 2: {
    // EIP-1559 transaction
    out->type = TX_TYPE_EIP1559;
    eip1559_tx_t *tx = &out->eip1559;
    eip1559_tx_init(tx);

    json_get_hex_u64(obj, "chainId", &tx->chain_id);
    json_get_hex_u64(obj, "nonce", &tx->nonce);
    json_get_hex_uint256(obj, "maxPriorityFeePerGas", &tx->max_priority_fee_per_gas);
    json_get_hex_uint256(obj, "maxFeePerGas", &tx->max_fee_per_gas);
    json_get_hex_u64(obj, "gas", &tx->gas_limit);
    if (!json_get_hex_u64(obj, "gas", &tx->gas_limit)) {
      json_get_hex_u64(obj, "gasLimit", &tx->gas_limit);
    }
    json_get_hex_uint256(obj, "value", &tx->value);
    json_get_hex_bytes(obj, "input", arena, &tx->data);
    if (tx->data.size == 0) {
      json_get_hex_bytes(obj, "data", arena, &tx->data);
    }

    // To address
    yyjson_val_t *to_val = json_obj_get(obj, "to");
    if (to_val != nullptr && !json_is_null(to_val)) {
      tx->to = div0_arena_alloc(arena, sizeof(address_t));
      if (tx->to == nullptr) {
        return json_err(JSON_ERR_ALLOC, "failed to allocate to address");
      }
      if (!json_val_hex_address(to_val, tx->to)) {
        return json_err(JSON_ERR_INVALID_HEX, "invalid to address");
      }
    }

    // Access list
    result = parse_access_list(json_obj_get(obj, "accessList"), arena, &tx->access_list);
    if (json_is_err(result)) {
      return result;
    }

    // Signature
    uint64_t y;
    if (json_get_hex_u64(obj, "yParity", &y)) {
      tx->y_parity = (uint8_t)y;
    } else if (json_get_hex_u64(obj, "v", &y)) {
      tx->y_parity = (uint8_t)(y & 1);
    }
    json_get_hex_uint256(obj, "r", &tx->r);
    json_get_hex_uint256(obj, "s", &tx->s);

    break;
  }

  case 3: {
    // EIP-4844 blob transaction
    out->type = TX_TYPE_EIP4844;
    eip4844_tx_t *tx = &out->eip4844;
    eip4844_tx_init(tx);

    json_get_hex_u64(obj, "chainId", &tx->chain_id);
    json_get_hex_u64(obj, "nonce", &tx->nonce);
    json_get_hex_uint256(obj, "maxPriorityFeePerGas", &tx->max_priority_fee_per_gas);
    json_get_hex_uint256(obj, "maxFeePerGas", &tx->max_fee_per_gas);
    json_get_hex_u64(obj, "gas", &tx->gas_limit);
    if (!json_get_hex_u64(obj, "gas", &tx->gas_limit)) {
      json_get_hex_u64(obj, "gasLimit", &tx->gas_limit);
    }
    json_get_hex_uint256(obj, "value", &tx->value);
    json_get_hex_bytes(obj, "input", arena, &tx->data);
    if (tx->data.size == 0) {
      json_get_hex_bytes(obj, "data", arena, &tx->data);
    }

    // To address (required for blob tx)
    if (!json_get_hex_address(obj, "to", &tx->to)) {
      return json_err(JSON_ERR_MISSING_FIELD, "blob transaction requires to address");
    }

    // Access list
    result = parse_access_list(json_obj_get(obj, "accessList"), arena, &tx->access_list);
    if (json_is_err(result)) {
      return result;
    }

    // Blob-specific fields
    json_get_hex_uint256(obj, "maxFeePerBlobGas", &tx->max_fee_per_blob_gas);

    result = parse_blob_hashes(json_obj_get(obj, "blobVersionedHashes"), arena,
                               &tx->blob_versioned_hashes, &tx->blob_hashes_count);
    if (json_is_err(result)) {
      return result;
    }

    // Signature
    uint64_t y;
    if (json_get_hex_u64(obj, "yParity", &y)) {
      tx->y_parity = (uint8_t)y;
    } else if (json_get_hex_u64(obj, "v", &y)) {
      tx->y_parity = (uint8_t)(y & 1);
    }
    json_get_hex_uint256(obj, "r", &tx->r);
    json_get_hex_uint256(obj, "s", &tx->s);

    break;
  }

  case 4: {
    // EIP-7702 SetCode transaction
    out->type = TX_TYPE_EIP7702;
    eip7702_tx_t *tx = &out->eip7702;
    eip7702_tx_init(tx);

    json_get_hex_u64(obj, "chainId", &tx->chain_id);
    json_get_hex_u64(obj, "nonce", &tx->nonce);
    json_get_hex_uint256(obj, "maxPriorityFeePerGas", &tx->max_priority_fee_per_gas);
    json_get_hex_uint256(obj, "maxFeePerGas", &tx->max_fee_per_gas);
    json_get_hex_u64(obj, "gas", &tx->gas_limit);
    if (!json_get_hex_u64(obj, "gas", &tx->gas_limit)) {
      json_get_hex_u64(obj, "gasLimit", &tx->gas_limit);
    }
    json_get_hex_uint256(obj, "value", &tx->value);
    json_get_hex_bytes(obj, "input", arena, &tx->data);
    if (tx->data.size == 0) {
      json_get_hex_bytes(obj, "data", arena, &tx->data);
    }

    // To address (required for EIP-7702)
    if (!json_get_hex_address(obj, "to", &tx->to)) {
      return json_err(JSON_ERR_MISSING_FIELD, "EIP-7702 transaction requires to address");
    }

    // Access list
    result = parse_access_list(json_obj_get(obj, "accessList"), arena, &tx->access_list);
    if (json_is_err(result)) {
      return result;
    }

    // Authorization list
    result = parse_authorization_list(json_obj_get(obj, "authorizationList"), arena,
                                      &tx->authorization_list);
    if (json_is_err(result)) {
      return result;
    }

    // Signature
    uint64_t y;
    if (json_get_hex_u64(obj, "yParity", &y)) {
      tx->y_parity = (uint8_t)y;
    } else if (json_get_hex_u64(obj, "v", &y)) {
      tx->y_parity = (uint8_t)(y & 1);
    }
    json_get_hex_uint256(obj, "r", &tx->r);
    json_get_hex_uint256(obj, "s", &tx->s);

    break;
  }

  default:
    return json_err(JSON_ERR_INVALID_TYPE, "unsupported transaction type");
  }

  (void)is_protected; // May be used for legacy tx v encoding
  return json_ok();
}

json_result_t t8n_parse_txs_value(yyjson_val_t *arr, div0_arena_t *arena, t8n_txs_t *out) {
  if (arr == nullptr || !json_is_arr(arr)) {
    return json_err(JSON_ERR_INVALID_TYPE, "txs must be an array");
  }

  size_t count = json_arr_len(arr);
  if (count == 0) {
    out->txs = nullptr;
    out->tx_count = 0;
    return json_ok();
  }

  out->txs = div0_arena_alloc(arena, count * sizeof(transaction_t));
  if (out->txs == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate transactions");
  }
  out->tx_count = count;

  json_arr_iter_t iter = json_arr_iter(arr);
  yyjson_val_t *tx_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &tx_val)) {
    json_result_t result = t8n_parse_tx(tx_val, arena, &out->txs[idx]);
    if (json_is_err(result)) {
      return result;
    }
    idx++;
  }

  return json_ok();
}

json_result_t t8n_parse_txs(const char *json, size_t len, div0_arena_t *arena, t8n_txs_t *out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse(json, len, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *root = json_doc_root(&doc);
  result = t8n_parse_txs_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

json_result_t t8n_parse_txs_file(const char *path, div0_arena_t *arena, t8n_txs_t *out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse_file(path, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *root = json_doc_root(&doc);
  result = t8n_parse_txs_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

// ============================================================================
// Serialization Helpers
// ============================================================================

/// Serialize an access list to JSON array.
static yyjson_mut_val_t *write_access_list(const access_list_t *list, json_writer_t *w) {
  yyjson_mut_val_t *arr = json_write_arr(w);
  if (arr == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < list->count; i++) {
    const access_list_entry_t *entry = &list->entries[i];

    yyjson_mut_val_t *entry_obj = json_write_obj(w);
    if (entry_obj == nullptr) {
      return nullptr;
    }

    json_obj_add_hex_address(w, entry_obj, "address", &entry->address);

    // Storage keys array
    yyjson_mut_val_t *keys_arr = json_write_arr(w);
    if (keys_arr == nullptr) {
      return nullptr;
    }

    for (size_t j = 0; j < entry->storage_keys_count; j++) {
      yyjson_mut_val_t *key = json_write_hex_uint256_padded(w, &entry->storage_keys[j]);
      if (key == nullptr) {
        return nullptr;
      }
      json_arr_append(w, keys_arr, key);
    }

    json_obj_add(w, entry_obj, "storageKeys", keys_arr);
    json_arr_append(w, arr, entry_obj);
  }

  return arr;
}

/// Serialize blob versioned hashes to JSON array.
static yyjson_mut_val_t *write_blob_hashes(const hash_t *hashes, size_t count, json_writer_t *w) {
  yyjson_mut_val_t *arr = json_write_arr(w);
  if (arr == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < count; i++) {
    yyjson_mut_val_t *hash = json_write_hex_hash(w, &hashes[i]);
    if (hash == nullptr) {
      return nullptr;
    }
    json_arr_append(w, arr, hash);
  }

  return arr;
}

/// Serialize an authorization list to JSON array.
static yyjson_mut_val_t *write_authorization_list(const authorization_list_t *list,
                                                  json_writer_t *w) {
  yyjson_mut_val_t *arr = json_write_arr(w);
  if (arr == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < list->count; i++) {
    const authorization_t *auth = &list->entries[i];

    yyjson_mut_val_t *auth_obj = json_write_obj(w);
    if (auth_obj == nullptr) {
      return nullptr;
    }

    json_obj_add_hex_u64(w, auth_obj, "chainId", auth->chain_id);
    json_obj_add_hex_address(w, auth_obj, "address", &auth->address);
    json_obj_add_hex_u64(w, auth_obj, "nonce", auth->nonce);
    json_obj_add_hex_u64(w, auth_obj, "yParity", auth->y_parity);
    json_obj_add_hex_uint256(w, auth_obj, "r", &auth->r);
    json_obj_add_hex_uint256(w, auth_obj, "s", &auth->s);

    json_arr_append(w, arr, auth_obj);
  }

  return arr;
}

// ============================================================================
// Serialization
// ============================================================================

yyjson_mut_val_t *t8n_write_tx(const transaction_t *tx, json_writer_t *w) {
  yyjson_mut_val_t *obj = json_write_obj(w);
  if (obj == nullptr) {
    return nullptr;
  }

  // Type field
  json_obj_add_hex_u64(w, obj, "type", (uint64_t)tx->type);

  switch (tx->type) {
  case TX_TYPE_LEGACY: {
    const legacy_tx_t *ltx = &tx->legacy;
    json_obj_add_hex_u64(w, obj, "nonce", ltx->nonce);
    json_obj_add_hex_uint256(w, obj, "gasPrice", &ltx->gas_price);
    json_obj_add_hex_u64(w, obj, "gas", ltx->gas_limit);
    if (ltx->to != nullptr) {
      json_obj_add_hex_address(w, obj, "to", ltx->to);
    } else {
      json_obj_add_null(w, obj, "to");
    }
    json_obj_add_hex_uint256(w, obj, "value", &ltx->value);
    json_obj_add_hex_bytes(w, obj, "input", ltx->data.data, ltx->data.size);
    json_obj_add_hex_u64(w, obj, "v", ltx->v);
    json_obj_add_hex_uint256(w, obj, "r", &ltx->r);
    json_obj_add_hex_uint256(w, obj, "s", &ltx->s);
    break;
  }

  case TX_TYPE_EIP2930: {
    const eip2930_tx_t *etx = &tx->eip2930;
    json_obj_add_hex_u64(w, obj, "chainId", etx->chain_id);
    json_obj_add_hex_u64(w, obj, "nonce", etx->nonce);
    json_obj_add_hex_uint256(w, obj, "gasPrice", &etx->gas_price);
    json_obj_add_hex_u64(w, obj, "gas", etx->gas_limit);
    if (etx->to != nullptr) {
      json_obj_add_hex_address(w, obj, "to", etx->to);
    } else {
      json_obj_add_null(w, obj, "to");
    }
    json_obj_add_hex_uint256(w, obj, "value", &etx->value);
    json_obj_add_hex_bytes(w, obj, "input", etx->data.data, etx->data.size);
    json_obj_add(w, obj, "accessList", write_access_list(&etx->access_list, w));
    json_obj_add_hex_u64(w, obj, "yParity", etx->y_parity);
    json_obj_add_hex_uint256(w, obj, "r", &etx->r);
    json_obj_add_hex_uint256(w, obj, "s", &etx->s);
    break;
  }

  case TX_TYPE_EIP1559: {
    const eip1559_tx_t *etx = &tx->eip1559;
    json_obj_add_hex_u64(w, obj, "chainId", etx->chain_id);
    json_obj_add_hex_u64(w, obj, "nonce", etx->nonce);
    json_obj_add_hex_uint256(w, obj, "maxPriorityFeePerGas", &etx->max_priority_fee_per_gas);
    json_obj_add_hex_uint256(w, obj, "maxFeePerGas", &etx->max_fee_per_gas);
    json_obj_add_hex_u64(w, obj, "gas", etx->gas_limit);
    if (etx->to != nullptr) {
      json_obj_add_hex_address(w, obj, "to", etx->to);
    } else {
      json_obj_add_null(w, obj, "to");
    }
    json_obj_add_hex_uint256(w, obj, "value", &etx->value);
    json_obj_add_hex_bytes(w, obj, "input", etx->data.data, etx->data.size);
    json_obj_add(w, obj, "accessList", write_access_list(&etx->access_list, w));
    json_obj_add_hex_u64(w, obj, "yParity", etx->y_parity);
    json_obj_add_hex_uint256(w, obj, "r", &etx->r);
    json_obj_add_hex_uint256(w, obj, "s", &etx->s);
    break;
  }

  case TX_TYPE_EIP4844: {
    const eip4844_tx_t *btx = &tx->eip4844;
    json_obj_add_hex_u64(w, obj, "chainId", btx->chain_id);
    json_obj_add_hex_u64(w, obj, "nonce", btx->nonce);
    json_obj_add_hex_uint256(w, obj, "maxPriorityFeePerGas", &btx->max_priority_fee_per_gas);
    json_obj_add_hex_uint256(w, obj, "maxFeePerGas", &btx->max_fee_per_gas);
    json_obj_add_hex_u64(w, obj, "gas", btx->gas_limit);
    json_obj_add_hex_address(w, obj, "to", &btx->to);
    json_obj_add_hex_uint256(w, obj, "value", &btx->value);
    json_obj_add_hex_bytes(w, obj, "input", btx->data.data, btx->data.size);
    json_obj_add_hex_uint256(w, obj, "maxFeePerBlobGas", &btx->max_fee_per_blob_gas);
    json_obj_add(w, obj, "blobVersionedHashes",
                 write_blob_hashes(btx->blob_versioned_hashes, btx->blob_hashes_count, w));
    json_obj_add(w, obj, "accessList", write_access_list(&btx->access_list, w));
    json_obj_add_hex_u64(w, obj, "yParity", btx->y_parity);
    json_obj_add_hex_uint256(w, obj, "r", &btx->r);
    json_obj_add_hex_uint256(w, obj, "s", &btx->s);
    break;
  }

  case TX_TYPE_EIP7702: {
    const eip7702_tx_t *stx = &tx->eip7702;
    json_obj_add_hex_u64(w, obj, "chainId", stx->chain_id);
    json_obj_add_hex_u64(w, obj, "nonce", stx->nonce);
    json_obj_add_hex_uint256(w, obj, "maxPriorityFeePerGas", &stx->max_priority_fee_per_gas);
    json_obj_add_hex_uint256(w, obj, "maxFeePerGas", &stx->max_fee_per_gas);
    json_obj_add_hex_u64(w, obj, "gas", stx->gas_limit);
    json_obj_add_hex_address(w, obj, "to", &stx->to);
    json_obj_add_hex_uint256(w, obj, "value", &stx->value);
    json_obj_add_hex_bytes(w, obj, "input", stx->data.data, stx->data.size);
    json_obj_add(w, obj, "accessList", write_access_list(&stx->access_list, w));
    json_obj_add(w, obj, "authorizationList",
                 write_authorization_list(&stx->authorization_list, w));
    json_obj_add_hex_u64(w, obj, "yParity", stx->y_parity);
    json_obj_add_hex_uint256(w, obj, "r", &stx->r);
    json_obj_add_hex_uint256(w, obj, "s", &stx->s);
    break;
  }
  }

  return obj;
}

yyjson_mut_val_t *t8n_write_txs(const t8n_txs_t *txs, json_writer_t *w) {
  yyjson_mut_val_t *arr = json_write_arr(w);
  if (arr == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < txs->tx_count; i++) {
    yyjson_mut_val_t *tx_obj = t8n_write_tx(&txs->txs[i], w);
    if (tx_obj == nullptr) {
      return nullptr;
    }
    json_arr_append(w, arr, tx_obj);
  }

  return arr;
}

#endif // DIV0_FREESTANDING
