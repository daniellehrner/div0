#include "div0/types/bytes32.h"

#include "div0/types/uint256.h"

#include <stdint.h>
#include <string.h>

bytes32_t bytes32_from_bytes_padded(const uint8_t *const data, const size_t len) {
  bytes32_t result = bytes32_zero();

  if (data == nullptr || len == 0) {
    return result;
  }

  const size_t copy_len = len < BYTES32_SIZE ? len : BYTES32_SIZE;
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.bytes, data, copy_len);

  return result;
}

uint256_t bytes32_to_uint256(const bytes32_t *b) {
  // bytes32 is big-endian, uint256 expects big-endian input
  return uint256_from_bytes_be(b->bytes, BYTES32_SIZE);
}

bytes32_t bytes32_from_uint256(const uint256_t *value) {
  bytes32_t result;
  uint256_to_bytes_be(*value, result.bytes);
  return result;
}