// Unit tests for bitwise opcodes: AND, OR, XOR, NOT, BYTE, SHL, SHR, SAR, SLT, SGT

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

// clang-format off
#include "../src/evm/opcodes/op_helpers.h"
#include "../src/evm/opcodes/bitwise.h"
// clang-format on

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_AND = 3;
constexpr uint64_t GAS_OR = 3;
constexpr uint64_t GAS_XOR = 3;
constexpr uint64_t GAS_NOT = 3;
constexpr uint64_t GAS_BYTE = 3;
constexpr uint64_t GAS_SHL = 3;
constexpr uint64_t GAS_SHR = 3;
constexpr uint64_t GAS_SAR = 3;
constexpr uint64_t GAS_SLT = 3;
constexpr uint64_t GAS_SGT = 3;

constexpr auto ONE = Uint256(1);
constexpr auto ZERO = Uint256(0);
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// AND Opcode Tests
// =============================================================================

TEST(OpcodeAnd, BasicAnd) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(0b1100));  // b
  (void)stack.push(Uint256(0b1010));  // a (top)

  const auto result = and_(stack, gas, GAS_AND);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0b1000));  // 1010 & 1100 = 1000
  EXPECT_EQ(gas, 1000 - GAS_AND);
}

TEST(OpcodeAnd, AndWithZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(0xFF));

  const auto result = and_(stack, gas, GAS_AND);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

TEST(OpcodeAnd, AndWithMax) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_val = Uint256::max();
  (void)stack.push(max_val);
  (void)stack.push(Uint256(0x12345678));

  const auto result = and_(stack, gas, GAS_AND);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0x12345678));
}

TEST(OpcodeAnd, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = and_(stack, gas, GAS_AND);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

// =============================================================================
// OR Opcode Tests
// =============================================================================

TEST(OpcodeOr, BasicOr) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(0b1100));
  (void)stack.push(Uint256(0b1010));

  const auto result = or_(stack, gas, GAS_OR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0b1110));  // 1010 | 1100 = 1110
  EXPECT_EQ(gas, 1000 - GAS_OR);
}

TEST(OpcodeOr, OrWithZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(ZERO);
  (void)stack.push(Uint256(0xFF));

  const auto result = or_(stack, gas, GAS_OR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xFF));
}

// =============================================================================
// XOR Opcode Tests
// =============================================================================

TEST(OpcodeXor, BasicXor) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(0b1100));
  (void)stack.push(Uint256(0b1010));

  const auto result = xor_(stack, gas, GAS_XOR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0b0110));  // 1010 ^ 1100 = 0110
  EXPECT_EQ(gas, 1000 - GAS_XOR);
}

TEST(OpcodeXor, XorWithSelf) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0x12345678));
  (void)stack.push(Uint256(0x12345678));

  const auto result = xor_(stack, gas, GAS_XOR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // x ^ x = 0
}

// =============================================================================
// NOT Opcode Tests
// =============================================================================

TEST(OpcodeNot, NotZero) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(ZERO);

  const auto result = not_(stack, gas, GAS_NOT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256::max());
  EXPECT_EQ(gas, 1000 - GAS_NOT);
}

TEST(OpcodeNot, NotMax) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256::max());

  const auto result = not_(stack, gas, GAS_NOT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

TEST(OpcodeNot, DoubleNot) {
  Stack stack;
  uint64_t gas = 100;
  const auto original = Uint256(0x12345678);
  (void)stack.push(original);

  not_(stack, gas, GAS_NOT);
  not_(stack, gas, GAS_NOT);

  EXPECT_EQ(stack[0], original);  // ~~x = x
}

// =============================================================================
// BYTE Opcode Tests
// =============================================================================

TEST(OpcodeByte, ExtractByte0) {
  Stack stack;
  uint64_t gas = 1000;
  // Create value with 0xAB in most significant byte (byte 0 in EVM)
  const auto value = Uint256(0, 0, 0, 0xAB00000000000000ULL);
  (void)stack.push(value);
  (void)stack.push(Uint256(0));  // byte index 0

  const auto result = byte(stack, gas, GAS_BYTE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xAB));
  EXPECT_EQ(gas, 1000 - GAS_BYTE);
}

TEST(OpcodeByte, ExtractByte31) {
  Stack stack;
  uint64_t gas = 100;
  // Least significant byte (byte 31 in EVM)
  (void)stack.push(Uint256(0xCD));
  (void)stack.push(Uint256(31));

  const auto result = byte(stack, gas, GAS_BYTE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xCD));
}

TEST(OpcodeByte, IndexOutOfRange) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0xFF));
  (void)stack.push(Uint256(32));  // Out of range

  const auto result = byte(stack, gas, GAS_BYTE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

TEST(OpcodeByte, LargeIndex) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0xFF));
  (void)stack.push(Uint256::max());  // Very large index

  const auto result = byte(stack, gas, GAS_BYTE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

// =============================================================================
// SHL Opcode Tests
// =============================================================================

TEST(OpcodeShl, ShiftByOne) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(1));  // value
  (void)stack.push(Uint256(1));  // shift

  const auto result = shl(stack, gas, GAS_SHL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(2));
  EXPECT_EQ(gas, 1000 - GAS_SHL);
}

TEST(OpcodeShl, ShiftByZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0x12345678));
  (void)stack.push(ZERO);

  const auto result = shl(stack, gas, GAS_SHL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0x12345678));
}

TEST(OpcodeShl, ShiftBy256) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(256));

  const auto result = shl(stack, gas, GAS_SHL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // Shifted completely out
}

TEST(OpcodeShl, LargeShift) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(1000));

  const auto result = shl(stack, gas, GAS_SHL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

// =============================================================================
// SHR Opcode Tests
// =============================================================================

TEST(OpcodeShr, ShiftByOne) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(2));  // value
  (void)stack.push(Uint256(1));  // shift

  const auto result = shr(stack, gas, GAS_SHR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(1));
  EXPECT_EQ(gas, 1000 - GAS_SHR);
}

TEST(OpcodeShr, ShiftByZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0x12345678));
  (void)stack.push(ZERO);

  const auto result = shr(stack, gas, GAS_SHR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0x12345678));
}

TEST(OpcodeShr, ShiftBy256) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256::max());
  (void)stack.push(Uint256(256));

  const auto result = shr(stack, gas, GAS_SHR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

// =============================================================================
// SAR (Arithmetic Shift Right) Opcode Tests
// =============================================================================

TEST(OpcodeSar, PositiveValueShift) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(8));  // positive value
  (void)stack.push(Uint256(1));  // shift by 1

  const auto result = sar(stack, gas, GAS_SAR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(4));  // 8 >> 1 = 4
  EXPECT_EQ(gas, 1000 - GAS_SAR);
}

TEST(OpcodeSar, NegativeValueShift) {
  Stack stack;
  uint64_t gas = 100;
  // -1 in two's complement (all bits set)
  (void)stack.push(Uint256::max());
  (void)stack.push(Uint256(1));

  const auto result = sar(stack, gas, GAS_SAR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256::max());  // -1 >> 1 = -1
}

TEST(OpcodeSar, NegativeByLargeShift) {
  Stack stack;
  uint64_t gas = 100;
  // High bit set (negative)
  const auto neg = Uint256(0, 0, 0, 0x8000000000000000ULL);
  (void)stack.push(neg);
  (void)stack.push(Uint256(256));

  const auto result = sar(stack, gas, GAS_SAR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256::max());  // Fills with 1s
}

TEST(OpcodeSar, PositiveByLargeShift) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0x7FFFFFFF));  // positive
  (void)stack.push(Uint256(256));

  const auto result = sar(stack, gas, GAS_SAR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // Fills with 0s
}

// =============================================================================
// SLT (Signed Less Than) Opcode Tests
// =============================================================================

TEST(OpcodeSlt, PositiveLessThanPositive) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(10));  // b
  (void)stack.push(Uint256(5));   // a (top)

  const auto result = slt(stack, gas, GAS_SLT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 5 < 10 (signed)
  EXPECT_EQ(gas, 1000 - GAS_SLT);
}

TEST(OpcodeSlt, NegativeLessThanPositive) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));      // b: positive 1
  (void)stack.push(Uint256::max());  // a: -1 (top)

  const auto result = slt(stack, gas, GAS_SLT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // -1 < 1 (signed)
}

TEST(OpcodeSlt, PositiveNotLessThanNegative) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256::max());  // b: -1
  (void)stack.push(Uint256(1));      // a: positive 1 (top)

  const auto result = slt(stack, gas, GAS_SLT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 1 not < -1 (signed)
}

TEST(OpcodeSlt, SameValueNotLess) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256::max());
  (void)stack.push(Uint256::max());

  const auto result = slt(stack, gas, GAS_SLT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // -1 not < -1
}

// =============================================================================
// SGT (Signed Greater Than) Opcode Tests
// =============================================================================

TEST(OpcodeSgt, PositiveGreaterThanNegative) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256::max());  // b: -1
  (void)stack.push(Uint256(1));      // a: 1 (top)

  const auto result = sgt(stack, gas, GAS_SGT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 1 > -1 (signed)
  EXPECT_EQ(gas, 1000 - GAS_SGT);
}

TEST(OpcodeSgt, NegativeNotGreaterThanPositive) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));      // b: 1
  (void)stack.push(Uint256::max());  // a: -1 (top)

  const auto result = sgt(stack, gas, GAS_SGT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // -1 not > 1 (signed)
}

TEST(OpcodeSgt, LargerPositiveGreater) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(10));

  const auto result = sgt(stack, gas, GAS_SGT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 10 > 5
}

// =============================================================================
// Gas and Stack Error Tests
// =============================================================================

TEST(OpcodeBitwise, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(2));

  const auto result = and_(stack, gas, GAS_AND);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeBitwise, StackUnderflowNot) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = not_(stack, gas, GAS_NOT);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

}  // namespace div0::evm::opcodes
