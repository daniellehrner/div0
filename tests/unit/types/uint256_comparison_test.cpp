// Unit tests for Uint256 comparison operations: ==, !=, <, >, <=, >=
// These are fundamental for EVM conditional operations (LT, GT, EQ, etc.)

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// Equality (==): Basic Cases
// =============================================================================

TEST(Uint256Equality, ZeroEqualsZero) { EXPECT_TRUE(Uint256(0) == Uint256(0)); }

TEST(Uint256Equality, SameSmallValues) {
  EXPECT_TRUE(Uint256(1) == Uint256(1));
  EXPECT_TRUE(Uint256(42) == Uint256(42));
  EXPECT_TRUE(Uint256(MAX64) == Uint256(MAX64));
}

TEST(Uint256Equality, SameMultiLimbValues) {
  EXPECT_TRUE(Uint256(MAX64, MAX64, 0, 0) == Uint256(MAX64, MAX64, 0, 0));
  EXPECT_TRUE(Uint256(1, 2, 3, 4) == Uint256(1, 2, 3, 4));
  EXPECT_TRUE(Uint256(MAX64, MAX64, MAX64, MAX64) == Uint256(MAX64, MAX64, MAX64, MAX64));
}

TEST(Uint256Equality, DifferentSmallValues) {
  EXPECT_FALSE(Uint256(0) == Uint256(1));
  EXPECT_FALSE(Uint256(42) == Uint256(43));
  EXPECT_FALSE(Uint256(MAX64) == Uint256(MAX64 - 1));
}

TEST(Uint256Equality, DifferentByOneLimb) {
  EXPECT_FALSE(Uint256(0, 0, 0, 0) == Uint256(1, 0, 0, 0));
  EXPECT_FALSE(Uint256(0, 0, 0, 0) == Uint256(0, 1, 0, 0));
  EXPECT_FALSE(Uint256(0, 0, 0, 0) == Uint256(0, 0, 1, 0));
  EXPECT_FALSE(Uint256(0, 0, 0, 0) == Uint256(0, 0, 0, 1));
}

TEST(Uint256Equality, DifferentByOneBit) {
  // Differs only in bit 0
  EXPECT_FALSE(Uint256(0) == Uint256(1));

  // Differs only in bit 63 (top of first limb)
  EXPECT_FALSE(Uint256(0) == Uint256(1ULL << 63));

  // Differs only in bit 255 (top of fourth limb)
  EXPECT_FALSE(Uint256(0) == Uint256(0, 0, 0, 1ULL << 63));
}

TEST(Uint256Equality, Reflexivity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL, 0x12345678ABCDEF01ULL,
                             0xFEDCBA9876543210ULL);
  EXPECT_TRUE(A == A);
}

// =============================================================================
// Inequality (!=): Basic Cases
// =============================================================================

TEST(Uint256Inequality, ZeroNotEqualsOne) { EXPECT_TRUE(Uint256(0) != Uint256(1)); }

TEST(Uint256Inequality, DifferentValues) {
  EXPECT_TRUE(Uint256(1) != Uint256(2));
  EXPECT_TRUE(Uint256(MAX64) != Uint256(MAX64 - 1));
  EXPECT_TRUE(Uint256(1, 0, 0, 0) != Uint256(0, 1, 0, 0));
}

TEST(Uint256Inequality, SameValuesFalse) {
  EXPECT_FALSE(Uint256(0) != Uint256(0));
  EXPECT_FALSE(Uint256(42) != Uint256(42));
  EXPECT_FALSE(Uint256(1, 2, 3, 4) != Uint256(1, 2, 3, 4));
}

TEST(Uint256Inequality, ConsistentWithEquality) {
  constexpr auto A = Uint256(0xDEADBEEF, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678, 0x87654321, 0, 0);

  EXPECT_EQ(A == B, !(A != B));
  EXPECT_EQ(A == A, !(A != A));
}

// =============================================================================
// Less Than (<): Basic Cases
// =============================================================================

TEST(Uint256LessThan, ZeroLessThanOne) { EXPECT_TRUE(Uint256(0) < Uint256(1)); }

TEST(Uint256LessThan, SmallValues) {
  EXPECT_TRUE(Uint256(1) < Uint256(2));
  EXPECT_TRUE(Uint256(41) < Uint256(42));
  EXPECT_TRUE(Uint256(MAX64 - 1) < Uint256(MAX64));
}

TEST(Uint256LessThan, NotLessThanEqual) {
  EXPECT_FALSE(Uint256(1) < Uint256(1));
  EXPECT_FALSE(Uint256(42) < Uint256(42));
  EXPECT_FALSE(Uint256(MAX64) < Uint256(MAX64));
}

TEST(Uint256LessThan, NotLessThanGreater) {
  EXPECT_FALSE(Uint256(2) < Uint256(1));
  EXPECT_FALSE(Uint256(42) < Uint256(41));
  EXPECT_FALSE(Uint256(MAX64) < Uint256(MAX64 - 1));
}

// =============================================================================
// Less Than (<): Multi-Limb Comparisons
// =============================================================================

TEST(Uint256LessThan, DifferentLimbCounts) {
  // Single limb < two limb
  EXPECT_TRUE(Uint256(MAX64) < Uint256(0, 1, 0, 0));

  // Two limb < three limb
  EXPECT_TRUE(Uint256(MAX64, MAX64, 0, 0) < Uint256(0, 0, 1, 0));

  // Three limb < four limb
  EXPECT_TRUE(Uint256(MAX64, MAX64, MAX64, 0) < Uint256(0, 0, 0, 1));
}

TEST(Uint256LessThan, SameLimbCountDifferentHighLimb) {
  // High limb determines result
  EXPECT_TRUE(Uint256(MAX64, MAX64, MAX64, 0) < Uint256(0, 0, 0, 1));
  EXPECT_TRUE(Uint256(0, 0, 0, 1) < Uint256(0, 0, 0, 2));
}

TEST(Uint256LessThan, SameLimbCountDifferentMiddleLimb) {
  // Third limb determines when fourth is equal
  EXPECT_TRUE(Uint256(0, 0, 1, 1) < Uint256(0, 0, 2, 1));

  // Second limb determines when third and fourth are equal
  EXPECT_TRUE(Uint256(0, 1, 1, 1) < Uint256(0, 2, 1, 1));
}

TEST(Uint256LessThan, OnlyLowestLimbDiffers) {
  EXPECT_TRUE(Uint256(0, 1, 1, 1) < Uint256(1, 1, 1, 1));
  EXPECT_TRUE(Uint256(MAX64 - 1, MAX64, MAX64, MAX64) < Uint256(MAX64, MAX64, MAX64, MAX64));
}

// =============================================================================
// Greater Than (>): Basic Cases
// =============================================================================

TEST(Uint256GreaterThan, OneGreaterThanZero) { EXPECT_TRUE(Uint256(1) > Uint256(0)); }

TEST(Uint256GreaterThan, SmallValues) {
  EXPECT_TRUE(Uint256(2) > Uint256(1));
  EXPECT_TRUE(Uint256(42) > Uint256(41));
  EXPECT_TRUE(Uint256(MAX64) > Uint256(MAX64 - 1));
}

TEST(Uint256GreaterThan, NotGreaterThanEqual) {
  EXPECT_FALSE(Uint256(1) > Uint256(1));
  EXPECT_FALSE(Uint256(42) > Uint256(42));
  EXPECT_FALSE(Uint256(MAX64) > Uint256(MAX64));
}

TEST(Uint256GreaterThan, NotGreaterThanLess) {
  EXPECT_FALSE(Uint256(1) > Uint256(2));
  EXPECT_FALSE(Uint256(41) > Uint256(42));
}

TEST(Uint256GreaterThan, ConsistentWithLessThan) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);

  EXPECT_EQ(A > B, B < A);
  EXPECT_EQ(B > A, A < B);
}

// =============================================================================
// Less Than Or Equal (<=): Basic Cases
// =============================================================================

TEST(Uint256LessOrEqual, ZeroLessOrEqualZero) { EXPECT_TRUE(Uint256(0) <= Uint256(0)); }

TEST(Uint256LessOrEqual, ZeroLessOrEqualOne) { EXPECT_TRUE(Uint256(0) <= Uint256(1)); }

TEST(Uint256LessOrEqual, EqualValues) {
  EXPECT_TRUE(Uint256(42) <= Uint256(42));
  EXPECT_TRUE(Uint256(MAX64) <= Uint256(MAX64));
  EXPECT_TRUE(Uint256(1, 2, 3, 4) <= Uint256(1, 2, 3, 4));
}

TEST(Uint256LessOrEqual, LessValues) {
  EXPECT_TRUE(Uint256(41) <= Uint256(42));
  EXPECT_TRUE(Uint256(MAX64 - 1) <= Uint256(MAX64));
}

TEST(Uint256LessOrEqual, GreaterValuesFalse) {
  EXPECT_FALSE(Uint256(2) <= Uint256(1));
  EXPECT_FALSE(Uint256(42) <= Uint256(41));
}

TEST(Uint256LessOrEqual, ConsistentWithGreaterThan) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);

  EXPECT_EQ(A <= B, !(A > B));
  EXPECT_EQ(B <= A, !(B > A));
}

// =============================================================================
// Greater Than Or Equal (>=): Basic Cases
// =============================================================================

TEST(Uint256GreaterOrEqual, ZeroGreaterOrEqualZero) { EXPECT_TRUE(Uint256(0) >= Uint256(0)); }

TEST(Uint256GreaterOrEqual, OneGreaterOrEqualZero) { EXPECT_TRUE(Uint256(1) >= Uint256(0)); }

TEST(Uint256GreaterOrEqual, EqualValues) {
  EXPECT_TRUE(Uint256(42) >= Uint256(42));
  EXPECT_TRUE(Uint256(MAX64) >= Uint256(MAX64));
  EXPECT_TRUE(Uint256(1, 2, 3, 4) >= Uint256(1, 2, 3, 4));
}

TEST(Uint256GreaterOrEqual, GreaterValues) {
  EXPECT_TRUE(Uint256(42) >= Uint256(41));
  EXPECT_TRUE(Uint256(MAX64) >= Uint256(MAX64 - 1));
}

TEST(Uint256GreaterOrEqual, LessValuesFalse) {
  EXPECT_FALSE(Uint256(1) >= Uint256(2));
  EXPECT_FALSE(Uint256(41) >= Uint256(42));
}

TEST(Uint256GreaterOrEqual, ConsistentWithLessThan) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);

  EXPECT_EQ(A >= B, !(A < B));
  EXPECT_EQ(B >= A, !(B < A));
}

// =============================================================================
// Transitivity
// =============================================================================

TEST(Uint256Comparison, Transitivity) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);
  constexpr auto C = Uint256(300);

  // If A < B and B < C, then A < C
  EXPECT_TRUE(A < B);
  EXPECT_TRUE(B < C);
  EXPECT_TRUE(A < C);
}

TEST(Uint256Comparison, TransitivityMultiLimb) {
  constexpr auto A = Uint256(MAX64, 0, 0, 0);
  constexpr auto B = Uint256(0, 1, 0, 0);
  constexpr auto C = Uint256(0, 0, 1, 0);

  EXPECT_TRUE(A < B);
  EXPECT_TRUE(B < C);
  EXPECT_TRUE(A < C);
}

// =============================================================================
// Antisymmetry
// =============================================================================

TEST(Uint256Comparison, Antisymmetry) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);

  // If A < B, then NOT (B < A)
  EXPECT_TRUE(A < B);
  EXPECT_FALSE(B < A);

  // If A <= C and C <= A, then A == C
  constexpr auto C = Uint256(100);
  EXPECT_TRUE(A <= C);
  EXPECT_TRUE(C <= A);
  EXPECT_TRUE(A == C);
}

// =============================================================================
// Trichotomy
// =============================================================================

TEST(Uint256Comparison, Trichotomy) {
  // For any two values, exactly one of: A < B, A == B, A > B is true
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);
  constexpr auto C = Uint256(100);

  // A vs B: A < B
  EXPECT_TRUE(A < B);
  EXPECT_FALSE(A == B);
  EXPECT_FALSE(A > B);

  // A vs C: A == C
  EXPECT_FALSE(A < C);
  EXPECT_TRUE(A == C);
  EXPECT_FALSE(A > C);

  // B vs A: B > A
  EXPECT_FALSE(B < A);
  EXPECT_FALSE(B == A);
  EXPECT_TRUE(B > A);
}

// =============================================================================
// Boundary Values
// =============================================================================

TEST(Uint256Comparison, ZeroAndMax) {
  constexpr auto ZERO = Uint256(0);
  constexpr auto MAX = Uint256(MAX64, MAX64, MAX64, MAX64);

  EXPECT_TRUE(ZERO < MAX);
  EXPECT_TRUE(MAX > ZERO);
  EXPECT_TRUE(ZERO <= MAX);
  EXPECT_TRUE(MAX >= ZERO);
  EXPECT_FALSE(ZERO == MAX);
  EXPECT_TRUE(ZERO != MAX);
}

TEST(Uint256Comparison, MaxMinusOne) {
  constexpr auto MAX = Uint256(MAX64, MAX64, MAX64, MAX64);
  constexpr auto MAX_MINUS_ONE = Uint256(MAX64 - 1, MAX64, MAX64, MAX64);

  EXPECT_TRUE(MAX_MINUS_ONE < MAX);
  EXPECT_TRUE(MAX > MAX_MINUS_ONE);
  EXPECT_TRUE(MAX_MINUS_ONE <= MAX);
  EXPECT_TRUE(MAX >= MAX_MINUS_ONE);
}

TEST(Uint256Comparison, LimbBoundaries) {
  // Test at exact limb boundaries
  constexpr auto LIMB0_MAX = Uint256(MAX64, 0, 0, 0);
  constexpr auto LIMB1_MIN = Uint256(0, 1, 0, 0);

  EXPECT_TRUE(LIMB0_MAX < LIMB1_MIN);

  constexpr auto LIMB1_MAX = Uint256(MAX64, MAX64, 0, 0);
  constexpr auto LIMB2_MIN = Uint256(0, 0, 1, 0);

  EXPECT_TRUE(LIMB1_MAX < LIMB2_MIN);
}

// =============================================================================
// EVM-Specific Test Cases
// =============================================================================

TEST(Uint256Comparison, EVMLTOpcode) {
  // EVM LT: a < b ? 1 : 0
  EXPECT_TRUE(Uint256(9) < Uint256(10));
  EXPECT_FALSE(Uint256(10) < Uint256(10));
  EXPECT_FALSE(Uint256(11) < Uint256(10));
}

TEST(Uint256Comparison, EVMGTOpcode) {
  // EVM GT: a > b ? 1 : 0
  EXPECT_TRUE(Uint256(11) > Uint256(10));
  EXPECT_FALSE(Uint256(10) > Uint256(10));
  EXPECT_FALSE(Uint256(9) > Uint256(10));
}

TEST(Uint256Comparison, EVMEQOpcode) {
  // EVM EQ: a == b ? 1 : 0
  EXPECT_TRUE(Uint256(10) == Uint256(10));
  EXPECT_FALSE(Uint256(10) == Uint256(11));
}

TEST(Uint256Comparison, EVMISZEROOpcode) {
  // EVM ISZERO: a == 0 ? 1 : 0
  EXPECT_TRUE(Uint256(0) == Uint256(0));
  EXPECT_FALSE(Uint256(1) == Uint256(0));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Comparison, ManySequentialValues) {
  // Test that sequential values maintain proper ordering
  for (uint64_t i = 0; i < 1000; ++i) {
    EXPECT_TRUE(Uint256(i) < Uint256(i + 1));
    EXPECT_TRUE(Uint256(i) <= Uint256(i));
    EXPECT_TRUE(Uint256(i) <= Uint256(i + 1));
    EXPECT_TRUE(Uint256(i + 1) > Uint256(i));
    EXPECT_TRUE(Uint256(i) >= Uint256(i));
    EXPECT_TRUE(Uint256(i + 1) >= Uint256(i));
    EXPECT_TRUE(Uint256(i) == Uint256(i));
    EXPECT_TRUE(Uint256(i) != Uint256(i + 1));
  }
}

TEST(Uint256Comparison, PowersOfTwo) {
  // Test comparisons of powers of two
  auto prev = Uint256(1);
  for (unsigned i = 1; i < 256; ++i) {
    const auto curr = Uint256(1) << Uint256(i);
    EXPECT_TRUE(prev < curr);
    EXPECT_TRUE(curr > prev);
    prev = curr;
  }
}

TEST(Uint256Comparison, AllLimbCombinations) {
  // Test various limb combinations to ensure all limbs are compared correctly
  const std::vector<Uint256> values = {
      Uint256(0),
      Uint256(1),
      Uint256(MAX64),
      Uint256(0, 1, 0, 0),
      Uint256(MAX64, MAX64, 0, 0),
      Uint256(0, 0, 1, 0),
      Uint256(0, 0, 0, 1),
      Uint256(MAX64, MAX64, MAX64, MAX64),
  };

  // Each value should be < all values after it in the list
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = i + 1; j < values.size(); ++j) {
      EXPECT_TRUE(values[i] < values[j]);
      EXPECT_TRUE(values[j] > values[i]);
    }
  }
}

}  // namespace div0::types
