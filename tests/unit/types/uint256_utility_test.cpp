// Unit tests for Uint256 utility methods: is_zero(), count_leading_zeros()
// Also covers constructors and factory methods

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// is_zero(): Basic Cases
// =============================================================================

TEST(Uint256IsZero, ZeroIsZero) {
  EXPECT_TRUE(Uint256(0).is_zero());
  EXPECT_TRUE(Uint256().is_zero());  // Default constructor
  EXPECT_TRUE(Uint256::zero().is_zero());
}

TEST(Uint256IsZero, AllZeroLimbs) { EXPECT_TRUE(Uint256(0, 0, 0, 0).is_zero()); }

TEST(Uint256IsZero, NonZeroSingleLimb) {
  EXPECT_FALSE(Uint256(1).is_zero());
  EXPECT_FALSE(Uint256(42).is_zero());
  EXPECT_FALSE(Uint256(MAX64).is_zero());
}

TEST(Uint256IsZero, NonZeroInEachLimb) {
  // Non-zero in limb 0
  EXPECT_FALSE(Uint256(1, 0, 0, 0).is_zero());

  // Non-zero in limb 1
  EXPECT_FALSE(Uint256(0, 1, 0, 0).is_zero());

  // Non-zero in limb 2
  EXPECT_FALSE(Uint256(0, 0, 1, 0).is_zero());

  // Non-zero in limb 3
  EXPECT_FALSE(Uint256(0, 0, 0, 1).is_zero());
}

TEST(Uint256IsZero, AllLimbsNonZero) {
  EXPECT_FALSE(Uint256(1, 1, 1, 1).is_zero());
  EXPECT_FALSE(Uint256(MAX64, MAX64, MAX64, MAX64).is_zero());
}

TEST(Uint256IsZero, OneBitSet) {
  // Test with a single bit set in each possible position
  for (unsigned limb = 0; limb < 4; ++limb) {
    for (unsigned bit = 0; bit < 64; ++bit) {
      std::array<uint64_t, 4> limbs = {0, 0, 0, 0};
      limbs.at(limb) = 1ULL << bit;
      EXPECT_FALSE(Uint256(limbs[0], limbs[1], limbs[2], limbs[3]).is_zero());
    }
  }
}

// =============================================================================
// count_leading_zeros(): Zero Value
// =============================================================================

TEST(Uint256CLZ, ZeroReturns256) {
  EXPECT_EQ(Uint256(0).count_leading_zeros(), 256);
  EXPECT_EQ(Uint256().count_leading_zeros(), 256);
  EXPECT_EQ(Uint256::zero().count_leading_zeros(), 256);
}

// =============================================================================
// count_leading_zeros(): Single Limb Values
// =============================================================================

TEST(Uint256CLZ, SingleLimbHighBit) {
  // Value with bit 63 set (MSB of limb 0)
  // This is at position 63 from LSB, so 256 - 64 = 192 leading zeros
  EXPECT_EQ(Uint256(1ULL << 63).count_leading_zeros(), 192);
}

TEST(Uint256CLZ, SingleLimbLowBit) {
  // Value with only bit 0 set
  // 255 leading zeros
  EXPECT_EQ(Uint256(1).count_leading_zeros(), 255);
}

TEST(Uint256CLZ, SingleLimbMax) {
  // MAX64 has all 64 bits set, so 256 - 64 = 192 leading zeros
  EXPECT_EQ(Uint256(MAX64).count_leading_zeros(), 192);
}

TEST(Uint256CLZ, SingleLimbVariousBits) {
  // Bit 0: 255 leading zeros
  EXPECT_EQ(Uint256(1).count_leading_zeros(), 255);

  // Bit 7: 248 leading zeros
  EXPECT_EQ(Uint256(0x80).count_leading_zeros(), 248);

  // Bit 15: 240 leading zeros
  EXPECT_EQ(Uint256(0x8000).count_leading_zeros(), 240);

  // Bit 31: 224 leading zeros
  EXPECT_EQ(Uint256(0x80000000ULL).count_leading_zeros(), 224);

  // Bit 63: 192 leading zeros
  EXPECT_EQ(Uint256(0x8000000000000000ULL).count_leading_zeros(), 192);
}

// =============================================================================
// count_leading_zeros(): Multi-Limb Values
// =============================================================================

TEST(Uint256CLZ, SecondLimbHighBit) {
  // Bit 127 set (MSB of limb 1)
  // 256 - 128 = 128 leading zeros
  EXPECT_EQ(Uint256(0, 1ULL << 63, 0, 0).count_leading_zeros(), 128);
}

TEST(Uint256CLZ, SecondLimbLowBit) {
  // Bit 64 set (LSB of limb 1)
  // 256 - 65 = 191 leading zeros
  EXPECT_EQ(Uint256(0, 1, 0, 0).count_leading_zeros(), 191);
}

TEST(Uint256CLZ, ThirdLimbHighBit) {
  // Bit 191 set (MSB of limb 2)
  // 256 - 192 = 64 leading zeros
  EXPECT_EQ(Uint256(0, 0, 1ULL << 63, 0).count_leading_zeros(), 64);
}

TEST(Uint256CLZ, ThirdLimbLowBit) {
  // Bit 128 set (LSB of limb 2)
  // 256 - 129 = 127 leading zeros
  EXPECT_EQ(Uint256(0, 0, 1, 0).count_leading_zeros(), 127);
}

TEST(Uint256CLZ, FourthLimbHighBit) {
  // Bit 255 set (MSB of limb 3) - highest bit
  // 0 leading zeros
  EXPECT_EQ(Uint256(0, 0, 0, 1ULL << 63).count_leading_zeros(), 0);
}

TEST(Uint256CLZ, FourthLimbLowBit) {
  // Bit 192 set (LSB of limb 3)
  // 256 - 193 = 63 leading zeros
  EXPECT_EQ(Uint256(0, 0, 0, 1).count_leading_zeros(), 63);
}

TEST(Uint256CLZ, MaxValue) {
  // All bits set - 0 leading zeros
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64).count_leading_zeros(), 0);
}

// =============================================================================
// count_leading_zeros(): Lower Limbs Don't Affect Result
// =============================================================================

TEST(Uint256CLZ, LowerLimbsIgnored) {
  // When high limb has value, lower limbs don't matter for CLZ
  EXPECT_EQ(Uint256(0, 0, 0, 1).count_leading_zeros(), 63);
  EXPECT_EQ(Uint256(MAX64, 0, 0, 1).count_leading_zeros(), 63);
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, 1).count_leading_zeros(), 63);

  // When limb 2 is highest, limbs 0,1 don't matter
  EXPECT_EQ(Uint256(0, 0, 1, 0).count_leading_zeros(), 127);
  EXPECT_EQ(Uint256(MAX64, MAX64, 1, 0).count_leading_zeros(), 127);
}

// =============================================================================
// count_leading_zeros(): Powers of Two
// =============================================================================

TEST(Uint256CLZ, PowersOfTwo) {
  // 2^i should have (255 - i) leading zeros
  for (unsigned i = 0; i < 256; ++i) {
    const Uint256 value = Uint256(1) << Uint256(i);
    EXPECT_EQ(value.count_leading_zeros(), 255 - static_cast<int>(i));
  }
}

// =============================================================================
// count_leading_zeros(): Relationship with Bit Width
// =============================================================================

TEST(Uint256CLZ, BitWidthRelationship) {
  // bit_width = 256 - count_leading_zeros()
  // For value n, bit_width is the position of the highest set bit + 1

  EXPECT_EQ(256 - Uint256(1).count_leading_zeros(), 1);    // 1 bit
  EXPECT_EQ(256 - Uint256(2).count_leading_zeros(), 2);    // 2 bits
  EXPECT_EQ(256 - Uint256(3).count_leading_zeros(), 2);    // 2 bits
  EXPECT_EQ(256 - Uint256(255).count_leading_zeros(), 8);  // 8 bits
  EXPECT_EQ(256 - Uint256(256).count_leading_zeros(), 9);  // 9 bits
}

// =============================================================================
// Constructors: Default
// =============================================================================

TEST(Uint256Constructor, Default) {
  constexpr Uint256 VALUE;
  EXPECT_TRUE(VALUE.is_zero());
  EXPECT_EQ(VALUE.count_leading_zeros(), 256);
}

// =============================================================================
// Constructors: Single Value
// =============================================================================

TEST(Uint256Constructor, SingleValue) {
  EXPECT_EQ(Uint256(0), Uint256(0, 0, 0, 0));
  EXPECT_EQ(Uint256(1), Uint256(1, 0, 0, 0));
  EXPECT_EQ(Uint256(MAX64), Uint256(MAX64, 0, 0, 0));
  EXPECT_EQ(Uint256(0xDEADBEEF), Uint256(0xDEADBEEF, 0, 0, 0));
}

// =============================================================================
// Constructors: Four Limbs
// =============================================================================

TEST(Uint256Constructor, FourLimbs) {
  constexpr auto VALUE = Uint256(1, 2, 3, 4);

  // Verify values are stored correctly by testing arithmetic
  // (1 + 2*2^64 + 3*2^128 + 4*2^192)
  EXPECT_FALSE(VALUE.is_zero());

  // Access via comparison with known values
  EXPECT_TRUE(VALUE > Uint256(0, 0, 0, 3));
  EXPECT_TRUE(VALUE < Uint256(0, 0, 0, 5));
}

// =============================================================================
// Factory Methods
// =============================================================================

TEST(Uint256Factory, Zero) {
  EXPECT_TRUE(Uint256::zero().is_zero());
  EXPECT_EQ(Uint256::zero(), Uint256(0));
  EXPECT_EQ(Uint256::zero(), Uint256(0, 0, 0, 0));
}

TEST(Uint256Factory, One) {
  EXPECT_FALSE(Uint256::one().is_zero());
  EXPECT_EQ(Uint256::one(), Uint256(1));
  EXPECT_EQ(Uint256::one(), Uint256(1, 0, 0, 0));
}

TEST(Uint256Factory, Max) {
  EXPECT_FALSE(Uint256::max().is_zero());
  EXPECT_EQ(Uint256::max(), Uint256(MAX64, MAX64, MAX64, MAX64));
  EXPECT_EQ(Uint256::max().count_leading_zeros(), 0);

  // max + 1 should wrap to 0
  EXPECT_EQ(Uint256::max() + Uint256::one(), Uint256::zero());
}

// =============================================================================
// data_unsafe(): Access to Internal Data
// =============================================================================

TEST(Uint256DataUnsafe, ReturnsCorrectSpan) {
  constexpr auto VALUE = Uint256(1, 2, 3, 4);
  const auto data = VALUE.data_unsafe();

  EXPECT_EQ(data.size(), 4);
  EXPECT_EQ(data[0], 1);
  EXPECT_EQ(data[1], 2);
  EXPECT_EQ(data[2], 3);
  EXPECT_EQ(data[3], 4);
}

TEST(Uint256DataUnsafe, ZeroValue) {
  constexpr auto VALUE = Uint256(0);
  const auto data = VALUE.data_unsafe();

  EXPECT_EQ(data[0], 0);
  EXPECT_EQ(data[1], 0);
  EXPECT_EQ(data[2], 0);
  EXPECT_EQ(data[3], 0);
}

TEST(Uint256DataUnsafe, MaxValue) {
  constexpr auto VALUE = Uint256::max();
  const auto data = VALUE.data_unsafe();

  EXPECT_EQ(data[0], MAX64);
  EXPECT_EQ(data[1], MAX64);
  EXPECT_EQ(data[2], MAX64);
  EXPECT_EQ(data[3], MAX64);
}

// =============================================================================
// EVM-Specific Utility Tests
// =============================================================================

TEST(Uint256Utility, EVMISZEROOpcode) {
  // ISZERO returns 1 if value is 0, else 0
  EXPECT_TRUE(Uint256(0).is_zero());
  EXPECT_FALSE(Uint256(1).is_zero());
  EXPECT_FALSE(Uint256(MAX64, MAX64, MAX64, MAX64).is_zero());
}

TEST(Uint256Utility, EVMBitLengthCalculation) {
  // Useful for SIGNEXTEND and other bit-manipulation opcodes
  // bit_length = 256 - count_leading_zeros()

  // 0xFF requires 8 bits
  EXPECT_EQ(256 - Uint256(0xFF).count_leading_zeros(), 8);

  // 0x100 requires 9 bits
  EXPECT_EQ(256 - Uint256(0x100).count_leading_zeros(), 9);

  // 2^64 - 1 requires 64 bits
  EXPECT_EQ(256 - Uint256(MAX64).count_leading_zeros(), 64);

  // 2^64 requires 65 bits
  EXPECT_EQ(256 - Uint256(0, 1, 0, 0).count_leading_zeros(), 65);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Utility, CLZConsistencyWithShifts) {
  // CLZ should decrease by 1 for each left shift (until overflow)
  auto value = Uint256(1);
  int expected_clz = 255;

  for (int i = 0; i < 255; ++i) {
    EXPECT_EQ(value.count_leading_zeros(), expected_clz);
    value = value << Uint256(1);
    expected_clz--;
  }

  // Final value should have 0 leading zeros (bit 255 set)
  EXPECT_EQ(value.count_leading_zeros(), 0);
}

TEST(Uint256Utility, IsZeroAfterOperations) {
  // A - A should be zero
  constexpr auto A = Uint256(0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x87654321);
  EXPECT_TRUE((A - A).is_zero());

  // A * 0 should be zero
  EXPECT_TRUE((A * Uint256(0)).is_zero());

  // A & 0 should be zero
  EXPECT_TRUE((A & Uint256(0)).is_zero());

  // A ^ A should be zero
  EXPECT_TRUE((A ^ A).is_zero());
}

TEST(Uint256Utility, AllSingleBitPositions) {
  // Test CLZ for every possible single-bit position
  for (unsigned i = 0; i < 256; ++i) {
    const Uint256 value = Uint256(1) << Uint256(i);
    const int expected_clz = 255 - static_cast<int>(i);
    EXPECT_EQ(value.count_leading_zeros(), expected_clz);
    EXPECT_FALSE(value.is_zero());
  }
}

}  // namespace div0::types
