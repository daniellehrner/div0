// Unit tests for Uint256 signed arithmetic and bit manipulation
// Covers SDIV, SMOD, SIGNEXTEND operations per EVM specification

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// Common constants for signed arithmetic
const Uint256 ZERO = Uint256(0);
const Uint256 ONE = Uint256(1);
const Uint256 MINUS_ONE = Uint256(MAX64, MAX64, MAX64, MAX64);
const Uint256 MIN_NEGATIVE = Uint256(0, 0, 0, 0x8000000000000000ULL);  // -2^255

// =============================================================================
// is_negative() Tests
// =============================================================================

TEST(Uint256Signed, IsNegativeZero) { EXPECT_FALSE(ZERO.is_negative()); }

TEST(Uint256Signed, IsNegativePositive) {
  EXPECT_FALSE(ONE.is_negative());
  EXPECT_FALSE(Uint256(MAX64).is_negative());
  EXPECT_FALSE(Uint256(MAX64, MAX64, MAX64, MAX64 >> 1).is_negative());  // Max positive
}

TEST(Uint256Signed, IsNegativeNegative) {
  EXPECT_TRUE(MINUS_ONE.is_negative());
  EXPECT_TRUE(MIN_NEGATIVE.is_negative());
  EXPECT_TRUE(Uint256(0, 0, 0, 0x8000000000000000ULL).is_negative());
}

// =============================================================================
// negate() Tests
// =============================================================================

TEST(Uint256Signed, NegateZero) { EXPECT_EQ(ZERO.negate(), ZERO); }

TEST(Uint256Signed, NegateOne) { EXPECT_EQ(ONE.negate(), MINUS_ONE); }

TEST(Uint256Signed, NegateMinusOne) { EXPECT_EQ(MINUS_ONE.negate(), ONE); }

TEST(Uint256Signed, NegateSymmetry) {
  EXPECT_EQ(Uint256(42).negate().negate(), Uint256(42));
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0).negate().negate(), Uint256(MAX64, MAX64, 0, 0));
}

TEST(Uint256Signed, NegateMinValue) {
  // -MIN_VALUE = MIN_VALUE (overflow, but that's two's complement behavior)
  EXPECT_EQ(MIN_NEGATIVE.negate(), MIN_NEGATIVE);
}

// =============================================================================
// SDIV: Signed Division
// =============================================================================

TEST(Uint256SDIV, DivisionByZero) {
  // EVM spec: SDIV by zero returns 0
  EXPECT_EQ(Uint256::sdiv(ZERO, ZERO), ZERO);
  EXPECT_EQ(Uint256::sdiv(ONE, ZERO), ZERO);
  EXPECT_EQ(Uint256::sdiv(MINUS_ONE, ZERO), ZERO);
  EXPECT_EQ(Uint256::sdiv(MIN_NEGATIVE, ZERO), ZERO);
}

TEST(Uint256SDIV, PositiveByPositive) {
  EXPECT_EQ(Uint256::sdiv(Uint256(10), Uint256(3)), Uint256(3));
  EXPECT_EQ(Uint256::sdiv(Uint256(100), Uint256(10)), Uint256(10));
  EXPECT_EQ(Uint256::sdiv(Uint256(MAX64), Uint256(2)), Uint256(MAX64 / 2));
}

TEST(Uint256SDIV, NegativeByPositive) {
  // -10 / 3 = -3 (truncated toward zero)
  const Uint256 neg_10 = Uint256(10).negate();
  const Uint256 neg_3 = Uint256(3).negate();
  EXPECT_EQ(Uint256::sdiv(neg_10, Uint256(3)), neg_3);
}

TEST(Uint256SDIV, PositiveByNegative) {
  // 10 / -3 = -3 (truncated toward zero)
  const Uint256 neg_3 = Uint256(3).negate();
  EXPECT_EQ(Uint256::sdiv(Uint256(10), neg_3), neg_3);
}

TEST(Uint256SDIV, NegativeByNegative) {
  // -10 / -3 = 3 (positive result)
  const Uint256 neg_10 = Uint256(10).negate();
  const Uint256 neg_3 = Uint256(3).negate();
  EXPECT_EQ(Uint256::sdiv(neg_10, neg_3), Uint256(3));
}

TEST(Uint256SDIV, MinValueDividedByMinusOne) {
  // EVM special case: MIN_VALUE / -1 = MIN_VALUE (overflow)
  EXPECT_EQ(Uint256::sdiv(MIN_NEGATIVE, MINUS_ONE), MIN_NEGATIVE);
}

TEST(Uint256SDIV, MinValueDividedByOne) {
  EXPECT_EQ(Uint256::sdiv(MIN_NEGATIVE, ONE), MIN_NEGATIVE);
}

TEST(Uint256SDIV, MinusOneDividedByAnything) {
  EXPECT_EQ(Uint256::sdiv(MINUS_ONE, ONE), MINUS_ONE);
  EXPECT_EQ(Uint256::sdiv(MINUS_ONE, MINUS_ONE), ONE);
  EXPECT_EQ(Uint256::sdiv(MINUS_ONE, Uint256(2)), ZERO);
}

// =============================================================================
// SMOD: Signed Modulo
// =============================================================================

TEST(Uint256SMOD, ModuloByZero) {
  // EVM spec: SMOD by zero returns 0
  EXPECT_EQ(Uint256::smod(ZERO, ZERO), ZERO);
  EXPECT_EQ(Uint256::smod(ONE, ZERO), ZERO);
  EXPECT_EQ(Uint256::smod(MINUS_ONE, ZERO), ZERO);
}

TEST(Uint256SMOD, PositiveByPositive) {
  EXPECT_EQ(Uint256::smod(Uint256(10), Uint256(3)), Uint256(1));
  EXPECT_EQ(Uint256::smod(Uint256(100), Uint256(10)), ZERO);
}

TEST(Uint256SMOD, NegativeByPositive) {
  // -10 % 3 = -1 (sign follows dividend)
  const Uint256 neg_10 = Uint256(10).negate();
  const Uint256 neg_1 = Uint256(1).negate();
  EXPECT_EQ(Uint256::smod(neg_10, Uint256(3)), neg_1);
}

TEST(Uint256SMOD, PositiveByNegative) {
  // 10 % -3 = 1 (sign follows dividend, which is positive)
  const Uint256 neg_3 = Uint256(3).negate();
  EXPECT_EQ(Uint256::smod(Uint256(10), neg_3), Uint256(1));
}

TEST(Uint256SMOD, NegativeByNegative) {
  // -10 % -3 = -1 (sign follows dividend)
  const Uint256 neg_10 = Uint256(10).negate();
  const Uint256 neg_3 = Uint256(3).negate();
  const Uint256 neg_1 = Uint256(1).negate();
  EXPECT_EQ(Uint256::smod(neg_10, neg_3), neg_1);
}

TEST(Uint256SMOD, ExactDivision) {
  EXPECT_EQ(Uint256::smod(Uint256(12), Uint256(4)), ZERO);
  EXPECT_EQ(Uint256::smod(Uint256(12).negate(), Uint256(4)), ZERO);
}

// =============================================================================
// SIGNEXTEND: Sign Extension
// =============================================================================

TEST(Uint256SignExtend, BytePosZeroPositive) {
  // byte_pos = 0 means extend from bit 7
  // 0x7F has bit 7 = 0 (positive), should remain unchanged in low byte
  EXPECT_EQ(Uint256::signextend(ZERO, Uint256(0x7F)), Uint256(0x7F));
}

TEST(Uint256SignExtend, BytePosZeroNegative) {
  // 0x80 has bit 7 = 1 (negative), should extend 1s
  const Uint256 result = Uint256::signextend(ZERO, Uint256(0x80));
  EXPECT_EQ(result, Uint256(MAX64, MAX64, MAX64, MAX64) - Uint256(0x7F));
}

TEST(Uint256SignExtend, BytePosOnePositive) {
  // byte_pos = 1 means extend from bit 15
  // 0x7FFF has bit 15 = 0 (positive)
  EXPECT_EQ(Uint256::signextend(Uint256(1), Uint256(0x7FFF)), Uint256(0x7FFF));
}

TEST(Uint256SignExtend, BytePosOneNegative) {
  // 0x8000 has bit 15 = 1 (negative), should extend 1s
  const Uint256 result = Uint256::signextend(Uint256(1), Uint256(0x8000));
  // Result should be 0xFFFF...FFFF8000
  EXPECT_TRUE(result.is_negative());
  EXPECT_EQ(result & Uint256(0xFFFF), Uint256(0x8000));
}

TEST(Uint256SignExtend, BytePos30) {
  // byte_pos = 30 means extend from bit 8*30+7 = 247
  // bit 247 is in limb 3 (247/64 = 3), at position 247%64 = 55

  // Value with bit 247 = 0 (positive in 248-bit signed)
  const Uint256 positive = Uint256(MAX64, MAX64, MAX64, 0x007FFFFFFFFFFFFFULL);

  // Value with bit 247 = 1 (negative in 248-bit signed)
  const Uint256 negative = Uint256(0, 0, 0, 0x0080000000000000ULL);  // 1 << 55

  // Positive value should stay positive after sign extension
  EXPECT_FALSE(Uint256::signextend(Uint256(30), positive).is_negative());

  // Negative value (bit 247 set) should extend 1s to fill upper bits
  EXPECT_TRUE(Uint256::signextend(Uint256(30), negative).is_negative());
}

TEST(Uint256SignExtend, BytePos31OrLarger) {
  // byte_pos >= 31 means all 256 bits are used, no extension needed
  const Uint256 val = Uint256(0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222);

  EXPECT_EQ(Uint256::signextend(Uint256(31), val), val);
  EXPECT_EQ(Uint256::signextend(Uint256(100), val), val);
  EXPECT_EQ(Uint256::signextend(Uint256(MAX64), val), val);
}

TEST(Uint256SignExtend, BytePosLargeInHighLimbs) {
  // byte_pos with high limbs set should return unchanged
  const Uint256 val = Uint256(0x12345678);
  EXPECT_EQ(Uint256::signextend(Uint256(0, 1, 0, 0), val), val);
  EXPECT_EQ(Uint256::signextend(Uint256(0, 0, 0, 1), val), val);
}

TEST(Uint256SignExtend, AllBytePositions) {
  // Test each byte position from 0 to 30
  for (uint64_t pos = 0; pos <= 30; ++pos) {
    // Create a value with the sign bit at this position set
    const auto bit_index = static_cast<unsigned>((8 * pos) + 7);
    const unsigned limb_index = bit_index / 64;
    const unsigned bit_in_limb = bit_index % 64;

    // Build Uint256 with the appropriate bit set in the correct limb
    const uint64_t limb_val = 1ULL << bit_in_limb;
    Uint256 val;
    switch (limb_index) {
      case 0:
        val = Uint256(limb_val, 0, 0, 0);
        break;
      case 1:
        val = Uint256(0, limb_val, 0, 0);
        break;
      case 2:
        val = Uint256(0, 0, limb_val, 0);
        break;
      case 3:
        val = Uint256(0, 0, 0, limb_val);
        break;
      default:
        FAIL() << "Unexpected limb_index: " << limb_index;
    }

    const Uint256 result = Uint256::signextend(Uint256(pos), val);
    EXPECT_TRUE(result.is_negative()) << "Failed at byte_pos=" << pos;
  }
}

TEST(Uint256SignExtend, ClearHighBits) {
  // When sign bit is 0, high bits should be cleared
  const Uint256 val = Uint256(0x0000007F, MAX64, MAX64, MAX64);  // High bits polluted
  const Uint256 result = Uint256::signextend(ZERO, val);
  EXPECT_EQ(result, Uint256(0x7F));
}

// =============================================================================
// SDIV/SMOD Division Identity
// =============================================================================

TEST(Uint256SignedIdentity, SDivSModIdentity) {
  // For signed: a = (a SDIV b) * b + (a SMOD b)
  // But with EVM semantics (truncation toward zero)

  const std::vector<std::pair<int64_t, int64_t>> cases = {
      {10, 3}, {10, -3}, {-10, 3}, {-10, -3}, {100, 7}, {-100, 7}, {100, -7}, {-100, -7},
  };

  for (const auto& [a_val, b_val] : cases) {
    const Uint256 a = a_val >= 0 ? Uint256(static_cast<uint64_t>(a_val))
                                 : Uint256(static_cast<uint64_t>(-a_val)).negate();
    const Uint256 b = b_val >= 0 ? Uint256(static_cast<uint64_t>(b_val))
                                 : Uint256(static_cast<uint64_t>(-b_val)).negate();

    const Uint256 q = Uint256::sdiv(a, b);
    const Uint256 r = Uint256::smod(a, b);

    // Verify: a = q * b + r
    EXPECT_EQ(q * b + r, a) << "Failed for a=" << a_val << ", b=" << b_val;
  }
}

}  // namespace div0::types
