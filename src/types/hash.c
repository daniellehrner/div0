#include "div0/types/hash.h"

#include "div0/types/uint256.h"

uint256_t hash_to_uint256(const hash_t *h) {
  // hash is big-endian, uint256 expects big-endian input
  return uint256_from_bytes_be(h->bytes, HASH_SIZE);
}

hash_t hash_from_uint256(const uint256_t *value) {
  hash_t result;
  uint256_to_bytes_be(*value, result.bytes);
  return result;
}