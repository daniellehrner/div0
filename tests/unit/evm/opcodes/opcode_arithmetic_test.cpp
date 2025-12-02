// Unit tests for arithmetic opcodes: ADD, SUB, MUL, DIV, MOD

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include "../src/evm/opcodes/arithmetic.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_ADD = 3;
constexpr uint64_t GAS_MUL = 5;
constexpr uint64_t GAS_SUB = 3;
constexpr uint64_t GAS_DIV = 5;
constexpr uint64_t GAS_MOD = 5;

// Maximum uint64 value for multi-limb tests
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// ADD Opcode Tests
// =============================================================================

TEST(OpcodeAdd, BasicAddition) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(3));  // stack[1] after second push
  (void)stack.push(Uint256(5));  // stack[0] (top)

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(8));  // 5 + 3 = 8
  EXPECT_EQ(gas, 1000 - GAS_ADD);
}

TEST(OpcodeAdd, AddZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(0));

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(42));
}

TEST(OpcodeAdd, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE_A = Uint256(MAX64, MAX64, 0, 0);
  constexpr auto LARGE_B = Uint256(1, 0, 0, 0);
  (void)stack.push(LARGE_B);
  (void)stack.push(LARGE_A);

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // MAX64 + 1 in first limb carries to second, then MAX64 + 1 in second carries to third
  EXPECT_EQ(stack[0], Uint256(0, 0, 1, 0));
}

TEST(OpcodeAdd, OverflowWraps) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(Uint256(1));
  (void)stack.push(max_value);

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));  // Wraps to 0
}

TEST(OpcodeAdd, StackUnderflowEmpty) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 100);  // Gas unchanged on error
}

TEST(OpcodeAdd, StackUnderflowOneElement) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 100);
}

TEST(OpcodeAdd, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;  // Less than GAS_ADD (3)
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(5));

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(gas, 2);  // Gas unchanged on error
}

TEST(OpcodeAdd, ExactGas) {
  Stack stack;
  uint64_t gas = GAS_ADD;  // Exactly enough
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(5));

  const auto result = add(stack, gas, GAS_ADD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 0);
}

// =============================================================================
// SUB Opcode Tests
// =============================================================================

TEST(OpcodeSub, BasicSubtraction) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(3));  // stack[1]: b
  (void)stack.push(Uint256(5));  // stack[0]: a (top)

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // EVM SUB: a - b where a=top, b=second
  EXPECT_EQ(stack[0], Uint256(2));  // 5 - 3 = 2
  EXPECT_EQ(gas, 1000 - GAS_SUB);
}

TEST(OpcodeSub, SubtractZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(42));

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(42));
}

TEST(OpcodeSub, SubtractFromZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(0));

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // 0 - 1 wraps to MAX
  EXPECT_EQ(stack[0], Uint256::max());
}

TEST(OpcodeSub, UnderflowWraps) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(10));
  (void)stack.push(Uint256(5));

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // 5 - 10 wraps (underflow)
  EXPECT_EQ(stack[0], Uint256::max() - Uint256(4));
}

TEST(OpcodeSub, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE = Uint256(0, 0, 1, 0);
  (void)stack.push(Uint256(1));
  (void)stack.push(LARGE);

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(MAX64, MAX64, 0, 0));
}

TEST(OpcodeSub, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeSub, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(5));

  const auto result = sub(stack, gas, GAS_SUB);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// MUL Opcode Tests
// =============================================================================

TEST(OpcodeMul, BasicMultiplication) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(5));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(15));
  EXPECT_EQ(gas, 1000 - GAS_MUL);
}

TEST(OpcodeMul, MultiplyByZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(12345));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeMul, MultiplyByOne) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(42));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(42));
}

TEST(OpcodeMul, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  // 2^64 * 2 = 2^65
  (void)stack.push(Uint256(2));
  (void)stack.push(Uint256(0, 1, 0, 0));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0, 2, 0, 0));
}

TEST(OpcodeMul, OverflowWraps) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE = Uint256(0, 0, 0, 1ULL << 63);  // 2^255
  (void)stack.push(Uint256(2));
  (void)stack.push(LARGE);

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // 2^255 * 2 = 2^256 wraps to 0
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeMul, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeMul, OutOfGas) {
  Stack stack;
  uint64_t gas = 4;  // Less than GAS_MUL (5)
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(5));

  const auto result = mul(stack, gas, GAS_MUL);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// DIV Opcode Tests
// =============================================================================

TEST(OpcodeDiv, BasicDivision) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(3));   // divisor
  (void)stack.push(Uint256(15));  // dividend (top)

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // EVM DIV: a / b where a=top, b=second
  EXPECT_EQ(stack[0], Uint256(5));  // 15 / 3 = 5
  EXPECT_EQ(gas, 1000 - GAS_DIV);
}

TEST(OpcodeDiv, DivisionByZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));    // divisor = 0
  (void)stack.push(Uint256(100));  // dividend

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // EVM spec: division by zero returns 0
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeDiv, DivideByOne) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(42));

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(42));
}

TEST(OpcodeDiv, DivideZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(0));

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeDiv, IntegerDivisionTruncates) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(10));

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(3));  // 10 / 3 = 3 (truncated)
}

TEST(OpcodeDiv, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE = Uint256(0, 2, 0, 0);  // 2^65
  (void)stack.push(Uint256(2));
  (void)stack.push(LARGE);

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0, 1, 0, 0));  // 2^64
}

TEST(OpcodeDiv, MaxDividedByMax) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(max_value);
  (void)stack.push(max_value);

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(1));
}

TEST(OpcodeDiv, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeDiv, OutOfGas) {
  Stack stack;
  uint64_t gas = 4;
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(15));

  const auto result = div(stack, gas, GAS_DIV);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// MOD Opcode Tests
// =============================================================================

TEST(OpcodeMod, BasicModulo) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(3));   // divisor
  (void)stack.push(Uint256(10));  // dividend (top)

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // EVM MOD: a % b where a=top, b=second
  EXPECT_EQ(stack[0], Uint256(1));  // 10 % 3 = 1
  EXPECT_EQ(gas, 1000 - GAS_MOD);
}

TEST(OpcodeMod, ModuloByZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(100));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // EVM spec: modulo by zero returns 0
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeMod, ModuloByOne) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(42));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));  // Any number % 1 = 0
}

TEST(OpcodeMod, ModuloZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(0));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodeMod, ModuloLargerThanDividend) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(100));
  (void)stack.push(Uint256(42));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(42));  // 42 % 100 = 42
}

TEST(OpcodeMod, ExactDivisor) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(15));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));  // 15 % 5 = 0
}

TEST(OpcodeMod, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE = Uint256(0, 1, 0, 0);  // 2^64
  (void)stack.push(Uint256(MAX64));
  (void)stack.push(LARGE);

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(1));  // 2^64 % (2^64 - 1) = 1
}

TEST(OpcodeMod, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeMod, OutOfGas) {
  Stack stack;
  uint64_t gas = 4;
  (void)stack.push(Uint256(3));
  (void)stack.push(Uint256(10));

  const auto result = mod(stack, gas, GAS_MOD);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Combined Arithmetic Properties
// =============================================================================

TEST(OpcodeArithmetic, AddSubInverse) {
  // (a + b) - b == a
  Stack stack;
  uint64_t gas = 100;
  constexpr auto A = Uint256(12345);
  constexpr auto B = Uint256(67890);

  (void)stack.push(B);
  (void)stack.push(A);
  add(stack, gas, GAS_ADD);  // stack[0] = A + B

  (void)stack.push(B);
  std::swap(stack[0], stack[1]);  // Put B on top
  sub(stack, gas, GAS_SUB);       // (A + B) - B

  EXPECT_EQ(stack[0], A);
}

TEST(OpcodeArithmetic, MulDivInverse) {
  // (a * b) / b == a (when b != 0)
  Stack stack;
  uint64_t gas = 100;
  constexpr auto A = Uint256(123);
  constexpr auto B = Uint256(456);

  (void)stack.push(B);
  (void)stack.push(A);
  mul(stack, gas, GAS_MUL);  // stack[0] = A * B

  (void)stack.push(B);
  std::swap(stack[0], stack[1]);  // Put B on top
  div(stack, gas, GAS_DIV);       // (A * B) / B

  EXPECT_EQ(stack[0], A);
}

TEST(OpcodeArithmetic, DivModRelationship) {
  // a == (a / b) * b + (a % b) (when b != 0)
  constexpr auto A = Uint256(12345);
  constexpr auto B = Uint256(100);

  Stack stack_div;
  uint64_t gas1 = 100;
  (void)stack_div.push(B);
  (void)stack_div.push(A);
  div(stack_div, gas1, GAS_DIV);
  const auto quotient = stack_div[0];

  Stack stack_mod;
  uint64_t gas2 = 100;
  (void)stack_mod.push(B);
  (void)stack_mod.push(A);
  mod(stack_mod, gas2, GAS_MOD);
  const auto remainder = stack_mod[0];

  // Verify: quotient * B + remainder == A
  EXPECT_EQ(quotient * B + remainder, A);
}

}  // namespace div0::evm::opcodes
