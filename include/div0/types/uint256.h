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

/// Checks if a uint256 fits in a 64-bit value.
/// Returns true iff the upper 192 bits are zero, meaning the value can be
/// losslessly represented as a uint64_t using uint256_to_u64_unsafe().
static inline bool uint256_fits_u64(uint256_t a) {
  return (a.limbs[1] | a.limbs[2] | a.limbs[3]) == 0;
}

/// Returns the low 64 bits. Caller must ensure uint256_fits_u64() is true.
static inline uint64_t uint256_to_u64_unsafe(uint256_t a) {
  return a.limbs[0];
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

// =============================================================================
// Additional comparison operations
// =============================================================================

/// Compares two uint256 values. Returns true if a > b.
static inline bool uint256_gt(uint256_t a, uint256_t b) {
  return uint256_lt(b, a);
}

/// Checks if uint256 is negative (in two's complement representation).
/// The value is negative if the most significant bit is set.
static inline bool uint256_is_negative(uint256_t a) {
  return (a.limbs[3] >> 63) != 0;
}

// =============================================================================
// Bitwise operations
// =============================================================================

/// Returns the bitwise AND of two uint256 values.
static inline uint256_t uint256_and(uint256_t a, uint256_t b) {
  return (uint256_t){{
      a.limbs[0] & b.limbs[0],
      a.limbs[1] & b.limbs[1],
      a.limbs[2] & b.limbs[2],
      a.limbs[3] & b.limbs[3],
  }};
}

/// Returns the bitwise OR of two uint256 values.
static inline uint256_t uint256_or(uint256_t a, uint256_t b) {
  return (uint256_t){{
      a.limbs[0] | b.limbs[0],
      a.limbs[1] | b.limbs[1],
      a.limbs[2] | b.limbs[2],
      a.limbs[3] | b.limbs[3],
  }};
}

/// Returns the bitwise XOR of two uint256 values.
static inline uint256_t uint256_xor(uint256_t a, uint256_t b) {
  return (uint256_t){{
      a.limbs[0] ^ b.limbs[0],
      a.limbs[1] ^ b.limbs[1],
      a.limbs[2] ^ b.limbs[2],
      a.limbs[3] ^ b.limbs[3],
  }};
}

/// Returns the bitwise NOT of a uint256 value.
static inline uint256_t uint256_not(uint256_t a) {
  return (uint256_t){{
      ~a.limbs[0],
      ~a.limbs[1],
      ~a.limbs[2],
      ~a.limbs[3],
  }};
}

/// Extracts a single byte from a uint256 value (EVM BYTE opcode).
/// Index 0 is the most significant byte, index 31 is the least significant.
/// Returns 0 if index >= 32.
static inline uint256_t uint256_byte(uint256_t index, uint256_t value) {
  // If index doesn't fit in u64 or is >= 32, return 0
  if (!uint256_fits_u64(index) || index.limbs[0] >= 32) {
    return uint256_zero();
  }

  const uint64_t idx = index.limbs[0];
  // Big-endian index 0 = byte 31 in little-endian (MSB)
  // Big-endian index 31 = byte 0 in little-endian (LSB)
  const uint64_t le_byte_idx = 31 - idx;
  const uint64_t limb_idx = le_byte_idx / 8;
  const uint64_t byte_in_limb = le_byte_idx % 8;

  const uint8_t byte_val = (uint8_t)((value.limbs[limb_idx] >> (byte_in_limb * 8)) & 0xFF);
  return uint256_from_u64(byte_val);
}

/// Shift left. Returns 0 if shift >= 256.
uint256_t uint256_shl(uint256_t shift, uint256_t value);

/// Logical shift right (zero-fill). Returns 0 if shift >= 256.
uint256_t uint256_shr(uint256_t shift, uint256_t value);

/// Arithmetic shift right (sign-extending). If shift >= 256, returns 0 or -1 depending on sign.
uint256_t uint256_sar(uint256_t shift, uint256_t value);

// =============================================================================
// Signed comparison operations
// =============================================================================

/// Signed less-than comparison.
/// Returns true if a < b when interpreted as two's complement signed integers.
static inline bool uint256_slt(uint256_t a, uint256_t b) {
  const bool a_neg = uint256_is_negative(a);
  const bool b_neg = uint256_is_negative(b);

  if (a_neg != b_neg) {
    // Different signs: negative < non-negative
    return a_neg;
  }
  // Same sign: unsigned comparison works
  return uint256_lt(a, b);
}

/// Signed greater-than comparison.
/// Returns true if a > b when interpreted as two's complement signed integers.
static inline bool uint256_sgt(uint256_t a, uint256_t b) {
  return uint256_slt(b, a);
}

// =============================================================================
// Signed arithmetic operations (Phase 1)
// =============================================================================

/// Signed division. Returns 0 if divisor is zero (EVM semantics).
/// Special case: MIN_VALUE / -1 returns MIN_VALUE (overflow protection).
uint256_t uint256_sdiv(uint256_t a, uint256_t b);

/// Signed modulo. Returns 0 if divisor is zero (EVM semantics).
/// Result sign follows dividend (not divisor) per EVM semantics.
uint256_t uint256_smod(uint256_t a, uint256_t b);

/// Sign extends value x at byte position byte_pos.
/// If byte_pos >= 31, returns x unchanged.
uint256_t uint256_signextend(uint256_t byte_pos, uint256_t x);

// =============================================================================
// Modular arithmetic operations (Phase 1)
// =============================================================================

/// Computes (a + b) mod n. Returns 0 if n is zero (EVM semantics).
uint256_t uint256_addmod(uint256_t a, uint256_t b, uint256_t n);

/// Computes (a * b) mod n. Returns 0 if n is zero (EVM semantics).
uint256_t uint256_mulmod(uint256_t a, uint256_t b, uint256_t n);

// =============================================================================
// Exponentiation (Phase 1)
// =============================================================================

/// Computes base^exponent mod 2^256 using binary exponentiation.
uint256_t uint256_exp(uint256_t base, uint256_t exponent);

/// Returns the number of bytes needed to represent the value.
/// Used for EXP gas calculation.
size_t uint256_byte_length(uint256_t value);

#endif // DIV0_TYPES_UINT256_H
