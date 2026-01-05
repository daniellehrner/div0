#include "div0/state/account.h"

#include "div0/rlp/decode.h"
#include "div0/rlp/encode.h"
#include "div0/trie/node.h"

// Empty code hash: keccak256("") - pre-computed
// = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
const hash_t EMPTY_CODE_HASH = {.bytes = {0xc5, 0xd2, 0x46, 0x01, 0x86, 0xf7, 0x23, 0x3c,
                                          0x92, 0x7e, 0x7d, 0xb2, 0xdc, 0xc7, 0x03, 0xc0,
                                          0xe5, 0x00, 0xb6, 0x53, 0xca, 0x82, 0x27, 0x3b,
                                          0x7b, 0xfa, 0xd8, 0x04, 0x5d, 0x85, 0xa4, 0x70}};

account_t account_empty(void) {
  return (account_t){
      .nonce = 0,
      .balance = uint256_zero(),
      .storage_root = MPT_EMPTY_ROOT,
      .code_hash = EMPTY_CODE_HASH,
  };
}

bool account_is_empty(const account_t *acc) {
  if (acc->nonce != 0) {
    return false;
  }
  if (!uint256_is_zero(acc->balance)) {
    return false;
  }
  return hash_equal(&acc->code_hash, &EMPTY_CODE_HASH);
}

bytes_t account_rlp_encode(const account_t *acc, div0_arena_t *arena) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  // Reserve space for list: prefix + nonce + balance + storage_root + code_hash
  // Max size: 9 (list header) + 9 (nonce) + 33 (balance) + 33 (storage_root) + 33 (code_hash)
  // = ~117 bytes max
  bytes_reserve(&result, 128);

  // Start the list
  rlp_list_builder_t builder;
  rlp_list_start(&builder, &result);

  // Encode and append each field
  // 1. nonce (uint64)
  bytes_t nonce_rlp = rlp_encode_u64(arena, acc->nonce);
  rlp_list_append(&result, &nonce_rlp);

  // 2. balance (uint256)
  bytes_t balance_rlp = rlp_encode_uint256(arena, &acc->balance);
  rlp_list_append(&result, &balance_rlp);

  // 3. storage_root (32 bytes)
  bytes_t storage_root_rlp = rlp_encode_bytes(arena, acc->storage_root.bytes, HASH_SIZE);
  rlp_list_append(&result, &storage_root_rlp);

  // 4. code_hash (32 bytes)
  bytes_t code_hash_rlp = rlp_encode_bytes(arena, acc->code_hash.bytes, HASH_SIZE);
  rlp_list_append(&result, &code_hash_rlp);

  // Finalize the list
  rlp_list_end(&builder);

  return result;
}

bool account_rlp_decode(const uint8_t *data, size_t len, account_t *out) {
  if (data == nullptr || len == 0 || out == nullptr) {
    return false;
  }

  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, data, len);

  // Decode list header
  rlp_list_result_t list_result = rlp_decode_list_header(&decoder);
  if (list_result.error != RLP_SUCCESS) {
    return false;
  }

  // Verify list payload matches remaining data
  if (list_result.payload_length != rlp_decoder_remaining(&decoder)) {
    return false;
  }

  // 1. Decode nonce (uint64)
  rlp_u64_result_t nonce_result = rlp_decode_u64(&decoder);
  if (nonce_result.error != RLP_SUCCESS) {
    return false;
  }
  out->nonce = nonce_result.value;

  // 2. Decode balance (uint256)
  rlp_uint256_result_t balance_result = rlp_decode_uint256(&decoder);
  if (balance_result.error != RLP_SUCCESS) {
    return false;
  }
  out->balance = balance_result.value;

  // 3. Decode storage_root (32 bytes)
  rlp_bytes_result_t storage_root_result = rlp_decode_bytes(&decoder);
  if (storage_root_result.error != RLP_SUCCESS || storage_root_result.len != HASH_SIZE) {
    return false;
  }
  out->storage_root = hash_from_bytes(storage_root_result.data);

  // 4. Decode code_hash (32 bytes)
  rlp_bytes_result_t code_hash_result = rlp_decode_bytes(&decoder);
  if (code_hash_result.error != RLP_SUCCESS || code_hash_result.len != HASH_SIZE) {
    return false;
  }
  out->code_hash = hash_from_bytes(code_hash_result.data);

  // Verify we consumed all data - NOLINT for bool conversion
  return !rlp_decoder_has_more(&decoder); // NOLINT(readability-implicit-bool-conversion)
}
