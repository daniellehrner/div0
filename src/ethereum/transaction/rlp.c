#include "div0/ethereum/transaction/rlp.h"

#include "div0/crypto/keccak256.h"
#include "div0/mem/stc_allocator.h"
#include "div0/rlp/decode.h"
#include "div0/rlp/encode.h"

// STC vec types for single-pass decoding
#define i_type vec_u256
#define i_key uint256_t
#include "stc/vec.h"

#define i_type vec_access_entry
#define i_key access_list_entry_t
#include "stc/vec.h"

#define i_type vec_auth
#define i_key authorization_t
#include "stc/vec.h"

#include <string.h>

// ============================================================================
// Error Strings
// ============================================================================

const char *tx_decode_error_string(tx_decode_error_t error) {
  switch (error) {
  case TX_DECODE_OK:
    return "success";
  case TX_DECODE_EMPTY_INPUT:
    return "empty input";
  case TX_DECODE_INVALID_TYPE:
    return "invalid transaction type";
  case TX_DECODE_INVALID_RLP:
    return "invalid RLP encoding";
  case TX_DECODE_INVALID_SIGNATURE:
    return "invalid signature";
  case TX_DECODE_MISSING_FIELD:
    return "missing required field";
  case TX_DECODE_INVALID_FIELD:
    return "invalid field value";
  case TX_DECODE_ALLOC_FAILED:
    return "memory allocation failed";
  }
  return "unknown error";
}

// ============================================================================
// Decode Helpers - Macros to reduce boilerplate
// ============================================================================

#define DECODE_U64(dec, field)                                                          \
  do {                                                                                  \
    rlp_u64_result_t _r = rlp_decode_u64(dec);                                          \
    if (_r.error != RLP_SUCCESS)                                                        \
      return (tx_decode_result_t){.error = TX_DECODE_INVALID_RLP, .bytes_consumed = 0}; \
    (field) = _r.value;                                                                 \
  } while (0)

#define DECODE_UINT256(dec, field)                                                      \
  do {                                                                                  \
    rlp_uint256_result_t _r = rlp_decode_uint256(dec);                                  \
    if (_r.error != RLP_SUCCESS)                                                        \
      return (tx_decode_result_t){.error = TX_DECODE_INVALID_RLP, .bytes_consumed = 0}; \
    (field) = _r.value;                                                                 \
  } while (0)

#define DECODE_YPARITY(dec, field)                                                      \
  do {                                                                                  \
    rlp_u64_result_t _r = rlp_decode_u64(dec);                                          \
    if (_r.error != RLP_SUCCESS)                                                        \
      return (tx_decode_result_t){.error = TX_DECODE_INVALID_RLP, .bytes_consumed = 0}; \
    (field) = (uint8_t)_r.value;                                                        \
  } while (0)

#define CHECK_HELPER(err)                                            \
  do {                                                               \
    tx_decode_error_t _e = (err);                                    \
    if (_e != TX_DECODE_OK)                                          \
      return (tx_decode_result_t){.error = _e, .bytes_consumed = 0}; \
  } while (0)

#define DECODE_LIST_HEADER(dec, var)                   \
  rlp_list_result_t var = rlp_decode_list_header(dec); \
  if (var.error != RLP_SUCCESS)                        \
  return (tx_decode_result_t){.error = TX_DECODE_INVALID_RLP, .bytes_consumed = 0}

#define RETURN_SUCCESS(list_header)                                               \
  return (tx_decode_result_t) {                                                   \
    .error = TX_DECODE_OK,                                                        \
    .bytes_consumed = (list_header).bytes_consumed + (list_header).payload_length \
  }

// ============================================================================
// Decode Helpers - Functions
// ============================================================================

static tx_decode_error_t decode_optional_address(rlp_decoder_t *dec, address_t **out,
                                                 div0_arena_t *arena) {
  rlp_bytes_result_t result = rlp_decode_bytes(dec);
  if (result.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }
  if (result.len == 0) {
    *out = nullptr;
    return TX_DECODE_OK;
  }
  if (result.len != ADDRESS_SIZE) {
    return TX_DECODE_INVALID_FIELD;
  }
  *out = (address_t *)div0_arena_alloc(arena, sizeof(address_t));
  if (!*out) {
    return TX_DECODE_ALLOC_FAILED;
  }
  memcpy((*out)->bytes, result.data, ADDRESS_SIZE);
  return TX_DECODE_OK;
}

static tx_decode_error_t decode_required_address(rlp_decoder_t *dec, address_t *out) {
  rlp_address_result_t result = rlp_decode_address(dec);
  if (result.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }
  *out = result.value;
  return TX_DECODE_OK;
}

static tx_decode_error_t decode_tx_data(rlp_decoder_t *dec, bytes_t *out, div0_arena_t *arena) {
  rlp_bytes_result_t result = rlp_decode_bytes(dec);
  if (result.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }
  bytes_init_arena(out, arena);
  if (result.len > 0) {
    if (!bytes_reserve(out, result.len)) {
      return TX_DECODE_ALLOC_FAILED;
    }
    memcpy(out->data, result.data, result.len);
    out->size = result.len;
  }
  return TX_DECODE_OK;
}

static tx_decode_error_t decode_access_list(rlp_decoder_t *dec, access_list_t *list,
                                            div0_arena_t *arena) {
  rlp_list_result_t list_header = rlp_decode_list_header(dec);
  if (list_header.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }
  if (list_header.payload_length == 0) {
    access_list_init(list);
    return TX_DECODE_OK;
  }

  // Set up STC arena for single-pass decoding
  div0_stc_arena = arena;
  size_t list_end = dec->pos + list_header.payload_length;

  // Single-pass: decode entries directly into a growing vector
  vec_access_entry entries = vec_access_entry_init();

  while (dec->pos < list_end) {
    rlp_list_result_t entry_header = rlp_decode_list_header(dec);
    if (entry_header.error != RLP_SUCCESS) {
      return TX_DECODE_INVALID_RLP;
    }

    access_list_entry_t entry;

    rlp_address_result_t addr_result = rlp_decode_address(dec);
    if (addr_result.error != RLP_SUCCESS) {
      return TX_DECODE_INVALID_RLP;
    }
    entry.address = addr_result.value;

    rlp_list_result_t keys_header = rlp_decode_list_header(dec);
    if (keys_header.error != RLP_SUCCESS) {
      return TX_DECODE_INVALID_RLP;
    }

    if (keys_header.payload_length == 0) {
      entry.storage_keys = nullptr;
      entry.storage_keys_count = 0;
    } else {
      // Single-pass for storage keys too
      size_t keys_end = dec->pos + keys_header.payload_length;
      vec_u256 keys = vec_u256_init();

      while (dec->pos < keys_end) {
        rlp_uint256_result_t key_result = rlp_decode_uint256(dec);
        if (key_result.error != RLP_SUCCESS) {
          return TX_DECODE_INVALID_RLP;
        }
        vec_u256_push(&keys, key_result.value);
      }

      entry.storage_keys = keys.data;
      entry.storage_keys_count = (size_t)vec_u256_size(&keys);
    }

    vec_access_entry_push(&entries, entry);
  }

  // Transfer ownership from vec to access_list
  list->entries = entries.data;
  list->count = (size_t)vec_access_entry_size(&entries);

  return TX_DECODE_OK;
}

static tx_decode_error_t decode_authorization_list(rlp_decoder_t *dec, authorization_list_t *list,
                                                   div0_arena_t *arena) {
  rlp_list_result_t list_header = rlp_decode_list_header(dec);
  if (list_header.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }
  if (list_header.payload_length == 0) {
    authorization_list_init(list);
    return TX_DECODE_OK;
  }

  // Set up STC arena for single-pass decoding
  div0_stc_arena = arena;
  size_t list_end = dec->pos + list_header.payload_length;

  // Single-pass: decode authorizations directly into a growing vector
  vec_auth auths = vec_auth_init();

  while (dec->pos < list_end) {
    rlp_list_result_t auth_header = rlp_decode_list_header(dec);
    if (auth_header.error != RLP_SUCCESS) {
      return TX_DECODE_INVALID_RLP;
    }

    authorization_t auth;

    rlp_u64_result_t chain_id = rlp_decode_u64(dec);
    if (chain_id.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.chain_id = chain_id.value;

    rlp_address_result_t addr = rlp_decode_address(dec);
    if (addr.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.address = addr.value;

    rlp_u64_result_t nonce = rlp_decode_u64(dec);
    if (nonce.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.nonce = nonce.value;

    rlp_u64_result_t y_parity = rlp_decode_u64(dec);
    if (y_parity.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.y_parity = (uint8_t)y_parity.value;

    rlp_uint256_result_t r = rlp_decode_uint256(dec);
    if (r.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.r = r.value;

    rlp_uint256_result_t s = rlp_decode_uint256(dec);
    if (s.error != RLP_SUCCESS)
      return TX_DECODE_INVALID_RLP;
    auth.s = s.value;

    vec_auth_push(&auths, auth);
  }

  // Transfer ownership from vec to authorization_list
  list->entries = auths.data;
  list->count = (size_t)vec_auth_size(&auths);

  return TX_DECODE_OK;
}

static tx_decode_error_t decode_blob_hashes(rlp_decoder_t *dec, eip4844_tx_t *tx,
                                            div0_arena_t *arena) {
  rlp_list_result_t hashes_header = rlp_decode_list_header(dec);
  if (hashes_header.error != RLP_SUCCESS) {
    return TX_DECODE_INVALID_RLP;
  }

  if (hashes_header.payload_length == 0) {
    tx->blob_versioned_hashes = nullptr;
    tx->blob_hashes_count = 0;
    return TX_DECODE_OK;
  }

  size_t hashes_end = dec->pos + hashes_header.payload_length;
  size_t start_pos = dec->pos;
  size_t hash_count = 0;

  while (dec->pos < hashes_end) {
    rlp_skip_item(dec);
    hash_count++;
  }

  if (!eip4844_tx_alloc_blob_hashes(tx, hash_count, arena)) {
    return TX_DECODE_ALLOC_FAILED;
  }

  dec->pos = start_pos;
  for (size_t i = 0; i < hash_count; i++) {
    rlp_bytes_result_t hash_bytes = rlp_decode_bytes(dec);
    if (hash_bytes.error != RLP_SUCCESS || hash_bytes.len != HASH_SIZE) {
      return TX_DECODE_INVALID_FIELD;
    }
    tx->blob_versioned_hashes[i] = hash_from_bytes(hash_bytes.data);
  }

  return TX_DECODE_OK;
}

// ============================================================================
// Transaction Decoding
// ============================================================================

tx_decode_result_t legacy_tx_decode(const uint8_t *data, size_t len, legacy_tx_t *tx,
                                    div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  legacy_tx_init(tx);
  rlp_decoder_t dec;
  rlp_decoder_init(&dec, data, len);

  DECODE_LIST_HEADER(&dec, list_header);

  DECODE_U64(&dec, tx->nonce);
  DECODE_UINT256(&dec, tx->gas_price);
  DECODE_U64(&dec, tx->gas_limit);
  CHECK_HELPER(decode_optional_address(&dec, &tx->to, arena));
  DECODE_UINT256(&dec, tx->value);
  CHECK_HELPER(decode_tx_data(&dec, &tx->data, arena));
  DECODE_U64(&dec, tx->v);
  DECODE_UINT256(&dec, tx->r);
  DECODE_UINT256(&dec, tx->s);

  RETURN_SUCCESS(list_header);
}

tx_decode_result_t eip2930_tx_decode(const uint8_t *data, size_t len, eip2930_tx_t *tx,
                                     div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  eip2930_tx_init(tx);
  rlp_decoder_t dec;
  rlp_decoder_init(&dec, data, len);

  DECODE_LIST_HEADER(&dec, list_header);

  DECODE_U64(&dec, tx->chain_id);
  DECODE_U64(&dec, tx->nonce);
  DECODE_UINT256(&dec, tx->gas_price);
  DECODE_U64(&dec, tx->gas_limit);
  CHECK_HELPER(decode_optional_address(&dec, &tx->to, arena));
  DECODE_UINT256(&dec, tx->value);
  CHECK_HELPER(decode_tx_data(&dec, &tx->data, arena));
  CHECK_HELPER(decode_access_list(&dec, &tx->access_list, arena));
  DECODE_YPARITY(&dec, tx->y_parity);
  DECODE_UINT256(&dec, tx->r);
  DECODE_UINT256(&dec, tx->s);

  RETURN_SUCCESS(list_header);
}

tx_decode_result_t eip1559_tx_decode(const uint8_t *data, size_t len, eip1559_tx_t *tx,
                                     div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  eip1559_tx_init(tx);
  rlp_decoder_t dec;
  rlp_decoder_init(&dec, data, len);

  DECODE_LIST_HEADER(&dec, list_header);

  DECODE_U64(&dec, tx->chain_id);
  DECODE_U64(&dec, tx->nonce);
  DECODE_UINT256(&dec, tx->max_priority_fee_per_gas);
  DECODE_UINT256(&dec, tx->max_fee_per_gas);
  DECODE_U64(&dec, tx->gas_limit);
  CHECK_HELPER(decode_optional_address(&dec, &tx->to, arena));
  DECODE_UINT256(&dec, tx->value);
  CHECK_HELPER(decode_tx_data(&dec, &tx->data, arena));
  CHECK_HELPER(decode_access_list(&dec, &tx->access_list, arena));
  DECODE_YPARITY(&dec, tx->y_parity);
  DECODE_UINT256(&dec, tx->r);
  DECODE_UINT256(&dec, tx->s);

  RETURN_SUCCESS(list_header);
}

tx_decode_result_t eip4844_tx_decode(const uint8_t *data, size_t len, eip4844_tx_t *tx,
                                     div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  eip4844_tx_init(tx);
  rlp_decoder_t dec;
  rlp_decoder_init(&dec, data, len);

  DECODE_LIST_HEADER(&dec, list_header);

  DECODE_U64(&dec, tx->chain_id);
  DECODE_U64(&dec, tx->nonce);
  DECODE_UINT256(&dec, tx->max_priority_fee_per_gas);
  DECODE_UINT256(&dec, tx->max_fee_per_gas);
  DECODE_U64(&dec, tx->gas_limit);
  CHECK_HELPER(decode_required_address(&dec, &tx->to));
  DECODE_UINT256(&dec, tx->value);
  CHECK_HELPER(decode_tx_data(&dec, &tx->data, arena));
  CHECK_HELPER(decode_access_list(&dec, &tx->access_list, arena));
  DECODE_UINT256(&dec, tx->max_fee_per_blob_gas);
  CHECK_HELPER(decode_blob_hashes(&dec, tx, arena));
  DECODE_YPARITY(&dec, tx->y_parity);
  DECODE_UINT256(&dec, tx->r);
  DECODE_UINT256(&dec, tx->s);

  RETURN_SUCCESS(list_header);
}

tx_decode_result_t eip7702_tx_decode(const uint8_t *data, size_t len, eip7702_tx_t *tx,
                                     div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  eip7702_tx_init(tx);
  rlp_decoder_t dec;
  rlp_decoder_init(&dec, data, len);

  DECODE_LIST_HEADER(&dec, list_header);

  DECODE_U64(&dec, tx->chain_id);
  DECODE_U64(&dec, tx->nonce);
  DECODE_UINT256(&dec, tx->max_priority_fee_per_gas);
  DECODE_UINT256(&dec, tx->max_fee_per_gas);
  DECODE_U64(&dec, tx->gas_limit);
  CHECK_HELPER(decode_required_address(&dec, &tx->to));
  DECODE_UINT256(&dec, tx->value);
  CHECK_HELPER(decode_tx_data(&dec, &tx->data, arena));
  CHECK_HELPER(decode_access_list(&dec, &tx->access_list, arena));
  CHECK_HELPER(decode_authorization_list(&dec, &tx->authorization_list, arena));
  DECODE_YPARITY(&dec, tx->y_parity);
  DECODE_UINT256(&dec, tx->r);
  DECODE_UINT256(&dec, tx->s);

  RETURN_SUCCESS(list_header);
}

tx_decode_result_t transaction_decode(const uint8_t *data, size_t len, transaction_t *tx,
                                      div0_arena_t *arena) {
  if (len == 0) {
    return (tx_decode_result_t){.error = TX_DECODE_EMPTY_INPUT, .bytes_consumed = 0};
  }

  uint8_t first_byte = data[0];

  if (first_byte >= 0xc0) {
    tx->type = TX_TYPE_LEGACY;
    return legacy_tx_decode(data, len, &tx->legacy, arena);
  }

  if (first_byte == 0x00 || first_byte > 0x04) {
    return (tx_decode_result_t){.error = TX_DECODE_INVALID_TYPE, .bytes_consumed = 0};
  }

  tx_decode_result_t result;
  switch (first_byte) {
  case 0x01:
    tx->type = TX_TYPE_EIP2930;
    result = eip2930_tx_decode(data + 1, len - 1, &tx->eip2930, arena);
    break;
  case 0x02:
    tx->type = TX_TYPE_EIP1559;
    result = eip1559_tx_decode(data + 1, len - 1, &tx->eip1559, arena);
    break;
  case 0x03:
    tx->type = TX_TYPE_EIP4844;
    result = eip4844_tx_decode(data + 1, len - 1, &tx->eip4844, arena);
    break;
  case 0x04:
    tx->type = TX_TYPE_EIP7702;
    result = eip7702_tx_decode(data + 1, len - 1, &tx->eip7702, arena);
    break;
  default:
    return (tx_decode_result_t){.error = TX_DECODE_INVALID_TYPE, .bytes_consumed = 0};
  }

  if (result.error == TX_DECODE_OK) {
    result.bytes_consumed += 1;
  }
  return result;
}

// ============================================================================
// Encoding Helpers (public, used by signer.c as well)
// ============================================================================

bytes_t tx_encode_optional_address(div0_arena_t *arena, const address_t *addr) {
  if (addr == nullptr) {
    return rlp_encode_bytes(arena, nullptr, 0);
  }
  return rlp_encode_address(arena, addr);
}

void tx_encode_access_list(bytes_t *output, const access_list_t *list, div0_arena_t *arena) {
  rlp_list_builder_t list_builder;
  rlp_list_start(&list_builder, output);

  for (size_t i = 0; i < list->count; i++) {
    rlp_list_builder_t entry_builder;
    rlp_list_start(&entry_builder, output);

    bytes_t addr_rlp = rlp_encode_address(arena, &list->entries[i].address);
    rlp_list_append(output, &addr_rlp);

    rlp_list_builder_t keys_builder;
    rlp_list_start(&keys_builder, output);
    for (size_t j = 0; j < list->entries[i].storage_keys_count; j++) {
      bytes_t key_rlp = rlp_encode_uint256(arena, &list->entries[i].storage_keys[j]);
      rlp_list_append(output, &key_rlp);
    }
    rlp_list_end(&keys_builder);

    rlp_list_end(&entry_builder);
  }

  rlp_list_end(&list_builder);
}

void tx_encode_authorization_list(bytes_t *output, const authorization_list_t *list,
                                  div0_arena_t *arena) {
  rlp_list_builder_t list_builder;
  rlp_list_start(&list_builder, output);

  for (size_t i = 0; i < list->count; i++) {
    rlp_list_builder_t auth_builder;
    rlp_list_start(&auth_builder, output);

    bytes_t chain_id_rlp = rlp_encode_u64(arena, list->entries[i].chain_id);
    rlp_list_append(output, &chain_id_rlp);

    bytes_t addr_rlp = rlp_encode_address(arena, &list->entries[i].address);
    rlp_list_append(output, &addr_rlp);

    bytes_t nonce_rlp = rlp_encode_u64(arena, list->entries[i].nonce);
    rlp_list_append(output, &nonce_rlp);

    bytes_t y_parity_rlp = rlp_encode_u64(arena, list->entries[i].y_parity);
    rlp_list_append(output, &y_parity_rlp);

    bytes_t r_rlp = rlp_encode_uint256(arena, &list->entries[i].r);
    rlp_list_append(output, &r_rlp);

    bytes_t s_rlp = rlp_encode_uint256(arena, &list->entries[i].s);
    rlp_list_append(output, &s_rlp);

    rlp_list_end(&auth_builder);
  }

  rlp_list_end(&list_builder);
}

// ============================================================================
// Transaction Encoding
// ============================================================================

bytes_t legacy_tx_encode(const legacy_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 256);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t gas_price_rlp = rlp_encode_uint256(arena, &tx->gas_price);
  rlp_list_append(&output, &gas_price_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  bytes_t v_rlp = rlp_encode_u64(arena, tx->v);
  rlp_list_append(&output, &v_rlp);

  bytes_t r_rlp = rlp_encode_uint256(arena, &tx->r);
  rlp_list_append(&output, &r_rlp);

  bytes_t s_rlp = rlp_encode_uint256(arena, &tx->s);
  rlp_list_append(&output, &s_rlp);

  rlp_list_end(&builder);
  return output;
}

bytes_t eip2930_tx_encode(const eip2930_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 512);

  bytes_append_byte(&output, 0x01);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t gas_price_rlp = rlp_encode_uint256(arena, &tx->gas_price);
  rlp_list_append(&output, &gas_price_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  bytes_t y_parity_rlp = rlp_encode_u64(arena, tx->y_parity);
  rlp_list_append(&output, &y_parity_rlp);

  bytes_t r_rlp = rlp_encode_uint256(arena, &tx->r);
  rlp_list_append(&output, &r_rlp);

  bytes_t s_rlp = rlp_encode_uint256(arena, &tx->s);
  rlp_list_append(&output, &s_rlp);

  rlp_list_end(&builder);
  return output;
}

bytes_t eip1559_tx_encode(const eip1559_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 512);

  bytes_append_byte(&output, 0x02);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  bytes_t y_parity_rlp = rlp_encode_u64(arena, tx->y_parity);
  rlp_list_append(&output, &y_parity_rlp);

  bytes_t r_rlp = rlp_encode_uint256(arena, &tx->r);
  rlp_list_append(&output, &r_rlp);

  bytes_t s_rlp = rlp_encode_uint256(arena, &tx->s);
  rlp_list_append(&output, &s_rlp);

  rlp_list_end(&builder);
  return output;
}

bytes_t eip4844_tx_encode(const eip4844_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 1024);

  bytes_append_byte(&output, 0x03);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = rlp_encode_address(arena, &tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  bytes_t max_blob_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_blob_gas);
  rlp_list_append(&output, &max_blob_fee_rlp);

  // Blob versioned hashes
  rlp_list_builder_t hashes_builder;
  rlp_list_start(&hashes_builder, &output);
  for (size_t i = 0; i < tx->blob_hashes_count; i++) {
    bytes_t hash_rlp = rlp_encode_bytes(arena, tx->blob_versioned_hashes[i].bytes, HASH_SIZE);
    rlp_list_append(&output, &hash_rlp);
  }
  rlp_list_end(&hashes_builder);

  bytes_t y_parity_rlp = rlp_encode_u64(arena, tx->y_parity);
  rlp_list_append(&output, &y_parity_rlp);

  bytes_t r_rlp = rlp_encode_uint256(arena, &tx->r);
  rlp_list_append(&output, &r_rlp);

  bytes_t s_rlp = rlp_encode_uint256(arena, &tx->s);
  rlp_list_append(&output, &s_rlp);

  rlp_list_end(&builder);
  return output;
}

bytes_t eip7702_tx_encode(const eip7702_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 1024);

  bytes_append_byte(&output, 0x04);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = rlp_encode_address(arena, &tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);
  tx_encode_authorization_list(&output, &tx->authorization_list, arena);

  bytes_t y_parity_rlp = rlp_encode_u64(arena, tx->y_parity);
  rlp_list_append(&output, &y_parity_rlp);

  bytes_t r_rlp = rlp_encode_uint256(arena, &tx->r);
  rlp_list_append(&output, &r_rlp);

  bytes_t s_rlp = rlp_encode_uint256(arena, &tx->s);
  rlp_list_append(&output, &s_rlp);

  rlp_list_end(&builder);
  return output;
}

bytes_t transaction_encode(const transaction_t *tx, div0_arena_t *arena) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return legacy_tx_encode(&tx->legacy, arena);
  case TX_TYPE_EIP2930:
    return eip2930_tx_encode(&tx->eip2930, arena);
  case TX_TYPE_EIP1559:
    return eip1559_tx_encode(&tx->eip1559, arena);
  case TX_TYPE_EIP4844:
    return eip4844_tx_encode(&tx->eip4844, arena);
  case TX_TYPE_EIP7702:
    return eip7702_tx_encode(&tx->eip7702, arena);
  }
  bytes_t empty;
  bytes_init(&empty);
  return empty;
}

// ============================================================================
// Transaction Hash
// ============================================================================

hash_t transaction_hash(const transaction_t *tx, div0_arena_t *arena) {
  bytes_t encoded = transaction_encode(tx, arena);
  return keccak256(encoded.data, encoded.size);
}
