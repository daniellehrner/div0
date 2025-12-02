// Extensive unit tests for Uint256 division operator
// Covers all code paths in divmod() and EVM specifications

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// EVM Specification: Division by Zero
// =============================================================================

TEST(Uint256Division, DivisionByZeroReturnsZero) {
  // EVM spec: division by zero returns 0, not an error
  EXPECT_EQ(Uint256(0) / Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(1) / Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(MAX64) / Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) / Uint256(0), Uint256(0));
}

TEST(Uint256Division, ModuloByZeroReturnsZero) {
  // EVM spec: modulo by zero returns 0
  EXPECT_EQ(Uint256(0) % Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(1) % Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) % Uint256(0), Uint256(0));
}

// =============================================================================
// Dividend < Divisor (quotient = 0, remainder = dividend)
// =============================================================================

TEST(Uint256Division, DividendSmallerThanDivisorSingleLimb) {
  EXPECT_EQ(Uint256(0) / Uint256(1), Uint256(0));
  EXPECT_EQ(Uint256(1) / Uint256(2), Uint256(0));
  EXPECT_EQ(Uint256(99) / Uint256(100), Uint256(0));
  EXPECT_EQ(Uint256(MAX64 - 1) / Uint256(MAX64), Uint256(0));
}

TEST(Uint256Division, DividendSmallerThanDivisorMultiLimb) {
  // Single limb dividend vs multi-limb divisor
  EXPECT_EQ(Uint256(MAX64) / Uint256(0, 1, 0, 0), Uint256(0));

  // Two limb dividend vs larger divisor
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0) / Uint256(0, 0, 1, 0), Uint256(0));

  // Three limb dividend vs four limb divisor
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, 0) / Uint256(0, 0, 0, 1), Uint256(0));

  // Close but still smaller
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64 - 1) / Uint256(MAX64, MAX64, MAX64, MAX64),
            Uint256(0));
}

TEST(Uint256Division, RemainderWhenDividendSmallerThanDivisor) {
  EXPECT_EQ(Uint256(42) % Uint256(100), Uint256(42));
  EXPECT_EQ(Uint256(MAX64) % Uint256(0, 1, 0, 0), Uint256(MAX64));
  EXPECT_EQ(Uint256(1, 2, 3, 0) % Uint256(0, 0, 0, 1), Uint256(1, 2, 3, 0));
}

// =============================================================================
// Equal Dividend and Divisor
// =============================================================================

TEST(Uint256Division, EqualDividendAndDivisor) {
  EXPECT_EQ(Uint256(1) / Uint256(1), Uint256(1));
  EXPECT_EQ(Uint256(MAX64) / Uint256(MAX64), Uint256(1));
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0) / Uint256(MAX64, MAX64, 0, 0), Uint256(1));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) / Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(1));

  // Remainder should be 0
  EXPECT_EQ(Uint256(12345) % Uint256(12345), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) % Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(0));
}

// =============================================================================
// Single Limb Divisor (Fast Path - DivmodSingleLimb)
// =============================================================================

TEST(Uint256Division, SingleLimbDivisorBasic) {
  EXPECT_EQ(Uint256(10) / Uint256(2), Uint256(5));
  EXPECT_EQ(Uint256(100) / Uint256(10), Uint256(10));
  EXPECT_EQ(Uint256(1000000) / Uint256(1000), Uint256(1000));
}

TEST(Uint256Division, SingleLimbDivisorWithRemainder) {
  EXPECT_EQ(Uint256(10) / Uint256(3), Uint256(3));
  EXPECT_EQ(Uint256(10) % Uint256(3), Uint256(1));

  EXPECT_EQ(Uint256(100) / Uint256(7), Uint256(14));
  EXPECT_EQ(Uint256(100) % Uint256(7), Uint256(2));
}

TEST(Uint256Division, SingleLimbDivisorMaxValues) {
  // MAX64 / 1
  EXPECT_EQ(Uint256(MAX64) / Uint256(1), Uint256(MAX64));

  // MAX64 / 2
  EXPECT_EQ(Uint256(MAX64) / Uint256(2), Uint256(MAX64 / 2));

  // MAX64 / MAX64
  EXPECT_EQ(Uint256(MAX64) / Uint256(MAX64), Uint256(1));
}

TEST(Uint256Division, SingleLimbDivisorMultiLimbDividend) {
  // Two limb dividend / single limb divisor
  // (2^64) / 2 = 2^63
  EXPECT_EQ(Uint256(0, 1, 0, 0) / Uint256(2), Uint256(1ULL << 63, 0, 0, 0));

  // (2^128 - 1) / 3
  const Uint256 two_limb_max = Uint256(MAX64, MAX64, 0, 0);
  const Uint256 result = two_limb_max / Uint256(3);
  // Verify by multiplication
  const Uint256 back = result * Uint256(3) + (two_limb_max % Uint256(3));
  EXPECT_EQ(back, two_limb_max);
}

TEST(Uint256Division, SingleLimbDivisorFourLimbDividend) {
  // Full 256-bit dividend / small divisor
  const Uint256 max256 = Uint256(MAX64, MAX64, MAX64, MAX64);

  // / 2
  const Uint256 half = max256 / Uint256(2);
  EXPECT_EQ(half, Uint256(MAX64, MAX64, MAX64, MAX64 >> 1));

  // / 256
  const Uint256 div256 = max256 / Uint256(256);
  const Uint256 mod256 = max256 % Uint256(256);
  EXPECT_EQ(div256 * Uint256(256) + mod256, max256);
}

TEST(Uint256Division, SingleLimbDivisorPowersOfTwo) {
  const Uint256 val = Uint256(0x8000000000000000ULL, 0x4000000000000000ULL, 0, 0);

  // / 2
  EXPECT_EQ(val / Uint256(2), Uint256(0x4000000000000000ULL, 0x2000000000000000ULL, 0, 0));

  // / 4
  EXPECT_EQ(val / Uint256(4), Uint256(0x2000000000000000ULL, 0x1000000000000000ULL, 0, 0));
}

// =============================================================================
// Multi-Limb Divisor (Knuth Algorithm D)
// =============================================================================

TEST(Uint256Division, TwoLimbDivisorBasic) {
  // (2^64) / (2^64) = 1
  EXPECT_EQ(Uint256(0, 1, 0, 0) / Uint256(0, 1, 0, 0), Uint256(1));

  // (2^65) / (2^64) = 2
  EXPECT_EQ(Uint256(0, 2, 0, 0) / Uint256(0, 1, 0, 0), Uint256(2));

  // (2^128 - 1) / (2^64)
  EXPECT_EQ(Uint256(MAX64, MAX64, 0, 0) / Uint256(0, 1, 0, 0), Uint256(MAX64, 0, 0, 0));
}

TEST(Uint256Division, TwoLimbDivisorWithRemainder) {
  const Uint256 dividend = Uint256(100, 200, 0, 0);
  const Uint256 divisor = Uint256(50, 30, 0, 0);
  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;

  // Verify: quotient * divisor + remainder == dividend
  EXPECT_EQ(quotient * divisor + remainder, dividend);
  // And remainder < divisor
  EXPECT_TRUE(remainder < divisor);
}

TEST(Uint256Division, ThreeLimbDivisorBasic) {
  EXPECT_EQ(Uint256(0, 0, 1, 0) / Uint256(0, 0, 1, 0), Uint256(1));
  EXPECT_EQ(Uint256(0, 0, 2, 0) / Uint256(0, 0, 1, 0), Uint256(2));
  EXPECT_EQ(Uint256(0, 0, 0, 1) / Uint256(0, 0, 1, 0), Uint256(0, 1, 0, 0));
}

TEST(Uint256Division, FourLimbDivisor) {
  const Uint256 max256 = Uint256(MAX64, MAX64, MAX64, MAX64);

  // MAX / MAX = 1
  EXPECT_EQ(max256 / max256, Uint256(1));

  // MAX / (MAX - 1) = 1
  const Uint256 almost_max = Uint256(MAX64 - 1, MAX64, MAX64, MAX64);
  EXPECT_EQ(max256 / almost_max, Uint256(1));
}

// =============================================================================
// Normalization (shift == 0 vs shift > 0)
// =============================================================================

TEST(Uint256Division, NoNormalizationNeeded) {
  // Divisor with top bit already set (no shift needed)
  // 0x8000000000000000 has MSB set
  const Uint256 divisor = Uint256(0, 0x8000000000000000ULL, 0, 0);
  const Uint256 dividend = Uint256(0, 0, 1, 0);

  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;
  EXPECT_EQ(quotient * divisor + remainder, dividend);
}

TEST(Uint256Division, MaxNormalizationNeeded) {
  // Divisor = 1 in high limb position needs 63-bit shift
  const Uint256 divisor = Uint256(0, 1, 0, 0);
  const Uint256 dividend = Uint256(0, 0, 1, 0);

  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;
  EXPECT_EQ(quotient * divisor + remainder, dividend);
  EXPECT_EQ(quotient, Uint256(0, 1, 0, 0));
}

TEST(Uint256Division, VariousNormalizationShifts) {
  // Test different shift amounts by varying the top bit position
  for (int shift = 0; shift < 64; shift += 8) {
    const uint64_t top_limb = 1ULL << (63 - shift);
    const Uint256 divisor = Uint256(0, top_limb, 0, 0);
    const Uint256 dividend = Uint256(MAX64, MAX64, MAX64, 0);

    const Uint256 quotient = dividend / divisor;
    const Uint256 remainder = dividend % divisor;
    EXPECT_EQ(quotient * divisor + remainder, dividend);
  }
}

// =============================================================================
// Q-hat Refinement Loop
// =============================================================================

TEST(Uint256Division, QhatRefinementBasic) {
  // Create cases where initial q_hat estimate is too large
  // This happens when v[n-2] * q_hat > (r_hat << 64) | u[j+n-2]

  // Case: q_hat initially >= 2^64 (will definitely need refinement)
  const Uint256 dividend = Uint256(0, 0, MAX64, MAX64);
  const Uint256 divisor = Uint256(0, 0, 1, 0);

  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;
  EXPECT_EQ(quotient * divisor + remainder, dividend);
}

TEST(Uint256Division, QhatRefinementEdgeCases) {
  // Values designed to trigger q_hat refinement
  // Using divisor where v[n-1] is small relative to v[n-2]

  const Uint256 divisor = Uint256(MAX64, 1, 0, 0);  // v[1]=1, v[0]=MAX64
  const Uint256 dividend = Uint256(0, 0, 1, 0);     // Large dividend

  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;
  EXPECT_EQ(quotient * divisor + remainder, dividend);
  EXPECT_TRUE(remainder < divisor);
}

// =============================================================================
// Add-back Case (rare but critical)
// =============================================================================

TEST(Uint256Division, AddBackCase) {
  // The add-back case occurs when q_hat is still 1 too large after refinement
  // This is rare (probability < 2/base) but must be handled correctly

  // Classic case that triggers add-back:
  // Dividend and divisor crafted so that after multiply-subtract,
  // we get a negative result requiring add-back

  // Knuth's example adapted for 64-bit limbs
  const Uint256 divisor = Uint256(0, 0x8000000000000000ULL, 0, 0);
  const Uint256 dividend = Uint256(0, 0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL, 0);

  const Uint256 quotient = dividend / divisor;
  const Uint256 remainder = dividend % divisor;
  EXPECT_EQ(quotient * divisor + remainder, dividend);
}

TEST(Uint256Division, AddBackMultipleCases) {
  // Multiple divisor sizes to test add-back at different positions

  // Two-limb divisor
  {
    const Uint256 d = Uint256(1, 0x8000000000000000ULL, 0, 0);
    const Uint256 n = Uint256(0, 0x8000000000000000ULL, 0x8000000000000000ULL, 0);
    EXPECT_EQ((n / d) * d + (n % d), n);
  }

  // Three-limb divisor
  {
    const Uint256 d = Uint256(1, 0, 0x8000000000000000ULL, 0);
    const Uint256 n = Uint256(0, 0, 0x8000000000000000ULL, 0x8000000000000000ULL);
    EXPECT_EQ((n / d) * d + (n % d), n);
  }
}

// =============================================================================
// Boundary Values
// =============================================================================

TEST(Uint256Division, LimbBoundaryDividend) {
  // Dividend at exact limb boundaries (powers of 2^64)
  EXPECT_EQ(Uint256(0, 1, 0, 0) / Uint256(1), Uint256(0, 1, 0, 0));  // 2^64
  EXPECT_EQ(Uint256(0, 0, 1, 0) / Uint256(1), Uint256(0, 0, 1, 0));  // 2^128
  EXPECT_EQ(Uint256(0, 0, 0, 1) / Uint256(1), Uint256(0, 0, 0, 1));  // 2^192
}

TEST(Uint256Division, LimbBoundaryDivisor) {
  // Divisor at exact limb boundaries
  EXPECT_EQ(Uint256(0, 2, 0, 0) / Uint256(0, 1, 0, 0), Uint256(2));  // 2^65 / 2^64
  EXPECT_EQ(Uint256(0, 0, 2, 0) / Uint256(0, 0, 1, 0), Uint256(2));  // 2^129 / 2^128
}

TEST(Uint256Division, MaxMinusOne) {
  const Uint256 max256 = Uint256(MAX64, MAX64, MAX64, MAX64);
  const Uint256 max_minus_1 = Uint256(MAX64 - 1, MAX64, MAX64, MAX64);

  // MAX / (MAX-1)
  EXPECT_EQ(max256 / max_minus_1, Uint256(1));
  EXPECT_EQ(max256 % max_minus_1, Uint256(1));
}

TEST(Uint256Division, OverflowInIntermediateCalculations) {
  // Large values that stress intermediate calculations
  const Uint256 a = Uint256(MAX64, MAX64, MAX64, MAX64 >> 1);
  const Uint256 b = Uint256(MAX64, MAX64, 0, 0);

  const Uint256 q = a / b;
  const Uint256 r = a % b;
  EXPECT_EQ(q * b + r, a);
}

// =============================================================================
// Powers of Two (common in EVM)
// =============================================================================

TEST(Uint256Division, DivideByPowersOfTwo) {
  const Uint256 val = Uint256(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 0, 0);

  // / 2
  EXPECT_EQ(val / Uint256(2), Uint256(0x091A2B3C4D5E6F78ULL, 0x7F6E5D4C3B2A1908ULL, 0, 0));

  // / 256 (2^8)
  const Uint256 div256 = val / Uint256(256);
  EXPECT_EQ(div256 * Uint256(256) + val % Uint256(256), val);

  // / 2^64
  EXPECT_EQ(val / Uint256(0, 1, 0, 0), Uint256(0xFEDCBA9876543210ULL));
}

TEST(Uint256Division, DividePowersOfTwo) {
  // 2^255 / 2^254 = 2
  const Uint256 pow255 = Uint256(0, 0, 0, 1ULL << 63);
  const Uint256 pow254 = Uint256(0, 0, 0, 1ULL << 62);
  EXPECT_EQ(pow255 / pow254, Uint256(2));

  // 2^192 / 2^64 = 2^128
  EXPECT_EQ(Uint256(0, 0, 0, 1) / Uint256(0, 1, 0, 0), Uint256(0, 0, 1, 0));
}

// =============================================================================
// Small Values
// =============================================================================

TEST(Uint256Division, SmallDividendSmallDivisor) {
  for (uint64_t a = 0; a <= 20; ++a) {
    for (uint64_t b = 1; b <= 20; ++b) {
      EXPECT_EQ(Uint256(a) / Uint256(b), Uint256(a / b));
      EXPECT_EQ(Uint256(a) % Uint256(b), Uint256(a % b));
    }
  }
}

TEST(Uint256Division, OneAsOperand) {
  EXPECT_EQ(Uint256(1) / Uint256(1), Uint256(1));
  EXPECT_EQ(Uint256(1) % Uint256(1), Uint256(0));

  const Uint256 large = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(large / Uint256(1), large);
  EXPECT_EQ(large % Uint256(1), Uint256(0));

  EXPECT_EQ(Uint256(1) / large, Uint256(0));
  EXPECT_EQ(Uint256(1) % large, Uint256(1));
}

// =============================================================================
// Consecutive Quotient Digits
// =============================================================================

TEST(Uint256Division, MultipleQuotientDigits) {
  // Cases where multiple quotient limbs are non-zero

  // Quotient spans 2 limbs
  const Uint256 dividend = Uint256(0, 0, 1, 1);  // > 2^128
  const Uint256 divisor = Uint256(0, 1, 0, 0);   // 2^64
  const Uint256 quotient = dividend / divisor;
  // Should have non-zero values in limb 0 and limb 1
  EXPECT_EQ(quotient * divisor + dividend % divisor, dividend);
}

TEST(Uint256Division, AllQuotientDigitsMaximal) {
  // Dividend / divisor where all quotient digits are MAX64
  // This would be MAX256 / 1, but let's also test intermediate cases

  const Uint256 dividend = Uint256(MAX64, MAX64, MAX64, 0);
  const Uint256 divisor = Uint256(1);
  EXPECT_EQ(dividend / divisor, dividend);
}

// =============================================================================
// Random-like Test Patterns
// =============================================================================

TEST(Uint256Division, AlternatingBitPatterns) {
  // 0xAAAA... / 0x5555...
  const Uint256 a = Uint256(0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0, 0);
  const Uint256 b = Uint256(0x5555555555555555ULL, 0x5555555555555555ULL, 0, 0);

  EXPECT_EQ(a / b, Uint256(2));  // 0xAAAA = 2 * 0x5555
  EXPECT_EQ(a % b, Uint256(0));
}

TEST(Uint256Division, SpecificHexPatterns) {
  // Common patterns in EVM
  const Uint256 a = Uint256(0xDEADBEEFDEADBEEFULL, 0xCAFEBABECAFEBABEULL, 0, 0);
  const Uint256 b = Uint256(0x123456789ABCDEF0ULL);

  const Uint256 q = a / b;
  const Uint256 r = a % b;
  EXPECT_EQ(q * b + r, a);
  EXPECT_TRUE(r < b);
}

// =============================================================================
// Verify Division Identity: (a / b) * b + (a % b) == a
// =============================================================================

TEST(Uint256Division, DivisionIdentity) {
  // Test the fundamental identity with various value combinations

  struct TestCase {
    Uint256 dividend;
    Uint256 divisor;
  };

  const std::vector<TestCase> cases = {
      {Uint256(1000), Uint256(7)},
      {Uint256(MAX64), Uint256(12345)},
      {Uint256(MAX64, MAX64, 0, 0), Uint256(MAX64)},
      {Uint256(MAX64, MAX64, 0, 0), Uint256(12345, 67890, 0, 0)},
      {Uint256(MAX64, MAX64, MAX64, 0), Uint256(1, 2, 3, 0)},
      {Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(MAX64, MAX64, 0, 0)},
      {Uint256(1, 2, 3, 4), Uint256(5, 6, 7, 8)},
  };

  for (const auto& tc : cases) {
    const Uint256 q = tc.dividend / tc.divisor;
    const Uint256 r = tc.dividend % tc.divisor;
    EXPECT_EQ(q * tc.divisor + r, tc.dividend);
    EXPECT_TRUE(r < tc.divisor);
  }
}

// =============================================================================
// EVM-Specific Test Cases
// =============================================================================

TEST(Uint256Division, EVMAddressRange) {
  // Addresses are 160 bits, so let's test division in that range
  const Uint256 addr1 = Uint256(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF, 0);
  const Uint256 addr2 = Uint256(0x1234567890ABCDEFULL);

  const Uint256 q = addr1 / addr2;
  const Uint256 r = addr1 % addr2;
  EXPECT_EQ(q * addr2 + r, addr1);
}

TEST(Uint256Division, EVMBalanceCalculations) {
  // Wei calculations - 1 ETH = 10^18 wei
  const Uint256 one_eth = Uint256(1000000000000000000ULL);  // 10^18
  const Uint256 total = Uint256(0, 100, 0, 0);              // A large balance

  const Uint256 eth_count = total / one_eth;
  const Uint256 wei_remainder = total % one_eth;
  EXPECT_EQ(eth_count * one_eth + wei_remainder, total);
}

TEST(Uint256Division, EVMGasCalculations) {
  // Gas price * gas used type calculations
  const Uint256 gas_price = Uint256(20000000000ULL);  // 20 Gwei
  const Uint256 total_cost = Uint256(0, 1, 0, 0);     // Large cost

  const Uint256 gas_used = total_cost / gas_price;
  EXPECT_EQ(gas_used * gas_price + total_cost % gas_price, total_cost);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Division, RepeatedDivisions) {
  // Repeatedly divide to test consistency
  Uint256 val = Uint256(MAX64, MAX64, MAX64, MAX64);

  for (int i = 0; i < 100; ++i) {
    const Uint256 divisor = Uint256(i + 2);
    const Uint256 q = val / divisor;
    const Uint256 r = val % divisor;
    EXPECT_EQ(q * divisor + r, val);
    val = q;
    if (val.is_zero()) {
      break;
    }
  }
}

TEST(Uint256Division, SymmetricOperations) {
  // If a = q*b + r, then we should be able to verify this
  const Uint256 b = Uint256(123456789, 987654321, 0, 0);

  for (uint64_t q_val = 1; q_val <= 100; ++q_val) {
    const Uint256 q = Uint256(q_val);
    const Uint256 a = q * b;  // a = q*b, so a/b should equal q

    EXPECT_EQ(a / b, q);
    EXPECT_EQ(a % b, Uint256(0));
  }
}

// =============================================================================
// Edge Cases from Knuth's Algorithm D
// =============================================================================

TEST(Uint256Division, KnuthEdgeCase1) {
  // Test case where initial q_hat = base (2^64)
  // This requires the comparison q_hat >= base to trigger
  const Uint256 divisor = Uint256(0, 1, 0, 0);   // 2^64
  const Uint256 dividend = Uint256(0, 0, 1, 0);  // 2^128

  EXPECT_EQ(dividend / divisor, Uint256(0, 1, 0, 0));  // 2^64
}

TEST(Uint256Division, KnuthEdgeCase2) {
  // Maximum quotient digit scenario
  const Uint256 divisor = Uint256(1, 0x8000000000000000ULL, 0, 0);
  const Uint256 dividend = Uint256(MAX64, MAX64, MAX64, 0);

  const Uint256 q = dividend / divisor;
  const Uint256 r = dividend % divisor;
  EXPECT_EQ(q * divisor + r, dividend);
}

// =============================================================================
// Carry/Borrow Propagation Tests
// =============================================================================

TEST(Uint256Division, BorrowPropagation) {
  // Test proper borrow handling in multiply-subtract step
  const Uint256 dividend = Uint256(0, MAX64, MAX64, 0);
  const Uint256 divisor = Uint256(MAX64, MAX64, 0, 0);

  const Uint256 q = dividend / divisor;
  const Uint256 r = dividend % divisor;
  EXPECT_EQ(q * divisor + r, dividend);
}

TEST(Uint256Division, CarryPropagationInAddBack) {
  // Test carry handling in add-back step
  const Uint256 dividend = Uint256(MAX64, MAX64, MAX64, MAX64);
  const Uint256 divisor = Uint256(MAX64, MAX64, MAX64, 0);

  const Uint256 q = dividend / divisor;
  const Uint256 r = dividend % divisor;
  EXPECT_EQ(q * divisor + r, dividend);
}

// =============================================================================
// Denormalization Tests
// =============================================================================

TEST(Uint256Division, RemainderDenormalization) {
  // Ensure remainder is correctly de-normalized after division

  // Different shift amounts
  for (int shift = 0; shift < 64; shift += 7) {
    const uint64_t top = 1ULL << (63 - shift);
    const Uint256 divisor = Uint256(12345, top, 0, 0);
    const Uint256 dividend = Uint256(MAX64, MAX64, MAX64, 0);

    const Uint256 r = dividend % divisor;
    EXPECT_TRUE(r < divisor);
  }
}

}  // namespace div0::types
