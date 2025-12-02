// Unit tests for Uint256 modular arithmetic and exponentiation
// Covers ADDMOD, MULMOD, EXP operations per EVM specification

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// Common constants
const Uint256 ZERO = Uint256(0);
const Uint256 ONE = Uint256(1);
const Uint256 MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);

// =============================================================================
// ADDMOD: (a + b) mod N
// =============================================================================

TEST(Uint256AddMod, ModuloByZero) {
  // EVM spec: ADDMOD with N=0 returns 0
  EXPECT_EQ(Uint256::addmod(ZERO, ZERO, ZERO), ZERO);
  EXPECT_EQ(Uint256::addmod(ONE, ONE, ZERO), ZERO);
  EXPECT_EQ(Uint256::addmod(MAX256, MAX256, ZERO), ZERO);
}

TEST(Uint256AddMod, NoOverflow) {
  // Simple cases without overflow
  EXPECT_EQ(Uint256::addmod(Uint256(5), Uint256(3), Uint256(10)), Uint256(8));
  EXPECT_EQ(Uint256::addmod(Uint256(7), Uint256(8), Uint256(10)), Uint256(5));
  EXPECT_EQ(Uint256::addmod(Uint256(100), Uint256(200), Uint256(1000)), Uint256(300));
}

TEST(Uint256AddMod, ResultEqualsModulus) {
  // a + b = N exactly
  EXPECT_EQ(Uint256::addmod(Uint256(5), Uint256(5), Uint256(10)), ZERO);
}

TEST(Uint256AddMod, SumLessThanModulus) {
  // a + b < N
  EXPECT_EQ(Uint256::addmod(Uint256(3), Uint256(4), Uint256(100)), Uint256(7));
}

TEST(Uint256AddMod, Overflow257Bits) {
  // Key test: sum overflows 256 bits
  // MAX256 + MAX256 = 2^257 - 2
  // (2^257 - 2) mod (2^256 - 1) = (2^257 - 2) - (2^256 - 1) = 2^256 - 1 = MAX256
  // But wait, that's still >= modulus, so we subtract again
  // Actually: MAX256 + MAX256 = 2 * MAX256 = 2 * (2^256 - 1) = 2^257 - 2
  // 2^257 - 2 mod MAX256 = (2^257 - 2) mod (2^256 - 1)
  // = 2^256 + 2^256 - 2 mod (2^256 - 1)
  // = (2^256 - 1 + 1) + (2^256 - 1 + 1) - 2 mod (2^256 - 1)
  // = 2 * (2^256 - 1) + 2 - 2 mod (2^256 - 1) = 0

  // Let's verify with a simpler overflow case:
  // MAX256 + 1 mod MAX256 = 0 (wraps to 2^256 which is 1 mod MAX256)
  EXPECT_EQ(Uint256::addmod(MAX256, ONE, MAX256), ONE);

  // MAX256 + MAX256 mod MAX256 should be MAX256 - 1 (since 2*MAX256 = MAX256 + MAX256 - 1 (mod
  // MAX256)) Actually: 2 * (2^256 - 1) mod (2^256 - 1) = 0... but with 257-bit intermediate Wait:
  // (MAX256 + MAX256) mod MAX256 = ((2^256-1) + (2^256-1)) mod (2^256-1) = (2^257 - 2) mod (2^256 -
  // 1) = 2^256 - 1 + (2^256 - 1) - (2^256 - 1) + ??? this is getting complex Let's just verify the
  // implementation works
  const Uint256 result = Uint256::addmod(MAX256, MAX256, MAX256);
  EXPECT_EQ(result, ZERO);  // 2 * MAX256 mod MAX256 = 0
}

TEST(Uint256AddMod, OverflowWithSmallModulus) {
  // MAX256 + MAX256 mod 7 with full 257-bit precision
  // This tests the 257-bit overflow handling
  const Uint256 result = Uint256::addmod(MAX256, MAX256, Uint256(7));
  // MAX256 mod 7: need to compute (2^256 - 1) mod 7
  // 2^3 = 8 ≡ 1 mod 7, so 2^255 = 2^(3*85) = 1 mod 7
  // 2^256 = 2 mod 7, so 2^256 - 1 = 1 mod 7
  // MAX256 + MAX256 = 2 * (2^256 - 1) = 2^257 - 2
  // 2^257 = 2 * 2^256 = 2 * 2 = 4 mod 7
  // 2^257 - 2 = 4 - 2 = 2 mod 7
  EXPECT_EQ(result, Uint256(2));
}

TEST(Uint256AddMod, ZeroOperands) {
  EXPECT_EQ(Uint256::addmod(ZERO, ZERO, Uint256(10)), ZERO);
  EXPECT_EQ(Uint256::addmod(ZERO, Uint256(5), Uint256(10)), Uint256(5));
  EXPECT_EQ(Uint256::addmod(Uint256(5), ZERO, Uint256(10)), Uint256(5));
}

TEST(Uint256AddMod, ModulusOne) {
  // Any sum mod 1 = 0
  EXPECT_EQ(Uint256::addmod(Uint256(12345), Uint256(67890), ONE), ZERO);
  // Note: MAX256 + MAX256 mod 1 = (2^257 - 2) mod 1 = 0
  // This requires full 257-bit handling
  EXPECT_EQ(Uint256::addmod(MAX256, MAX256, ONE), ZERO);
}

// =============================================================================
// MULMOD: (a * b) mod N
// =============================================================================

TEST(Uint256MulMod, ModuloByZero) {
  // EVM spec: MULMOD with N=0 returns 0
  EXPECT_EQ(Uint256::mulmod(ZERO, ZERO, ZERO), ZERO);
  EXPECT_EQ(Uint256::mulmod(ONE, ONE, ZERO), ZERO);
  EXPECT_EQ(Uint256::mulmod(MAX256, MAX256, ZERO), ZERO);
}

TEST(Uint256MulMod, ZeroMultiplier) {
  EXPECT_EQ(Uint256::mulmod(ZERO, Uint256(12345), Uint256(100)), ZERO);
  EXPECT_EQ(Uint256::mulmod(Uint256(12345), ZERO, Uint256(100)), ZERO);
}

TEST(Uint256MulMod, NoOverflow) {
  // Simple cases where product fits in 256 bits
  EXPECT_EQ(Uint256::mulmod(Uint256(5), Uint256(3), Uint256(10)), Uint256(5));
  EXPECT_EQ(Uint256::mulmod(Uint256(7), Uint256(8), Uint256(10)), Uint256(6));
  EXPECT_EQ(Uint256::mulmod(Uint256(100), Uint256(200), Uint256(7)), Uint256(20000 % 7));
}

TEST(Uint256MulMod, ProductEqualsModulus) {
  EXPECT_EQ(Uint256::mulmod(Uint256(5), Uint256(2), Uint256(10)), ZERO);
}

TEST(Uint256MulMod, Overflow512Bits) {
  // MAX256 * MAX256 requires full 512-bit product
  // (2^256 - 1)^2 = 2^512 - 2^257 + 1
  // This is the key stress test for the 512-bit multiplication and division

  // MAX256 * MAX256 mod MAX256 = (MAX256^2) mod MAX256
  // = ((MAX256 mod MAX256) * (MAX256 mod MAX256)) mod MAX256 = 0 * 0 = 0
  // Wait, MAX256 mod MAX256 = 0, so the product is 0
  EXPECT_EQ(Uint256::mulmod(MAX256, MAX256, MAX256), ZERO);

  // MAX256 * 2 mod (MAX256 - 1)
  // = (2 * (2^256 - 1)) mod (2^256 - 2)
  // = (2^257 - 2) mod (2^256 - 2)
  // = (2^256 - 2 + 2^256) mod (2^256 - 2)
  // = 2^256 mod (2^256 - 2) = 2
  const Uint256 max_minus_1 = MAX256 - ONE;
  EXPECT_EQ(Uint256::mulmod(MAX256, Uint256(2), max_minus_1), Uint256(2));
}

TEST(Uint256MulMod, OverflowWithSmallModulus) {
  // MAX256 * MAX256 mod 7
  const Uint256 max_mod_7 = MAX256 % Uint256(7);
  const Uint256 expected = (max_mod_7 * max_mod_7) % Uint256(7);
  EXPECT_EQ(Uint256::mulmod(MAX256, MAX256, Uint256(7)), expected);
}

TEST(Uint256MulMod, ModulusOne) {
  // Any product mod 1 = 0
  EXPECT_EQ(Uint256::mulmod(Uint256(12345), Uint256(67890), ONE), ZERO);
  EXPECT_EQ(Uint256::mulmod(MAX256, MAX256, ONE), ZERO);
}

TEST(Uint256MulMod, LargeMultiplication) {
  // 2^128 * 2^128 = 2^256 (overflows to 0 in 256 bits, but MULMOD uses 512)
  const Uint256 pow128 = Uint256(0, 0, 1, 0);   // 2^128
  const Uint256 modulus = Uint256(0, 0, 0, 1);  // 2^192

  // 2^256 mod 2^192 = 0 (since 2^256 = 2^64 * 2^192)
  EXPECT_EQ(Uint256::mulmod(pow128, pow128, modulus), ZERO);
}

TEST(Uint256MulMod, ProductFitsIn256Bits) {
  // Product just under 256 bits - should use fast path
  const Uint256 a = Uint256(MAX64, 0, 0, 0);  // 2^64 - 1
  const Uint256 b = Uint256(MAX64, 0, 0, 0);  // 2^64 - 1
  // Product = (2^64 - 1)^2 = 2^128 - 2^65 + 1, fits in 128 bits
  const Uint256 expected = (a * b) % Uint256(1000);
  EXPECT_EQ(Uint256::mulmod(a, b, Uint256(1000)), expected);
}

// =============================================================================
// EXP: base^exponent mod 2^256
// =============================================================================

TEST(Uint256Exp, ExponentZero) {
  // x^0 = 1 for any x
  EXPECT_EQ(Uint256::exp(ZERO, ZERO), ONE);
  EXPECT_EQ(Uint256::exp(ONE, ZERO), ONE);
  EXPECT_EQ(Uint256::exp(Uint256(12345), ZERO), ONE);
  EXPECT_EQ(Uint256::exp(MAX256, ZERO), ONE);
}

TEST(Uint256Exp, BaseZero) {
  // 0^n = 0 for n > 0
  EXPECT_EQ(Uint256::exp(ZERO, ONE), ZERO);
  EXPECT_EQ(Uint256::exp(ZERO, Uint256(100)), ZERO);
  EXPECT_EQ(Uint256::exp(ZERO, MAX256), ZERO);
}

TEST(Uint256Exp, BaseOne) {
  // 1^n = 1 for any n
  EXPECT_EQ(Uint256::exp(ONE, ONE), ONE);
  EXPECT_EQ(Uint256::exp(ONE, Uint256(100)), ONE);
  EXPECT_EQ(Uint256::exp(ONE, MAX256), ONE);
}

TEST(Uint256Exp, ExponentOne) {
  // x^1 = x
  EXPECT_EQ(Uint256::exp(Uint256(42), ONE), Uint256(42));
  EXPECT_EQ(Uint256::exp(MAX256, ONE), MAX256);
}

TEST(Uint256Exp, SmallPowers) {
  // 2^10 = 1024
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(10)), Uint256(1024));

  // 3^5 = 243
  EXPECT_EQ(Uint256::exp(Uint256(3), Uint256(5)), Uint256(243));

  // 10^6 = 1,000,000
  EXPECT_EQ(Uint256::exp(Uint256(10), Uint256(6)), Uint256(1000000));
}

TEST(Uint256Exp, PowersOfTwo) {
  // 2^64 = 2^64
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(64)), Uint256(0, 1, 0, 0));

  // 2^128
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(128)), Uint256(0, 0, 1, 0));

  // 2^192
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(192)), Uint256(0, 0, 0, 1));

  // 2^255 (highest bit)
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(255)), Uint256(0, 0, 0, 0x8000000000000000ULL));
}

TEST(Uint256Exp, PowersOfTwoOverflow) {
  // 2^256 overflows to 0
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(256)), ZERO);

  // 2^300 also 0
  EXPECT_EQ(Uint256::exp(Uint256(2), Uint256(300)), ZERO);
}

TEST(Uint256Exp, EvenBaseOverflow) {
  // Optimization: even base with large exponent overflows to 0
  EXPECT_EQ(Uint256::exp(Uint256(4), Uint256(200)), ZERO);  // 4^200 = 2^400 = 0
  EXPECT_EQ(Uint256::exp(Uint256(100), Uint256(256)), ZERO);
}

TEST(Uint256Exp, OddBaseLargeExponent) {
  // Odd bases don't overflow to 0, but result wraps
  // 3^256 mod 2^256 is some non-zero value
  const Uint256 result = Uint256::exp(Uint256(3), Uint256(256));
  EXPECT_FALSE(result.is_zero());
}

TEST(Uint256Exp, LargeBaseSmallExponent) {
  // (2^128)^2 = 2^256 = 0 (overflow)
  const Uint256 pow128 = Uint256(0, 0, 1, 0);
  EXPECT_EQ(Uint256::exp(pow128, Uint256(2)), ZERO);

  // (2^127)^2 = 2^254 (not overflow)
  // 2^127 = 2^(64+63) has bit 127 set, which is in limb 1 (127/64=1) at position 63
  const Uint256 pow127 = Uint256(0, 0x8000000000000000ULL, 0, 0);
  // 2^254 has bit 254 set, which is in limb 3 (254/64=3) at position 62
  const Uint256 expected = Uint256(0, 0, 0, 0x4000000000000000ULL);
  EXPECT_EQ(Uint256::exp(pow127, Uint256(2)), expected);
}

TEST(Uint256Exp, ModularExponentiationProperty) {
  // (a * b)^n = a^n * b^n (mod 2^256)
  const Uint256 a = Uint256(3);
  const Uint256 b = Uint256(5);
  const Uint256 n = Uint256(10);

  const Uint256 lhs = Uint256::exp(a * b, n);
  const Uint256 rhs = Uint256::exp(a, n) * Uint256::exp(b, n);
  EXPECT_EQ(lhs, rhs);
}

TEST(Uint256Exp, MultiLimbExponent) {
  // Test exponent with bits set in multiple limbs
  // a^(m+n) = a^m * a^n
  const Uint256 base(3);
  const Uint256 exp_2_64(0, 1, 0, 0);         // 2^64
  const Uint256 exp_2_64_plus_1(1, 1, 0, 0);  // 2^64 + 1

  // 3^(2^64 + 1) should equal 3^(2^64) * 3^1
  const Uint256 result_combined = Uint256::exp(base, exp_2_64_plus_1);
  const Uint256 result_separate = Uint256::exp(base, exp_2_64) * Uint256::exp(base, Uint256(1));

  EXPECT_EQ(result_combined, result_separate);
}

TEST(Uint256Exp, ExponentInHigherLimbOnly) {
  // Test exponent with bits only in limb 1 (not limb 0)
  // 3^(2^64) should NOT equal 3 (which would happen if squaring was skipped)
  const Uint256 base(3);
  const Uint256 exp_2_64(0, 1, 0, 0);  // 2^64

  const Uint256 result = Uint256::exp(base, exp_2_64);

  // With the bug, this would incorrectly return 3 (no squaring happened in limb 0)
  EXPECT_NE(result, base);
}

// =============================================================================
// byte_length() Tests (used for EXP gas calculation)
// =============================================================================

TEST(Uint256ByteLength, Zero) { EXPECT_EQ(ZERO.byte_length(), 0U); }

TEST(Uint256ByteLength, SmallValues) {
  EXPECT_EQ(Uint256(1).byte_length(), 1U);
  EXPECT_EQ(Uint256(255).byte_length(), 1U);
  EXPECT_EQ(Uint256(256).byte_length(), 2U);
  EXPECT_EQ(Uint256(0xFFFF).byte_length(), 2U);
  EXPECT_EQ(Uint256(0x10000).byte_length(), 3U);
}

TEST(Uint256ByteLength, LimbBoundaries) {
  // Limb 0 full
  EXPECT_EQ(Uint256(MAX64).byte_length(), 8U);

  // Limb 1 starts
  EXPECT_EQ(Uint256(0, 1, 0, 0).byte_length(), 9U);

  // Limb 1 full
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0).byte_length(), 16U);

  // Limb 2 starts
  EXPECT_EQ(Uint256(0, 0, 1, 0).byte_length(), 17U);

  // Limb 3 starts
  EXPECT_EQ(Uint256(0, 0, 0, 1).byte_length(), 25U);

  // Max value
  EXPECT_EQ(MAX256.byte_length(), 32U);
}

}  // namespace div0::types
