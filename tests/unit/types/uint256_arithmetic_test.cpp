// Unit tests for Uint256 arithmetic operations: addition, subtraction, multiplication
// Covers modular arithmetic semantics (mod 2^256) as required by EVM

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// Addition: Basic Operations
// =============================================================================

TEST(Uint256Addition, ZeroPlusZero) { EXPECT_EQ(Uint256(0) + Uint256(0), Uint256(0)); }

TEST(Uint256Addition, ZeroPlusValue) {
  EXPECT_EQ(Uint256(0) + Uint256(42), Uint256(42));
  EXPECT_EQ(Uint256(42) + Uint256(0), Uint256(42));
  EXPECT_EQ(Uint256(0) + Uint256(MAX64), Uint256(MAX64));
}

TEST(Uint256Addition, SmallValues) {
  EXPECT_EQ(Uint256(1) + Uint256(1), Uint256(2));
  EXPECT_EQ(Uint256(100) + Uint256(200), Uint256(300));
  EXPECT_EQ(Uint256(MAX64 - 1) + Uint256(1), Uint256(MAX64));
}

// =============================================================================
// Addition: Carry Propagation
// =============================================================================

TEST(Uint256Addition, CarryIntoSecondLimb) {
  // MAX64 + 1 should carry into limb 1
  EXPECT_EQ(Uint256(MAX64) + Uint256(1), Uint256(0, 1, 0, 0));
}

TEST(Uint256Addition, CarryIntoThirdLimb) {
  // (MAX64, MAX64) + 1 should carry into limb 2
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0) + Uint256(1), Uint256(0, 0, 1, 0));
}

TEST(Uint256Addition, CarryIntoFourthLimb) {
  // (MAX64, MAX64, MAX64) + 1 should carry into limb 3
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, 0) + Uint256(1), Uint256(0, 0, 0, 1));
}

TEST(Uint256Addition, CarryPropagationChain) {
  // Test carry propagation through all limbs
  constexpr auto ALMOST_MAX = Uint256(MAX64, MAX64, MAX64, 0);
  constexpr auto ONE = Uint256(1);
  EXPECT_EQ(ALMOST_MAX + ONE, Uint256(0, 0, 0, 1));
}

TEST(Uint256Addition, MultiLimbAddition) {
  constexpr auto A = Uint256(0x8000000000000000ULL, 0x8000000000000000ULL, 0, 0);
  constexpr auto B = Uint256(0x8000000000000000ULL, 0x8000000000000000ULL, 0, 0);
  // Each limb overflows, carry propagates
  EXPECT_EQ(A + B, Uint256(0, 1, 1, 0));
}

// =============================================================================
// Addition: Overflow (Modular Arithmetic)
// =============================================================================

TEST(Uint256Addition, OverflowWrapsToZero) {
  // MAX + 1 = 0 (mod 2^256)
  constexpr auto MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(MAX256 + Uint256(1), Uint256(0));
}

TEST(Uint256Addition, OverflowWrapsCorrectly) {
  // MAX + 2 = 1 (mod 2^256)
  constexpr auto MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(MAX256 + Uint256(2), Uint256(1));

  // MAX + MAX = MAX - 1 (since MAX + MAX = 2*MAX = 2^256 - 2 mod 2^256)
  EXPECT_EQ(MAX256 + MAX256, Uint256(MAX64 - 1, MAX64, MAX64, MAX64));
}

TEST(Uint256Addition, OverflowPartialLimbs) {
  constexpr auto A = Uint256(MAX64, MAX64, MAX64, MAX64 >> 1);
  constexpr auto B = Uint256(0, 0, 0, MAX64 >> 1);
  // Should not overflow (just fills the high limb)
  EXPECT_EQ(A + B, Uint256(MAX64, MAX64, MAX64, MAX64 - 1));
}

// =============================================================================
// Addition: Commutativity
// =============================================================================

TEST(Uint256Addition, Commutativity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL, 0, 0);
  constexpr auto B = Uint256(0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL, 0, 0);
  EXPECT_EQ(A + B, B + A);
}

// =============================================================================
// Addition: Associativity
// =============================================================================

TEST(Uint256Addition, Associativity) {
  constexpr auto A = Uint256(MAX64, 0, 0, 0);
  constexpr auto B = Uint256(MAX64, 0, 0, 0);
  constexpr auto C = Uint256(MAX64, 0, 0, 0);
  EXPECT_EQ((A + B) + C, A + (B + C));
}

// =============================================================================
// Subtraction: Basic Operations
// =============================================================================

TEST(Uint256Subtraction, ZeroMinusZero) { EXPECT_EQ(Uint256(0) - Uint256(0), Uint256(0)); }

TEST(Uint256Subtraction, ValueMinusZero) {
  EXPECT_EQ(Uint256(42) - Uint256(0), Uint256(42));
  EXPECT_EQ(Uint256(MAX64) - Uint256(0), Uint256(MAX64));
}

TEST(Uint256Subtraction, SmallValues) {
  EXPECT_EQ(Uint256(10) - Uint256(3), Uint256(7));
  EXPECT_EQ(Uint256(1000) - Uint256(999), Uint256(1));
  EXPECT_EQ(Uint256(MAX64) - Uint256(MAX64 - 1), Uint256(1));
}

TEST(Uint256Subtraction, EqualValuesResultInZero) {
  EXPECT_EQ(Uint256(42) - Uint256(42), Uint256(0));
  EXPECT_EQ(Uint256(MAX64) - Uint256(MAX64), Uint256(0));
  constexpr auto LARGE = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(LARGE - LARGE, Uint256(0));
}

// =============================================================================
// Subtraction: Borrow Propagation
// =============================================================================

TEST(Uint256Subtraction, BorrowFromSecondLimb) {
  // (0, 1) - 1 = MAX64 (borrow from limb 1)
  EXPECT_EQ(Uint256(0, 1, 0, 0) - Uint256(1), Uint256(MAX64, 0, 0, 0));
}

TEST(Uint256Subtraction, BorrowFromThirdLimb) {
  // (0, 0, 1) - 1 = (MAX64, MAX64)
  EXPECT_EQ(Uint256(0, 0, 1, 0) - Uint256(1), Uint256(MAX64, MAX64, 0, 0));
}

TEST(Uint256Subtraction, BorrowFromFourthLimb) {
  // (0, 0, 0, 1) - 1 = (MAX64, MAX64, MAX64)
  EXPECT_EQ(Uint256(0, 0, 0, 1) - Uint256(1), Uint256(MAX64, MAX64, MAX64, 0));
}

TEST(Uint256Subtraction, BorrowPropagationChain) {
  // Test borrow propagation through all limbs
  constexpr auto POWER192 = Uint256(0, 0, 0, 1);
  constexpr auto ONE = Uint256(1);
  EXPECT_EQ(POWER192 - ONE, Uint256(MAX64, MAX64, MAX64, 0));
}

// =============================================================================
// Subtraction: Underflow (Modular Arithmetic)
// =============================================================================

TEST(Uint256Subtraction, UnderflowWrapsToMax) {
  // 0 - 1 = MAX (mod 2^256)
  EXPECT_EQ(Uint256(0) - Uint256(1), Uint256(MAX64, MAX64, MAX64, MAX64));
}

TEST(Uint256Subtraction, UnderflowWrapsCorrectly) {
  // 0 - 2 = MAX - 1
  EXPECT_EQ(Uint256(0) - Uint256(2), Uint256(MAX64 - 1, MAX64, MAX64, MAX64));

  // 1 - 2 = MAX
  EXPECT_EQ(Uint256(1) - Uint256(2), Uint256(MAX64, MAX64, MAX64, MAX64));
}

// =============================================================================
// Subtraction: Inverse of Addition
// =============================================================================

TEST(Uint256Subtraction, InverseOfAddition) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL, 0, 0);
  constexpr auto B = Uint256(0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL, 0, 0);
  EXPECT_EQ((A + B) - B, A);
  EXPECT_EQ((A + B) - A, B);
}

// =============================================================================
// Multiplication: Basic Operations
// =============================================================================

TEST(Uint256Multiplication, ZeroTimesAnything) {
  EXPECT_EQ(Uint256(0) * Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) * Uint256(42), Uint256(0));
  EXPECT_EQ(Uint256(42) * Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) * Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(0));
}

TEST(Uint256Multiplication, OneTimesAnything) {
  EXPECT_EQ(Uint256(1) * Uint256(42), Uint256(42));
  EXPECT_EQ(Uint256(42) * Uint256(1), Uint256(42));
  constexpr auto LARGE = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(Uint256(1) * LARGE, LARGE);
}

TEST(Uint256Multiplication, SmallValues) {
  EXPECT_EQ(Uint256(2) * Uint256(3), Uint256(6));
  EXPECT_EQ(Uint256(100) * Uint256(200), Uint256(20000));
  EXPECT_EQ(Uint256(12345) * Uint256(67890), Uint256(838102050ULL));
}

// =============================================================================
// Multiplication: Powers of Two (Equivalent to Left Shift)
// =============================================================================

TEST(Uint256Multiplication, PowersOfTwo) {
  EXPECT_EQ(Uint256(1) * Uint256(2), Uint256(2));
  EXPECT_EQ(Uint256(1) * Uint256(256), Uint256(256));
  EXPECT_EQ(Uint256(0xFF) * Uint256(256), Uint256(0xFF00));

  // Multiply by 2^64
  EXPECT_EQ(Uint256(1) * Uint256(0, 1, 0, 0), Uint256(0, 1, 0, 0));
  EXPECT_EQ(Uint256(42) * Uint256(0, 1, 0, 0), Uint256(0, 42, 0, 0));
}

// =============================================================================
// Multiplication: Carry Across Limbs
// =============================================================================

TEST(Uint256Multiplication, SingleLimbOverflow) {
  // (2^32) * (2^32) = 2^64, which needs limb 1
  constexpr auto TWO_POW_32 = 1ULL << 32;
  EXPECT_EQ(Uint256(TWO_POW_32) * Uint256(TWO_POW_32), Uint256(0, 1, 0, 0));
}

TEST(Uint256Multiplication, MultiLimbProduct) {
  // (2^64 - 1) * 2 = 2^65 - 2
  EXPECT_EQ(Uint256(MAX64) * Uint256(2), Uint256(MAX64 - 1, 1, 0, 0));

  // (2^64 - 1) * (2^64 - 1) = 2^128 - 2^65 + 1
  EXPECT_EQ(Uint256(MAX64) * Uint256(MAX64), Uint256(1, MAX64 - 1, 0, 0));
}

TEST(Uint256Multiplication, ThreeLimbProduct) {
  // Value that produces a three-limb result
  constexpr auto A = Uint256(0, 1, 0, 0);  // 2^64
  constexpr auto B = Uint256(0, 1, 0, 0);  // 2^64
  EXPECT_EQ(A * B, Uint256(0, 0, 1, 0));   // 2^128
}

TEST(Uint256Multiplication, FourLimbProduct) {
  // Value that produces a four-limb result
  constexpr auto A = Uint256(0, 0, 1, 0);  // 2^128
  constexpr auto B = Uint256(0, 1, 0, 0);  // 2^64
  EXPECT_EQ(A * B, Uint256(0, 0, 0, 1));   // 2^192
}

// =============================================================================
// Multiplication: Overflow (Modular Arithmetic)
// =============================================================================

TEST(Uint256Multiplication, OverflowTruncates) {
  // 2^192 * 2^64 = 2^256 = 0 (mod 2^256)
  constexpr auto A = Uint256(0, 0, 0, 1);  // 2^192
  constexpr auto B = Uint256(0, 1, 0, 0);  // 2^64
  EXPECT_EQ(A * B, Uint256(0));

  // 2^192 * 2^65 = 2^257 = 0 (mod 2^256)
  constexpr auto C = Uint256(0, 2, 0, 0);  // 2^65
  EXPECT_EQ(A * C, Uint256(0));
}

TEST(Uint256Multiplication, PartialOverflow) {
  // MAX * 2 should overflow and lose the high bit
  constexpr auto MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);
  // MAX * 2 = 2*MAX = 2^257 - 2 mod 2^256 = -2 mod 2^256 = MAX - 1
  EXPECT_EQ(MAX256 * Uint256(2), Uint256(MAX64 - 1, MAX64, MAX64, MAX64));
}

// =============================================================================
// Multiplication: Commutativity
// =============================================================================

TEST(Uint256Multiplication, Commutativity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x1234567890ABCDEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(A * B, B * A);
}

// =============================================================================
// Multiplication: Associativity
// =============================================================================

TEST(Uint256Multiplication, Associativity) {
  constexpr auto A = Uint256(1000);
  constexpr auto B = Uint256(2000);
  constexpr auto C = Uint256(3000);
  EXPECT_EQ((A * B) * C, A * (B * C));
}

// =============================================================================
// Multiplication: Distributivity over Addition
// =============================================================================

TEST(Uint256Multiplication, Distributivity) {
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);
  constexpr auto C = Uint256(300);
  EXPECT_EQ(A * (B + C), A * B + A * C);
}

// =============================================================================
// Multiplication: Division Inverse (for non-overflowing cases)
// =============================================================================

TEST(Uint256Multiplication, DivisionInverse) {
  constexpr auto A = Uint256(12345);
  constexpr auto B = Uint256(67890);
  const Uint256 product = A * B;
  EXPECT_EQ(product / A, B);
  EXPECT_EQ(product / B, A);
}

// =============================================================================
// Boundary Values
// =============================================================================

TEST(Uint256Arithmetic, LimbBoundaries) {
  // Test operations at 64-bit boundaries
  constexpr auto LIMB_BOUNDARY = Uint256(0, 1, 0, 0);  // 2^64

  EXPECT_EQ(LIMB_BOUNDARY + LIMB_BOUNDARY, Uint256(0, 2, 0, 0));
  EXPECT_EQ(LIMB_BOUNDARY - Uint256(1), Uint256(MAX64, 0, 0, 0));
  EXPECT_EQ(LIMB_BOUNDARY * Uint256(2), Uint256(0, 2, 0, 0));
}

TEST(Uint256Arithmetic, MaxValuesPerLimb) {
  // Test with max values in each limb
  constexpr auto ALL_MAX = Uint256(MAX64, MAX64, MAX64, MAX64);
  constexpr auto ONE = Uint256(1);

  // MAX + 1 = 0
  EXPECT_EQ(ALL_MAX + ONE, Uint256(0));

  // MAX - MAX = 0
  EXPECT_EQ(ALL_MAX - ALL_MAX, Uint256(0));

  // MAX * 1 = MAX
  EXPECT_EQ(ALL_MAX * ONE, ALL_MAX);
}

// =============================================================================
// EVM-Specific Test Cases
// =============================================================================

TEST(Uint256Arithmetic, EVMWeiCalculations) {
  // 1 ETH = 10^18 wei
  constexpr auto ONE_ETH_WEI = Uint256(1000000000000000000ULL);
  const auto ten_eth = ONE_ETH_WEI * Uint256(10);
  EXPECT_EQ(ten_eth, Uint256(10000000000000000000ULL));
}

TEST(Uint256Arithmetic, EVMGasCalculations) {
  // gas_price * gas_used
  constexpr auto GAS_PRICE = Uint256(20000000000ULL);  // 20 Gwei
  constexpr auto GAS_USED = Uint256(21000);            // Standard transfer
  const auto cost = GAS_PRICE * GAS_USED;
  EXPECT_EQ(cost, Uint256(420000000000000ULL));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Arithmetic, RepeatedAdditions) {
  auto sum = Uint256(0);
  for (uint64_t i = 1; i <= 1000; ++i) {
    sum = sum + Uint256(i);
  }
  // Sum of 1 to 1000 = 1000 * 1001 / 2 = 500500
  EXPECT_EQ(sum, Uint256(500500));
}

TEST(Uint256Arithmetic, RepeatedMultiplications) {
  auto product = Uint256(1);
  for (uint64_t i = 1; i <= 10; ++i) {
    product = product * Uint256(i);
  }
  // 10! = 3628800
  EXPECT_EQ(product, Uint256(3628800));
}

TEST(Uint256Arithmetic, AlternatingOperations) {
  constexpr auto A = Uint256(1000000);
  constexpr auto B = Uint256(500);

  Uint256 result = A;
  result = result + B;  // 1000500
  result = result - B;  // 1000000
  result = result * B;  // 500000000
  result = result / B;  // 1000000
  EXPECT_EQ(result, A);
}

}  // namespace div0::types
