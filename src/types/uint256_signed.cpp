#include <div0/types/uint256.h>

namespace div0::types {

// MIN_VALUE = -2^255 = 0x8000...0000 (only high bit set)
// This is the most negative value in two's complement representation.
// It has no positive equivalent since 2^255 > max positive value (2^255 - 1).
static constexpr Uint256 MIN_NEGATIVE_VALUE = Uint256(0, 0, 0, 0x8000000000000000ULL);

// -1 in two's complement is all 1s
static constexpr Uint256 MINUS_ONE = Uint256(~0ULL, ~0ULL, ~0ULL, ~0ULL);

Uint256 Uint256::sdiv(const Uint256& a, const Uint256& b) noexcept {
  // Division by zero returns 0 (EVM semantics)
  if (b.is_zero()) [[unlikely]] {
    return {0};
  }

  // Special case: MIN_VALUE / -1 would overflow (result > MAX_POSITIVE)
  // EVM returns MIN_VALUE in this case
  if (a == MIN_NEGATIVE_VALUE && b == MINUS_ONE) [[unlikely]] {
    return MIN_NEGATIVE_VALUE;
  }

  // Get signs from MSB
  const bool a_neg = a.is_negative();
  const bool b_neg = b.is_negative();

  // Convert to absolute values
  const Uint256 abs_a = a_neg ? a.negate() : a;
  const Uint256 abs_b = b_neg ? b.negate() : b;

  // Fast path: both operands fit in 64 bits - use native hardware division
  if ((abs_a.data_[1] | abs_a.data_[2] | abs_a.data_[3]) == 0 &&
      (abs_b.data_[1] | abs_b.data_[2] | abs_b.data_[3]) == 0) [[likely]] {
    const uint64_t q = abs_a.data_[0] / abs_b.data_[0];
    // Apply sign: negative if exactly one operand was negative
    if (a_neg != b_neg) {
      return Uint256(q).negate();
    }
    return {q};
  }

  // Perform unsigned division for multi-limb case
  const Uint256 quotient = abs_a / abs_b;

  // Apply sign: negative if exactly one operand was negative
  return (a_neg != b_neg) ? quotient.negate() : quotient;
}

Uint256 Uint256::smod(const Uint256& a, const Uint256& b) noexcept {
  // Modulo by zero returns 0 (EVM semantics)
  if (b.is_zero()) [[unlikely]] {
    return {0};
  }

  // Get signs
  const bool a_neg = a.is_negative();
  const bool b_neg = b.is_negative();

  // Convert to absolute values
  const Uint256 abs_a = a_neg ? a.negate() : a;
  const Uint256 abs_b = b_neg ? b.negate() : b;

  // Fast path: both operands fit in 64 bits - use native hardware modulo
  if ((abs_a.data_[1] | abs_a.data_[2] | abs_a.data_[3]) == 0 &&
      (abs_b.data_[1] | abs_b.data_[2] | abs_b.data_[3]) == 0) [[likely]] {
    const uint64_t r = abs_a.data_[0] % abs_b.data_[0];
    // Result sign follows dividend sign (EVM semantics)
    if (a_neg) {
      return Uint256(r).negate();
    }
    return {r};
  }

  // Perform unsigned modulo for multi-limb case
  const Uint256 remainder = abs_a % abs_b;

  // Result sign follows dividend sign (EVM semantics)
  return a_neg ? remainder.negate() : remainder;
}

Uint256 Uint256::signextend(const Uint256& byte_pos, const Uint256& x) noexcept {
  // If byte_pos >= 31 (or any upper limbs set), the value already uses all 256 bits
  // No sign extension needed
  if (byte_pos.data_[1] != 0 || byte_pos.data_[2] != 0 || byte_pos.data_[3] != 0 ||
      byte_pos.data_[0] >= 31) {
    return x;
  }

  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

  // byte_pos is 0-30, indicating which byte's MSB is the sign bit
  // Byte 0 = bits 0-7, Byte 1 = bits 8-15, etc.
  // Sign bit is at position: (8 * byte_pos) + 7
  const auto pos = byte_pos.data_[0];
  const auto bit_index = static_cast<unsigned>((8 * pos) + 7);

  // Determine which limb contains the sign bit
  const auto limb_index = bit_index / 64;
  const auto bit_in_limb = bit_index % 64;

  // Check if the sign bit is set
  const bool sign_bit = ((x.data_[limb_index] >> bit_in_limb) & 1) != 0;

  if (!sign_bit) {
    // Positive: clear all bits above the sign bit position
    // Create a mask with 1s from bit 0 to bit_index (inclusive)
    Uint256 result = x;

    // Clear bits above sign bit in the containing limb
    // Handle bit_in_limb == 63 specially to avoid shift overflow
    const uint64_t limb_mask = (bit_in_limb == 63) ? ~0ULL : (1ULL << (bit_in_limb + 1)) - 1;
    result.data_[limb_index] &= limb_mask;

    // Clear all higher limbs
    for (auto i = limb_index + 1; i < 4; ++i) {
      result.data_[i] = 0;
    }

    return result;
  }

  // Negative: set all bits above the sign bit position to 1
  Uint256 result = x;

  // Set bits above sign bit in the containing limb to 1
  // Handle bit_in_limb == 63 specially to avoid shift overflow
  const uint64_t limb_mask = (bit_in_limb == 63) ? 0ULL : ~((1ULL << (bit_in_limb + 1)) - 1);
  result.data_[limb_index] |= limb_mask;

  // Set all higher limbs to all 1s
  for (auto i = limb_index + 1; i < 4; ++i) {
    result.data_[i] = ~0ULL;
  }

  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

  return result;
}

}  // namespace div0::types
