#ifndef DIV0_TYPES_UINT256_H
#define DIV0_TYPES_UINT256_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// 256-bit unsigned integer.
/// Internal storage is little-endian: limbs[0] = least significant 64 bits.
/// EVM I/O uses big-endian; use conversion functions for that.
typedef struct {
  uint64_t limbs[4];
} uint256_t;

/// Returns a zero-initialized uint256.
static inline uint256_t uint256_zero(void) {
  return (uint256_t){{0, 0, 0, 0}};
}

/// Creates a uint256 from a 64-bit value.
static inline uint256_t uint256_from_u64(uint64_t value) {
  return (uint256_t){{value, 0, 0, 0}};
}

/// Checks if a uint256 is zero.
static inline bool uint256_is_zero(uint256_t a) {
  return (a.limbs[0] | a.limbs[1] | a.limbs[2] | a.limbs[3]) == 0;
}

/// Checks equality of two uint256 values.
static inline bool uint256_eq(uint256_t a, uint256_t b) {
  // Use XOR: if all limbs are equal, XOR produces 0
  uint64_t diff = (a.limbs[0] ^ b.limbs[0]) | (a.limbs[1] ^ b.limbs[1]) |
                  (a.limbs[2] ^ b.limbs[2]) | (a.limbs[3] ^ b.limbs[3]);
  return diff == 0;
}

/// Adds two uint256 values. Wraps on overflow.
static inline uint256_t uint256_add(uint256_t a, uint256_t b) {
  uint256_t result;
  unsigned long long carry = 0;

  result.limbs[0] = __builtin_addcll(a.limbs[0], b.limbs[0], carry, &carry);
  result.limbs[1] = __builtin_addcll(a.limbs[1], b.limbs[1], carry, &carry);
  result.limbs[2] = __builtin_addcll(a.limbs[2], b.limbs[2], carry, &carry);
  result.limbs[3] = __builtin_addcll(a.limbs[3], b.limbs[3], carry, &carry);

  return result;
}

/// Creates a uint256 from big-endian bytes.
/// @param data Pointer to big-endian byte array
/// @param len Number of bytes (0-32). If less than 32, value is zero-padded on the left.
uint256_t uint256_from_bytes_be(const uint8_t *data, size_t len);

/// Exports a uint256 to a 32-byte big-endian array.
/// @param value The uint256 to export
/// @param out Output buffer (must be at least 32 bytes)
void uint256_to_bytes_be(uint256_t value, uint8_t *out);

#endif // DIV0_TYPES_UINT256_H
