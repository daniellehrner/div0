// Unit tests for comparison opcodes: LT, GT, EQ, ISZERO

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

// clang-format off
// must not be ordered alphabetically, but stay in this order
#include "../src/evm/opcodes/op_helpers.h"
#include "../src/evm/opcodes/comparison.h"
// clang-format on

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_LT = 3;
constexpr uint64_t GAS_GT = 3;
constexpr uint64_t GAS_EQ = 3;
constexpr uint64_t GAS_ISZERO = 3;

// Maximum uint64 value for multi-limb tests
constexpr uint64_t MAX64 = UINT64_MAX;

// Result values
constexpr auto ONE = Uint256(1);
constexpr auto ZERO = Uint256(0);

// =============================================================================
// LT (Less Than) Opcode Tests
// =============================================================================

TEST(OpcodeLt, TrueWhenLessThan) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(10));  // b (stack[1])
  (void)stack.push(Uint256(5));   // a (stack[0], top)

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // EVM LT: a < b where a=top, b=second
  EXPECT_EQ(stack[0], ONE);  // 5 < 10 is true
  EXPECT_EQ(gas, 1000 - GAS_LT);
}

TEST(OpcodeLt, FalseWhenGreater) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(10));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 10 < 5 is false
}

TEST(OpcodeLt, FalseWhenEqual) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(42));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 42 < 42 is false
}

TEST(OpcodeLt, ZeroLessThanOne) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));
  (void)stack.push(Uint256(0));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 0 < 1 is true
}

TEST(OpcodeLt, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE_A = Uint256(MAX64, MAX64, 0, 0);
  constexpr auto LARGE_B = Uint256(0, 0, 1, 0);  // 2^128
  (void)stack.push(LARGE_B);
  (void)stack.push(LARGE_A);

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // Values < 2^128 are less than 2^128
}

TEST(OpcodeLt, MaxValueComparison) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(max_value);
  (void)stack.push(Uint256(0));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 0 < MAX is true
}

TEST(OpcodeLt, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeLt, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(10));
  (void)stack.push(Uint256(5));

  const auto result = lt(stack, gas, GAS_LT);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// GT (Greater Than) Opcode Tests
// =============================================================================

TEST(OpcodeGt, TrueWhenGreater) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(5));   // b
  (void)stack.push(Uint256(10));  // a (top)

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // EVM GT: a > b where a=top, b=second
  EXPECT_EQ(stack[0], ONE);  // 10 > 5 is true
  EXPECT_EQ(gas, 1000 - GAS_GT);
}

TEST(OpcodeGt, FalseWhenLessThan) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(10));
  (void)stack.push(Uint256(5));

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 5 > 10 is false
}

TEST(OpcodeGt, FalseWhenEqual) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(42));

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 42 > 42 is false
}

TEST(OpcodeGt, OneGreaterThanZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(1));

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 1 > 0 is true
}

TEST(OpcodeGt, LargeValues) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto LARGE_A = Uint256(0, 0, 1, 0);  // 2^128
  constexpr auto LARGE_B = Uint256(MAX64, MAX64, 0, 0);
  (void)stack.push(LARGE_B);
  (void)stack.push(LARGE_A);

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // 2^128 > values < 2^128
}

TEST(OpcodeGt, MaxValueComparison) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(Uint256(0));
  (void)stack.push(max_value);

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);  // MAX > 0 is true
}

TEST(OpcodeGt, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeGt, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(5));
  (void)stack.push(Uint256(10));

  const auto result = gt(stack, gas, GAS_GT);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// EQ (Equality) Opcode Tests
// =============================================================================

TEST(OpcodeEq, TrueWhenEqual) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(42));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], ONE);  // 42 == 42 is true
  EXPECT_EQ(gas, 1000 - GAS_EQ);
}

TEST(OpcodeEq, FalseWhenNotEqual) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(100));
  (void)stack.push(Uint256(42));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 42 == 100 is false
}

TEST(OpcodeEq, ZeroEqualsZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(0));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);
}

TEST(OpcodeEq, MaxEqualsMax) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(max_value);
  (void)stack.push(max_value);

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);
}

TEST(OpcodeEq, DifferentLimbsSameValue) {
  Stack stack;
  uint64_t gas = 100;
  // Both represent 2^64
  constexpr auto A = Uint256(0, 1, 0, 0);
  constexpr auto B = Uint256(0, 1, 0, 0);
  (void)stack.push(A);
  (void)stack.push(B);

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);
}

TEST(OpcodeEq, DifferentByOneBit) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(0));
  (void)stack.push(Uint256(1));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // 1 != 0
}

TEST(OpcodeEq, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(5));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeEq, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(42));

  const auto result = eq(stack, gas, GAS_EQ);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// ISZERO Opcode Tests
// =============================================================================

TEST(OpcodeIsZero, TrueWhenZero) {
  Stack stack;
  uint64_t gas = 1000;
  (void)stack.push(Uint256(0));

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], ONE);  // ISZERO(0) = 1
  EXPECT_EQ(gas, 1000 - GAS_ISZERO);
}

TEST(OpcodeIsZero, FalseWhenNonZero) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(42));

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // ISZERO(42) = 0
}

TEST(OpcodeIsZero, FalseWhenOne) {
  Stack stack;
  uint64_t gas = 100;
  (void)stack.push(Uint256(1));

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);  // ISZERO(1) = 0
}

TEST(OpcodeIsZero, FalseWhenMax) {
  Stack stack;
  uint64_t gas = 100;
  const auto max_value = Uint256::max();
  (void)stack.push(max_value);

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

TEST(OpcodeIsZero, FalseWhenHighLimbOnly) {
  Stack stack;
  uint64_t gas = 100;
  // Value with only high limb set
  constexpr auto HIGH_ONLY = Uint256(0, 0, 0, 1);
  (void)stack.push(HIGH_ONLY);

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZERO);
}

TEST(OpcodeIsZero, AllZeroLimbs) {
  Stack stack;
  uint64_t gas = 100;
  constexpr auto ALL_ZERO = Uint256(0, 0, 0, 0);
  (void)stack.push(ALL_ZERO);

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ONE);
}

TEST(OpcodeIsZero, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeIsZero, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;
  (void)stack.push(Uint256(0));

  const auto result = is_zero(stack, gas, GAS_ISZERO);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Combined Comparison Properties
// =============================================================================

TEST(OpcodeComparison, LtGtMutuallyExclusive) {
  // For any a != b, exactly one of LT or GT is true
  constexpr auto A = Uint256(100);
  constexpr auto B = Uint256(200);

  Stack stack_lt;
  uint64_t gas1 = 100;
  (void)stack_lt.push(B);
  (void)stack_lt.push(A);
  lt(stack_lt, gas1, GAS_LT);

  Stack stack_gt;
  uint64_t gas2 = 100;
  (void)stack_gt.push(B);
  (void)stack_gt.push(A);
  gt(stack_gt, gas2, GAS_GT);

  // 100 < 200 is true, 100 > 200 is false
  EXPECT_EQ(stack_lt[0], ONE);
  EXPECT_EQ(stack_gt[0], ZERO);
}

TEST(OpcodeComparison, EqImpliesNotLtNotGt) {
  // If a == b, then NOT(a < b) AND NOT(a > b)
  constexpr auto VALUE = Uint256(42);

  Stack stack_eq;
  uint64_t gas1 = 100;
  (void)stack_eq.push(VALUE);
  (void)stack_eq.push(VALUE);
  eq(stack_eq, gas1, GAS_EQ);

  Stack stack_lt;
  uint64_t gas2 = 100;
  (void)stack_lt.push(VALUE);
  (void)stack_lt.push(VALUE);
  lt(stack_lt, gas2, GAS_LT);

  Stack stack_gt;
  uint64_t gas3 = 100;
  (void)stack_gt.push(VALUE);
  (void)stack_gt.push(VALUE);
  gt(stack_gt, gas3, GAS_GT);

  EXPECT_EQ(stack_eq[0], ONE);   // Equal
  EXPECT_EQ(stack_lt[0], ZERO);  // Not less than
  EXPECT_EQ(stack_gt[0], ZERO);  // Not greater than
}

TEST(OpcodeComparison, IsZeroEquivalentToEqZero) {
  // ISZERO(x) == EQ(x, 0)
  constexpr auto VALUE = Uint256(0);

  Stack stack_iszero;
  uint64_t gas1 = 100;
  (void)stack_iszero.push(VALUE);
  is_zero(stack_iszero, gas1, GAS_ISZERO);

  Stack stack_eq;
  uint64_t gas2 = 100;
  (void)stack_eq.push(Uint256(0));
  (void)stack_eq.push(VALUE);
  eq(stack_eq, gas2, GAS_EQ);

  EXPECT_EQ(stack_iszero[0], stack_eq[0]);
}

}  // namespace div0::evm::opcodes
