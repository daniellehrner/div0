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

/// Size of uint256 in bytes.
static constexpr size_t UINT256_SIZE_BYTES = 32;

static_assert(sizeof(uint256_t) == UINT256_SIZE_BYTES, "uint256_t must be 32 bytes");

/// Returns a zero-initialized uint256.
static inline uint256_t uint256_zero(void) {
  return (uint256_t){{0, 0, 0, 0}};
}

/// Creates a uint256 from a 64-bit value.
static inline uint256_t uint256_from_u64(uint64_t value) {
  return (uint256_t){{value, 0, 0, 0}};
}

/// Creates a uint256 from four 64-bit limbs (little-endian: limb0 = LSB).
static inline uint256_t uint256_from_limbs(uint64_t limb0, uint64_t limb1, uint64_t limb2,
                                           uint64_t limb3) {
  return (uint256_t){{limb0, limb1, limb2, limb3}};
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

/// Subtracts two uint256 values. Wraps on underflow.
static inline uint256_t uint256_sub(uint256_t a, uint256_t b) {
  uint256_t result;
  unsigned long long borrow = 0;

  result.limbs[0] = __builtin_subcll(a.limbs[0], b.limbs[0], borrow, &borrow);
  result.limbs[1] = __builtin_subcll(a.limbs[1], b.limbs[1], borrow, &borrow);
  result.limbs[2] = __builtin_subcll(a.limbs[2], b.limbs[2], borrow, &borrow);
  result.limbs[3] = __builtin_subcll(a.limbs[3], b.limbs[3], borrow, &borrow);

  return result;
}

/// Compares two uint256 values. Returns true if a < b.
static inline bool uint256_lt(uint256_t a, uint256_t b) {
  if (a.limbs[3] != b.limbs[3]) {
    return a.limbs[3] < b.limbs[3];
  }
  if (a.limbs[2] != b.limbs[2]) {
    return a.limbs[2] < b.limbs[2];
  }
  if (a.limbs[1] != b.limbs[1]) {
    return a.limbs[1] < b.limbs[1];
  }
  return a.limbs[0] < b.limbs[0];
}

/// Multiplies two uint256 values. Wraps on overflow (mod 2^256).
uint256_t uint256_mul(uint256_t a, uint256_t b);

/// Divides two uint256 values. Returns 0 if divisor is zero (EVM semantics).
uint256_t uint256_div(uint256_t a, uint256_t b);

/// Computes modulo of two uint256 values. Returns 0 if divisor is zero (EVM semantics).
uint256_t uint256_mod(uint256_t a, uint256_t b);

/// Creates a uint256 from big-endian bytes.
/// @param data Pointer to big-endian byte array
/// @param len Number of bytes (0-32). If less than 32, value is zero-padded on the left.
uint256_t uint256_from_bytes_be(const uint8_t *data, size_t len);

/// Exports a uint256 to a 32-byte big-endian array.
/// @param value The uint256 to export
/// @param out Output buffer (must be at least 32 bytes)
void uint256_to_bytes_be(uint256_t value, uint8_t *out);

/// Parse a uint256 from a hex string.
/// Accepts optional "0x" prefix. Requires exactly 64 hex characters.
/// @param hex Hex string (with or without 0x prefix)
/// @param out Output uint256
/// @return true if parsing succeeded, false on invalid input
bool uint256_from_hex(const char *hex, uint256_t *out);

#endif // DIV0_TYPES_UINT256_H
