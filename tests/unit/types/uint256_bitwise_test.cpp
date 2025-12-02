// Unit tests for Uint256 bitwise operations: AND, OR, XOR, NOT
// These operations are fundamental for EVM bytecode manipulation

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// Bitwise AND: Basic Operations
// =============================================================================

TEST(Uint256BitwiseAnd, ZeroAndAnything) {
  EXPECT_EQ(Uint256(0) & Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) & Uint256(MAX64), Uint256(0));
  EXPECT_EQ(Uint256(MAX64) & Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) & Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(0));
}

TEST(Uint256BitwiseAnd, AllOnesAndAnything) {
  constexpr auto ALL_ONES = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(ALL_ONES & Uint256(42), Uint256(42));
  EXPECT_EQ(ALL_ONES & ALL_ONES, ALL_ONES);
  EXPECT_EQ(Uint256(0xDEADBEEF) & ALL_ONES, Uint256(0xDEADBEEF));
}

TEST(Uint256BitwiseAnd, SingleBits) {
  EXPECT_EQ(Uint256(0b1010) & Uint256(0b1100), Uint256(0b1000));
  EXPECT_EQ(Uint256(0xFF) & Uint256(0x0F), Uint256(0x0F));
  EXPECT_EQ(Uint256(0xFFFF0000) & Uint256(0x0000FFFF), Uint256(0));
}

TEST(Uint256BitwiseAnd, MultiLimb) {
  constexpr auto A =
      Uint256(0xFFFFFFFFFFFFFFFFULL, 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, 0);
  constexpr auto B =
      Uint256(0x0F0F0F0F0F0F0F0FULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, MAX64);
  constexpr auto EXPECTED =
      Uint256(0x0F0F0F0F0F0F0F0FULL, 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, 0);
  EXPECT_EQ(A & B, EXPECTED);
}

TEST(Uint256BitwiseAnd, Commutativity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0xFF00FF00FF00FF00ULL, 0x12345678, 0, 0);
  EXPECT_EQ(A & B, B & A);
}

TEST(Uint256BitwiseAnd, Idempotent) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0x12345678, 0x87654321);
  EXPECT_EQ(A & A, A);
}

// =============================================================================
// Bitwise OR: Basic Operations
// =============================================================================

TEST(Uint256BitwiseOr, ZeroOrAnything) {
  EXPECT_EQ(Uint256(0) | Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) | Uint256(42), Uint256(42));
  EXPECT_EQ(Uint256(42) | Uint256(0), Uint256(42));
}

TEST(Uint256BitwiseOr, AllOnesOrAnything) {
  constexpr auto ALL_ONES = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(ALL_ONES | Uint256(0), ALL_ONES);
  EXPECT_EQ(ALL_ONES | Uint256(42), ALL_ONES);
  EXPECT_EQ(Uint256(0) | ALL_ONES, ALL_ONES);
}

TEST(Uint256BitwiseOr, SingleBits) {
  EXPECT_EQ(Uint256(0b1010) | Uint256(0b0101), Uint256(0b1111));
  EXPECT_EQ(Uint256(0xFF00) | Uint256(0x00FF), Uint256(0xFFFF));
  EXPECT_EQ(Uint256(0xF0F0F0F0) | Uint256(0x0F0F0F0F), Uint256(0xFFFFFFFF));
}

TEST(Uint256BitwiseOr, MultiLimb) {
  constexpr auto A = Uint256(0xFF00FF00FF00FF00ULL, 0, 0xFF00FF00FF00FF00ULL, 0);
  constexpr auto B =
      Uint256(0x00FF00FF00FF00FFULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0xFFFFFFFFFFFFFFFFULL);
  constexpr auto EXPECTED = Uint256(MAX64, MAX64, 0xFF00FF00FF00FF00ULL, MAX64);
  EXPECT_EQ(A | B, EXPECTED);
}

TEST(Uint256BitwiseOr, Commutativity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(A | B, B | A);
}

TEST(Uint256BitwiseOr, Idempotent) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0x12345678, 0x87654321);
  EXPECT_EQ(A | A, A);
}

// =============================================================================
// Bitwise XOR: Basic Operations
// =============================================================================

TEST(Uint256BitwiseXor, ZeroXorAnything) {
  EXPECT_EQ(Uint256(0) ^ Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(0) ^ Uint256(42), Uint256(42));
  EXPECT_EQ(Uint256(42) ^ Uint256(0), Uint256(42));
}

TEST(Uint256BitwiseXor, SameValueXorIsZero) {
  EXPECT_EQ(Uint256(42) ^ Uint256(42), Uint256(0));
  EXPECT_EQ(Uint256(MAX64) ^ Uint256(MAX64), Uint256(0));
  constexpr auto LARGE = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_EQ(LARGE ^ LARGE, Uint256(0));
}

TEST(Uint256BitwiseXor, SingleBits) {
  EXPECT_EQ(Uint256(0b1010) ^ Uint256(0b1100), Uint256(0b0110));
  EXPECT_EQ(Uint256(0xFF00) ^ Uint256(0x00FF), Uint256(0xFFFF));
  EXPECT_EQ(Uint256(0xFFFF) ^ Uint256(0xFFFF), Uint256(0));
}

TEST(Uint256BitwiseXor, AllOnesXor) {
  constexpr auto ALL_ONES = Uint256(MAX64, MAX64, MAX64, MAX64);
  constexpr auto VALUE = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL, 0, 0);
  // XOR with all ones is equivalent to NOT
  EXPECT_EQ(VALUE ^ ALL_ONES, ~VALUE);
}

TEST(Uint256BitwiseXor, MultiLimb) {
  constexpr auto A = Uint256(0xFFFFFFFFFFFFFFFFULL, 0xAAAAAAAAAAAAAAAAULL, 0, 0);
  constexpr auto B = Uint256(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, 0, 0);
  constexpr auto EXPECTED = Uint256(0x5555555555555555ULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0);
  EXPECT_EQ(A ^ B, EXPECTED);
}

TEST(Uint256BitwiseXor, Commutativity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(A ^ B, B ^ A);
}

TEST(Uint256BitwiseXor, SelfInverse) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0x12345678, 0x87654321);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0xABCDEF01, 0xFEDCBA98);
  // (A ^ B) ^ B == A
  EXPECT_EQ((A ^ B) ^ B, A);
  // (A ^ B) ^ A == B
  EXPECT_EQ((A ^ B) ^ A, B);
}

// =============================================================================
// Bitwise NOT: Basic Operations
// =============================================================================

TEST(Uint256BitwiseNot, NotZero) { EXPECT_EQ(~Uint256(0), Uint256(MAX64, MAX64, MAX64, MAX64)); }

TEST(Uint256BitwiseNot, NotAllOnes) { EXPECT_EQ(~Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(0)); }

TEST(Uint256BitwiseNot, SingleLimb) {
  EXPECT_EQ(~Uint256(0xFF), Uint256(~0xFFULL, MAX64, MAX64, MAX64));
  EXPECT_EQ(~Uint256(0xAAAAAAAAAAAAAAAAULL), Uint256(0x5555555555555555ULL, MAX64, MAX64, MAX64));
}

TEST(Uint256BitwiseNot, MultiLimb) {
  constexpr auto A = Uint256(0xFFFFFFFF00000000ULL, 0x0000FFFF0000FFFFULL, 0, MAX64);
  constexpr auto EXPECTED = Uint256(0x00000000FFFFFFFFULL, 0xFFFF0000FFFF0000ULL, MAX64, 0);
  EXPECT_EQ(~A, EXPECTED);
}

TEST(Uint256BitwiseNot, DoubleNotIsIdentity) {
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL, 0x12345678ABCDEF01ULL,
                             0xFEDCBA9876543210ULL);
  EXPECT_EQ(~~A, A);
}

// =============================================================================
// De Morgan's Laws
// =============================================================================

TEST(Uint256BitwiseLaws, DeMorganAnd) {
  // ~(A & B) == (~A) | (~B)
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(~(A & B), (~A) | (~B));
}

TEST(Uint256BitwiseLaws, DeMorganOr) {
  // ~(A | B) == (~A) & (~B)
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(~(A | B), (~A) & (~B));
}

// =============================================================================
// Absorption Laws
// =============================================================================

TEST(Uint256BitwiseLaws, AbsorptionAnd) {
  // A & (A | B) == A
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(A & (A | B), A);
}

TEST(Uint256BitwiseLaws, AbsorptionOr) {
  // A | (A & B) == A
  constexpr auto A = Uint256(0xDEADBEEF12345678ULL, 0xCAFEBABE, 0, 0);
  constexpr auto B = Uint256(0x12345678DEADBEEFULL, 0x87654321, 0, 0);
  EXPECT_EQ(A | (A & B), A);
}

// =============================================================================
// Distributive Laws
// =============================================================================

TEST(Uint256BitwiseLaws, DistributiveAndOverOr) {
  // A & (B | C) == (A & B) | (A & C)
  constexpr auto A = Uint256(0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL, 0, 0);
  constexpr auto B = Uint256(0x0F0F0F0F0F0F0F0FULL, 0, 0, 0);
  constexpr auto C = Uint256(0xF0F0F0F0F0F0F0F0ULL, 0, 0, 0);
  EXPECT_EQ(A & (B | C), (A & B) | (A & C));
}

TEST(Uint256BitwiseLaws, DistributiveOrOverAnd) {
  // A | (B & C) == (A | B) & (A | C)
  constexpr auto A = Uint256(0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL, 0, 0);
  constexpr auto B = Uint256(0x0F0F0F0F0F0F0F0FULL, 0, 0, 0);
  constexpr auto C = Uint256(0xF0F0F0F0F0F0F0F0ULL, 0, 0, 0);
  EXPECT_EQ(A | (B & C), (A | B) & (A | C));
}

// =============================================================================
// EVM-Specific Patterns
// =============================================================================

TEST(Uint256Bitwise, EVMAddressMask) {
  // EVM addresses are 160 bits, mask with 20-byte mask
  constexpr auto ADDRESS_MASK = Uint256(MAX64, MAX64, 0xFFFFFFFF, 0);  // 160 bits
  constexpr auto FULL_VALUE = Uint256(MAX64, MAX64, MAX64, MAX64);
  const auto masked = FULL_VALUE & ADDRESS_MASK;
  EXPECT_EQ(masked, ADDRESS_MASK);
}

TEST(Uint256Bitwise, EVMByteMask) {
  // Extract single byte using AND
  constexpr auto VALUE = Uint256(0x123456789ABCDEF0ULL, 0, 0, 0);
  constexpr auto BYTE_MASK = Uint256(0xFF);
  EXPECT_EQ(VALUE & BYTE_MASK, Uint256(0xF0));
}

TEST(Uint256Bitwise, EVMSetBit) {
  // Set a specific bit using OR
  constexpr auto VALUE = Uint256(0);
  constexpr auto BIT_7 = Uint256(1ULL << 7);
  EXPECT_EQ(VALUE | BIT_7, Uint256(128));
}

TEST(Uint256Bitwise, EVMToggleBit) {
  // Toggle a specific bit using XOR
  constexpr auto VALUE = Uint256(0xFF);
  constexpr auto BIT_0 = Uint256(1);
  EXPECT_EQ(VALUE ^ BIT_0, Uint256(0xFE));
  EXPECT_EQ((VALUE ^ BIT_0) ^ BIT_0, VALUE);  // Toggle back
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Bitwise, AlternatingPatterns) {
  constexpr auto PATTERN_A = Uint256(0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL,
                                     0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL);
  constexpr auto PATTERN_5 = Uint256(0x5555555555555555ULL, 0x5555555555555555ULL,
                                     0x5555555555555555ULL, 0x5555555555555555ULL);

  // A and 5 have no overlapping bits
  EXPECT_EQ(PATTERN_A & PATTERN_5, Uint256(0));

  // A or 5 fills all bits
  EXPECT_EQ(PATTERN_A | PATTERN_5, Uint256(MAX64, MAX64, MAX64, MAX64));

  // A xor 5 fills all bits
  EXPECT_EQ(PATTERN_A ^ PATTERN_5, Uint256(MAX64, MAX64, MAX64, MAX64));

  // NOT of A is 5
  EXPECT_EQ(~PATTERN_A, PATTERN_5);
}

TEST(Uint256Bitwise, AllLimbBoundaries) {
  // Test operations at every limb boundary
  constexpr auto A = Uint256(MAX64, 0, MAX64, 0);
  constexpr auto B = Uint256(0, MAX64, 0, MAX64);

  EXPECT_EQ(A & B, Uint256(0));
  EXPECT_EQ(A | B, Uint256(MAX64, MAX64, MAX64, MAX64));
  EXPECT_EQ(A ^ B, Uint256(MAX64, MAX64, MAX64, MAX64));
}

}  // namespace div0::types
