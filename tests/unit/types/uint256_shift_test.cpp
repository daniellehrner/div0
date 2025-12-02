// Unit tests for Uint256 shift operations: left shift (<<), right shift (>>)
// Covers EVM SHL and SHR opcode semantics

#include <div0/types/uint256.h>
#include <gtest/gtest.h>

namespace div0::types {

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// Left Shift: Zero Shift
// =============================================================================

TEST(Uint256LeftShift, ShiftByZero) {
  EXPECT_EQ(Uint256(0) << Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(1) << Uint256(0), Uint256(1));
  EXPECT_EQ(Uint256(MAX64) << Uint256(0), Uint256(MAX64));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) << Uint256(0), Uint256(MAX64, MAX64, MAX64, MAX64));
}

// =============================================================================
// Left Shift: Small Shifts (within first limb)
// =============================================================================

TEST(Uint256LeftShift, SmallShifts) {
  EXPECT_EQ(Uint256(1) << Uint256(1), Uint256(2));
  EXPECT_EQ(Uint256(1) << Uint256(2), Uint256(4));
  EXPECT_EQ(Uint256(1) << Uint256(8), Uint256(256));
  EXPECT_EQ(Uint256(1) << Uint256(16), Uint256(65536));
  EXPECT_EQ(Uint256(1) << Uint256(32), Uint256(1ULL << 32));
}

TEST(Uint256LeftShift, ShiftPattern) {
  EXPECT_EQ(Uint256(0xFF) << Uint256(4), Uint256(0xFF0));
  EXPECT_EQ(Uint256(0xFF) << Uint256(8), Uint256(0xFF00));
  EXPECT_EQ(Uint256(0xDEADBEEF) << Uint256(16), Uint256(0xDEADBEEF0000ULL));
}

// =============================================================================
// Left Shift: Limb Boundary (64-bit)
// =============================================================================

TEST(Uint256LeftShift, ShiftBy64) {
  // Shift into second limb
  EXPECT_EQ(Uint256(1) << Uint256(64), Uint256(0, 1, 0, 0));
  EXPECT_EQ(Uint256(MAX64) << Uint256(64), Uint256(0, MAX64, 0, 0));
  EXPECT_EQ(Uint256(0xDEADBEEF) << Uint256(64), Uint256(0, 0xDEADBEEF, 0, 0));
}

TEST(Uint256LeftShift, ShiftBy128) {
  // Shift into third limb
  EXPECT_EQ(Uint256(1) << Uint256(128), Uint256(0, 0, 1, 0));
  EXPECT_EQ(Uint256(MAX64) << Uint256(128), Uint256(0, 0, MAX64, 0));
}

TEST(Uint256LeftShift, ShiftBy192) {
  // Shift into fourth limb
  EXPECT_EQ(Uint256(1) << Uint256(192), Uint256(0, 0, 0, 1));
  EXPECT_EQ(Uint256(MAX64) << Uint256(192), Uint256(0, 0, 0, MAX64));
}

// =============================================================================
// Left Shift: Cross-Limb Shifts (bits span limb boundaries)
// =============================================================================

TEST(Uint256LeftShift, CrossLimbShift) {
  // Shift 65 bits: crosses from limb 0 to limb 1
  EXPECT_EQ(Uint256(1) << Uint256(65), Uint256(0, 2, 0, 0));

  // Shift 63 bits: fills most of limb 0, spills 1 bit into limb 1
  EXPECT_EQ(Uint256(3) << Uint256(63), Uint256(1ULL << 63, 1, 0, 0));

  // Shift 96 bits: 1.5 limbs
  EXPECT_EQ(Uint256(1) << Uint256(96), Uint256(0, 1ULL << 32, 0, 0));
}

TEST(Uint256LeftShift, CrossMultipleLimbs) {
  // Value in limb 0 shifts across to limb 2
  EXPECT_EQ(Uint256(1) << Uint256(129), Uint256(0, 0, 2, 0));

  // Multi-limb value shifts
  constexpr auto VALUE = Uint256(MAX64, MAX64, 0, 0);
  const auto shifted = VALUE << Uint256(64);
  EXPECT_EQ(shifted, Uint256(0, MAX64, MAX64, 0));
}

// =============================================================================
// Left Shift: Maximum Shifts
// =============================================================================

TEST(Uint256LeftShift, ShiftBy255) {
  // Shift to the highest bit
  EXPECT_EQ(Uint256(1) << Uint256(255), Uint256(0, 0, 0, 1ULL << 63));
}

TEST(Uint256LeftShift, ShiftBy256ReturnsZero) {
  // EVM spec: shift >= 256 returns 0
  EXPECT_EQ(Uint256(1) << Uint256(256), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) << Uint256(256), Uint256(0));
}

TEST(Uint256LeftShift, ShiftByLargeValueReturnsZero) {
  // Shift amounts that don't fit in 8 bits
  EXPECT_EQ(Uint256(1) << Uint256(257), Uint256(0));
  EXPECT_EQ(Uint256(1) << Uint256(1000), Uint256(0));
  EXPECT_EQ(Uint256(1) << Uint256(MAX64), Uint256(0));

  // Multi-limb shift amounts
  EXPECT_EQ(Uint256(1) << Uint256(0, 1, 0, 0), Uint256(0));
  EXPECT_EQ(Uint256(1) << Uint256(MAX64, MAX64, MAX64, MAX64), Uint256(0));
}

// =============================================================================
// Left Shift: Zero Value
// =============================================================================

TEST(Uint256LeftShift, ZeroShiftedIsZero) {
  EXPECT_EQ(Uint256(0) << Uint256(1), Uint256(0));
  EXPECT_EQ(Uint256(0) << Uint256(64), Uint256(0));
  EXPECT_EQ(Uint256(0) << Uint256(255), Uint256(0));
}

// =============================================================================
// Left Shift: Overflow (bits shifted out are lost)
// =============================================================================

TEST(Uint256LeftShift, OverflowLosesBits) {
  // Shift high bits out
  constexpr auto HIGH_BIT_SET = Uint256(0, 0, 0, 1ULL << 63);
  EXPECT_EQ(HIGH_BIT_SET << Uint256(1), Uint256(0));

  // Partial overflow
  constexpr auto VALUE = Uint256(0, 0, 0, MAX64);
  EXPECT_EQ(VALUE << Uint256(1), Uint256(0, 0, 0, MAX64 - 1));  // Loses top bit
}

// =============================================================================
// Right Shift: Zero Shift
// =============================================================================

TEST(Uint256RightShift, ShiftByZero) {
  EXPECT_EQ(Uint256(0) >> Uint256(0), Uint256(0));
  EXPECT_EQ(Uint256(1) >> Uint256(0), Uint256(1));
  EXPECT_EQ(Uint256(MAX64) >> Uint256(0), Uint256(MAX64));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(0), Uint256(MAX64, MAX64, MAX64, MAX64));
}

// =============================================================================
// Right Shift: Small Shifts (within first limb)
// =============================================================================

TEST(Uint256RightShift, SmallShifts) {
  EXPECT_EQ(Uint256(2) >> Uint256(1), Uint256(1));
  EXPECT_EQ(Uint256(4) >> Uint256(2), Uint256(1));
  EXPECT_EQ(Uint256(256) >> Uint256(8), Uint256(1));
  EXPECT_EQ(Uint256(0xFF00) >> Uint256(8), Uint256(0xFF));
}

TEST(Uint256RightShift, ShiftPattern) {
  EXPECT_EQ(Uint256(0xFF0) >> Uint256(4), Uint256(0xFF));
  EXPECT_EQ(Uint256(0xDEADBEEF0000ULL) >> Uint256(16), Uint256(0xDEADBEEF));
}

// =============================================================================
// Right Shift: Limb Boundary (64-bit)
// =============================================================================

TEST(Uint256RightShift, ShiftBy64) {
  // Shift from second limb to first
  EXPECT_EQ(Uint256(0, 1, 0, 0) >> Uint256(64), Uint256(1));
  EXPECT_EQ(Uint256(0, MAX64, 0, 0) >> Uint256(64), Uint256(MAX64));
  EXPECT_EQ(Uint256(0, 0xDEADBEEF, 0, 0) >> Uint256(64), Uint256(0xDEADBEEF));
}

TEST(Uint256RightShift, ShiftBy128) {
  // Shift from third limb
  EXPECT_EQ(Uint256(0, 0, 1, 0) >> Uint256(128), Uint256(1));
  EXPECT_EQ(Uint256(0, 0, MAX64, 0) >> Uint256(128), Uint256(MAX64));
}

TEST(Uint256RightShift, ShiftBy192) {
  // Shift from fourth limb
  EXPECT_EQ(Uint256(0, 0, 0, 1) >> Uint256(192), Uint256(1));
  EXPECT_EQ(Uint256(0, 0, 0, MAX64) >> Uint256(192), Uint256(MAX64));
}

// =============================================================================
// Right Shift: Cross-Limb Shifts (bits span limb boundaries)
// =============================================================================

TEST(Uint256RightShift, CrossLimbShift) {
  // Shift 65 bits from limb 1
  EXPECT_EQ(Uint256(0, 2, 0, 0) >> Uint256(65), Uint256(1));

  // Shift 63 bits from a value spanning limbs
  EXPECT_EQ(Uint256(1ULL << 63, 1, 0, 0) >> Uint256(63), Uint256(3));

  // Shift 96 bits: 1.5 limbs
  EXPECT_EQ(Uint256(0, 1ULL << 32, 0, 0) >> Uint256(96), Uint256(1));
}

TEST(Uint256RightShift, CrossMultipleLimbs) {
  // Multi-limb value shifts
  constexpr auto VALUE = Uint256(0, MAX64, MAX64, 0);
  const auto shifted = VALUE >> Uint256(64);
  EXPECT_EQ(shifted, Uint256(MAX64, MAX64, 0, 0));
}

// =============================================================================
// Right Shift: Maximum Shifts
// =============================================================================

TEST(Uint256RightShift, ShiftBy255) {
  // Shift the highest bit down
  EXPECT_EQ(Uint256(0, 0, 0, 1ULL << 63) >> Uint256(255), Uint256(1));
}

TEST(Uint256RightShift, ShiftBy256ReturnsZero) {
  // EVM spec: shift >= 256 returns 0
  EXPECT_EQ(Uint256(1) >> Uint256(256), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(256), Uint256(0));
}

TEST(Uint256RightShift, ShiftByLargeValueReturnsZero) {
  // Shift amounts that don't fit in 8 bits
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(257), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(1000), Uint256(0));
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(MAX64), Uint256(0));

  // Multi-limb shift amounts
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(0, 1, 0, 0), Uint256(0));
}

// =============================================================================
// Right Shift: Zero Value
// =============================================================================

TEST(Uint256RightShift, ZeroShiftedIsZero) {
  EXPECT_EQ(Uint256(0) >> Uint256(1), Uint256(0));
  EXPECT_EQ(Uint256(0) >> Uint256(64), Uint256(0));
  EXPECT_EQ(Uint256(0) >> Uint256(255), Uint256(0));
}

// =============================================================================
// Right Shift: Low Bits Are Lost
// =============================================================================

TEST(Uint256RightShift, LowBitsLost) {
  EXPECT_EQ(Uint256(3) >> Uint256(1), Uint256(1));                // Loses bit 0
  EXPECT_EQ(Uint256(0xFF) >> Uint256(4), Uint256(0x0F));          // Loses low nibble
  EXPECT_EQ(Uint256(MAX64) >> Uint256(32), Uint256(0xFFFFFFFF));  // Keeps high 32 bits
}

// =============================================================================
// Shift: Inverse Relationship
// =============================================================================

TEST(Uint256Shift, LeftThenRight) {
  // (x << n) >> n == x if no overflow
  constexpr auto VALUE = Uint256(0xDEADBEEF);
  for (const unsigned shift : {1, 8, 32, 63, 64, 100, 128}) {
    EXPECT_EQ((VALUE << Uint256(shift)) >> Uint256(shift), VALUE);
  }
}

TEST(Uint256Shift, RightThenLeft) {
  // (x >> n) << n loses low bits
  constexpr auto VALUE = Uint256(0xDEADBEEF);
  EXPECT_EQ((VALUE >> Uint256(8)) << Uint256(8), Uint256(0xDEADBE00));
  EXPECT_EQ((VALUE >> Uint256(16)) << Uint256(16), Uint256(0xDEAD0000));
}

// =============================================================================
// Shift: Equivalent to Multiply/Divide by Powers of 2
// =============================================================================

TEST(Uint256Shift, LeftShiftEqualsMultiplyByPowerOf2) {
  constexpr auto VALUE = Uint256(12345);

  // << 1 == * 2
  EXPECT_EQ(VALUE << Uint256(1), VALUE * Uint256(2));

  // << 3 == * 8
  EXPECT_EQ(VALUE << Uint256(3), VALUE * Uint256(8));

  // << 10 == * 1024
  EXPECT_EQ(VALUE << Uint256(10), VALUE * Uint256(1024));
}

TEST(Uint256Shift, RightShiftEqualsDivideByPowerOf2) {
  constexpr auto VALUE = Uint256(12345678);

  // >> 1 == / 2
  EXPECT_EQ(VALUE >> Uint256(1), VALUE / Uint256(2));

  // >> 3 == / 8
  EXPECT_EQ(VALUE >> Uint256(3), VALUE / Uint256(8));

  // >> 10 == / 1024
  EXPECT_EQ(VALUE >> Uint256(10), VALUE / Uint256(1024));
}

// =============================================================================
// EVM-Specific Test Cases
// =============================================================================

TEST(Uint256Shift, EVMSHLOpcode) {
  // SHL opcode test cases from EVM spec
  // SHL(1, 1) = 2
  EXPECT_EQ(Uint256(1) << Uint256(1), Uint256(2));

  // SHL(1, 0xFF) = 0x1FE
  EXPECT_EQ(Uint256(0xFF) << Uint256(1), Uint256(0x1FE));

  // SHL(0, MAX) = MAX
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) << Uint256(0), Uint256(MAX64, MAX64, MAX64, MAX64));

  // SHL(256, 1) = 0
  EXPECT_EQ(Uint256(1) << Uint256(256), Uint256(0));
}

TEST(Uint256Shift, EVMSHROpcode) {
  // SHR opcode test cases from EVM spec
  // SHR(1, 2) = 1
  EXPECT_EQ(Uint256(2) >> Uint256(1), Uint256(1));

  // SHR(1, 0xFF) = 0x7F
  EXPECT_EQ(Uint256(0xFF) >> Uint256(1), Uint256(0x7F));

  // SHR(0, MAX) = MAX
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(0), Uint256(MAX64, MAX64, MAX64, MAX64));

  // SHR(256, MAX) = 0
  EXPECT_EQ(Uint256(MAX64, MAX64, MAX64, MAX64) >> Uint256(256), Uint256(0));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(Uint256Shift, AllShiftAmounts) {
  // Test all shift amounts from 0 to 255
  constexpr auto ONE = Uint256(1);
  for (unsigned i = 0; i < 256; ++i) {
    const Uint256 shifted = ONE << Uint256(i);
    // Should have exactly one bit set at position i
    EXPECT_EQ(shifted >> Uint256(i), ONE);
  }
}

TEST(Uint256Shift, MultiLimbAllShifts) {
  // Test shifting a multi-limb value
  constexpr auto VALUE = Uint256(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0, 0);

  for (const unsigned shift : {1, 7, 8, 15, 16, 31, 32, 63, 64, 65, 127, 128}) {
    const Uint256 left_shifted = VALUE << Uint256(shift);
    const Uint256 right_shifted = left_shifted >> Uint256(shift);
    EXPECT_EQ(right_shifted, VALUE);
  }
}

TEST(Uint256Shift, SequentialShifts) {
  // Test multiple sequential shifts
  auto value = Uint256(1);

  // Shift left 256 times by 1 should result in 0
  for (int i = 0; i < 256; ++i) {
    value = value << Uint256(1);
  }
  EXPECT_EQ(value, Uint256(0));

  // Shift right 256 times by 1 from max should result in 0
  value = Uint256(MAX64, MAX64, MAX64, MAX64);
  for (int i = 0; i < 256; ++i) {
    value = value >> Uint256(1);
  }
  EXPECT_EQ(value, Uint256(0));
}

}  // namespace div0::types
