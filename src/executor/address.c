#include "div0/crypto/keccak256.h"
#include "div0/executor/block_executor.h"
#include "div0/mem/arena.h"
#include "div0/rlp/encode.h"
#include "div0/types/bytes.h"

address_t compute_create_address(const address_t *const sender, const uint64_t nonce) {
  // CREATE address = keccak256(rlp([sender, nonce]))[12:]

  // Use stack-allocated arena for temporary RLP encoding
  div0_arena_t arena;
  (void)div0_arena_init(&arena);

  // Encode sender address (20 bytes)
  const bytes_t encoded_addr = rlp_encode_address(&arena, sender);

  // Encode nonce
  const bytes_t encoded_nonce = rlp_encode_u64(&arena, nonce);

  // Build RLP list: [sender, nonce]
  bytes_t list_data;
  bytes_init_arena(&list_data, &arena);
  (void)bytes_reserve(&list_data, 64);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &list_data);
  rlp_list_append(&list_data, &encoded_addr);
  rlp_list_append(&list_data, &encoded_nonce);
  rlp_list_end(&builder);

  // Hash the RLP-encoded list
  const hash_t hash = keccak256(list_data.data, list_data.size);

  // Take last 20 bytes (bytes 12-31)
  address_t result;
  __builtin___memcpy_chk(result.bytes, hash.bytes + 12, 20, sizeof(result.bytes));

  div0_arena_destroy(&arena);
  return result;
}

address_t compute_create2_address(const address_t *sender, const hash_t *salt,
                                  const hash_t *init_code_hash) {
  // CREATE2 address = keccak256(0xff ++ sender ++ salt ++ keccak256(init_code))[12:]

  // Build the preimage: 1 + 20 + 32 + 32 = 85 bytes
  uint8_t preimage[85];
  preimage[0] = 0xff;
  __builtin___memcpy_chk(preimage + 1, sender->bytes, 20, 84);
  __builtin___memcpy_chk(preimage + 21, salt->bytes, 32, 64);
  __builtin___memcpy_chk(preimage + 53, init_code_hash->bytes, 32, 32);

  // Hash the preimage
  const hash_t hash = keccak256(preimage, sizeof(preimage));

  // Take last 20 bytes (bytes 12-31)
  address_t result;
  __builtin___memcpy_chk(result.bytes, hash.bytes + 12, 20, sizeof(result.bytes));

  return result;
}
