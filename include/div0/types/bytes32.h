#ifndef DIV0_TYPES_BYTES32_H
#define DIV0_TYPES_BYTES32_H

#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// Size of bytes32 in bytes.
static constexpr size_t BYTES32_SIZE = 32;

/// 32-byte fixed-size array.
/// Storage is big-endian (EVM convention).
typedef struct {
  uint8_t bytes[BYTES32_SIZE];
} bytes32_t;

static_assert(sizeof(bytes32_t) == BYTES32_SIZE, "bytes32_t must be 32 bytes");

/// Returns a zero-initialized bytes32.
static inline bytes32_t bytes32_zero(void) {
  return (bytes32_t){{0}};
}

/// Checks if a bytes32 is zero.
static inline bool bytes32_is_zero(const bytes32_t *b) {
  for (size_t i = 0; i < BYTES32_SIZE; i++) {
    if (b->bytes[i] != 0) {
      return false;
    }
  }
  return true;
}

/// Compares two bytes32 values for equality.
static inline bool bytes32_equal(const bytes32_t *a, const bytes32_t *b) {
  return memcmp(a->bytes, b->bytes, BYTES32_SIZE) == 0;
}

/// Creates a bytes32 from a byte array.
/// @param data Source data (must be exactly 32 bytes)
/// @return bytes32 containing the data
static inline bytes32_t bytes32_from_bytes(const uint8_t *data) {
  bytes32_t result;
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.bytes, data, BYTES32_SIZE);
  return result;
}

/// Creates a bytes32 from a byte array with padding.
/// If len < 32, the value is zero-padded on the right.
/// If len > 32, only the first 32 bytes are used.
/// @param data Source data
/// @param len Number of bytes to copy
/// @return bytes32 containing the padded data
bytes32_t bytes32_from_bytes_padded(const uint8_t *data, size_t len);

/// Converts a bytes32 to a uint256.
/// bytes32 is big-endian, uint256 uses little-endian limbs.
/// @param b Source bytes32
/// @return uint256 representation
uint256_t bytes32_to_uint256(const bytes32_t *b);

/// Creates a bytes32 from a uint256.
/// uint256 uses little-endian limbs, bytes32 is big-endian.
/// @param value Source uint256
/// @return bytes32 representation
bytes32_t bytes32_from_uint256(const uint256_t *value);

#endif // DIV0_TYPES_BYTES32_H