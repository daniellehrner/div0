// Unit tests for dup opcodes: DUP1-DUP16

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include <string>

#include "../src/evm/opcodes/dup.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas cost from EVM specification
constexpr uint64_t GAS_DUP = 3;

// =============================================================================
// DUP1 Opcode Tests
// =============================================================================

TEST(OpcodeDup1, DuplicatesTopElement) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));

  uint64_t gas = 1000;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack[0], Uint256(100));  // New top (copy)
  EXPECT_EQ(stack[1], Uint256(100));  // Original
  EXPECT_EQ(gas, 1000 - GAS_DUP);
}

TEST(OpcodeDup1, ConsumesGasOnSuccess) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 100 - GAS_DUP);
}

TEST(OpcodeDup1, OutOfGas) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 2;  // Less than GAS_DUP (3)
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(gas, 2);           // Unchanged
  EXPECT_EQ(stack.size(), 1);  // Stack unchanged
}

TEST(OpcodeDup1, StackUnderflowWithEmptyStack) {
  Stack stack;

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 100);  // Gas NOT consumed on error
  EXPECT_TRUE(stack.empty());
}

TEST(OpcodeDup1, StackOverflowAtMaxCapacity) {
  Stack stack;

  // Fill stack to capacity
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(gas, 100);                       // Gas NOT consumed on error
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);  // Stack unchanged
}

// =============================================================================
// DUP2 Opcode Tests
// =============================================================================

TEST(OpcodeDup2, DuplicatesSecondElement) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  uint64_t gas = 1000;
  const auto result = dup_n(stack, gas, GAS_DUP, 2);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], Uint256(100));  // Copy of second element
  EXPECT_EQ(stack[1], Uint256(200));  // Original top
  EXPECT_EQ(stack[2], Uint256(100));  // Original second
}

TEST(OpcodeDup2, StackUnderflowWithOneElement) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 2);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

// =============================================================================
// DUP16 Opcode Tests (Maximum Depth)
// =============================================================================

TEST(OpcodeDup16, DuplicatesSixteenthElement) {
  Stack stack;

  // Push 16 elements
  for (size_t i = 0; i < 16; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i * 100)));
  }
  // Stack: [0, 100, 200, ..., 1500] with 1500 on top

  uint64_t gas = 1000;
  const auto result = dup_n(stack, gas, GAS_DUP, 16);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 17);
  EXPECT_EQ(stack[0], Uint256(0));     // Copy of 16th element (bottom)
  EXPECT_EQ(stack[1], Uint256(1500));  // Original top
  EXPECT_EQ(gas, 1000 - GAS_DUP);
}

TEST(OpcodeDup16, StackUnderflowWithFifteenElements) {
  Stack stack;

  // Push only 15 elements (need 16 for DUP16)
  for (size_t i = 0; i < 15; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 16);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

// =============================================================================
// All DUP Depths (DUP1-DUP16)
// =============================================================================

TEST(OpcodeDupAll, AllDepthsSucceed) {
  for (size_t depth = 1; depth <= 16; ++depth) {
    SCOPED_TRACE("depth = " + std::to_string(depth));
    Stack stack;

    // Push exactly depth elements
    for (size_t i = 0; i < depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i * 10)));
    }

    const auto target_value = stack[depth - 1];  // Element at depth (0-indexed)
    const auto original_size = stack.size();

    uint64_t gas = 100;
    const auto result = dup_n(stack, gas, GAS_DUP, depth);

    EXPECT_EQ(result, ExecutionStatus::Success);
    EXPECT_EQ(stack.size(), original_size + 1);
    EXPECT_EQ(stack[0], target_value);  // New top is copy of target
    EXPECT_EQ(gas, 100 - GAS_DUP);
  }
}

TEST(OpcodeDupAll, AllDepthsUnderflowWithInsufficientElements) {
  for (size_t depth = 1; depth <= 16; ++depth) {
    SCOPED_TRACE("depth = " + std::to_string(depth));
    Stack stack;

    // Push depth - 1 elements (one less than needed)
    for (size_t i = 0; i + 1 < depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i)));
    }

    uint64_t gas = 100;
    const auto result = dup_n(stack, gas, GAS_DUP, depth);

    EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  }
}

// =============================================================================
// Gas Consumption Tests
// =============================================================================

TEST(OpcodeDupGas, ExactGasConsumption) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));

  uint64_t gas = GAS_DUP;  // Exactly enough
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 0);
}

TEST(OpcodeDupGas, MultipleDupsGas) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 1);  // Stack: [1, 1]
  dup_n(stack, gas, GAS_DUP, 2);  // Stack: [1, 1, 1]
  dup_n(stack, gas, GAS_DUP, 1);  // Stack: [1, 1, 1, 1]

  EXPECT_EQ(gas, 100 - (3 * GAS_DUP));
  EXPECT_EQ(stack.size(), 4);
}

TEST(OpcodeDupGas, ZeroGasFails) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));

  uint64_t gas = 0;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Stack Integrity Tests
// =============================================================================

TEST(OpcodeDupIntegrity, IncreasesStackSizeByOne) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  const auto size_before = stack.size();
  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 2);

  EXPECT_EQ(stack.size(), size_before + 1);
}

TEST(OpcodeDupIntegrity, PreservesAllExistingElements) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 2);  // Duplicate second element (200)

  // All original elements preserved (shifted down by 1)
  EXPECT_EQ(stack[0], Uint256(200));  // New copy
  EXPECT_EQ(stack[1], Uint256(300));  // Original top
  EXPECT_EQ(stack[2], Uint256(200));  // Original second
  EXPECT_EQ(stack[3], Uint256(100));  // Original third
}

TEST(OpcodeDupIntegrity, DoubleDupCreatesMultipleCopies) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 1);
  dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], Uint256(42));
  EXPECT_EQ(stack[1], Uint256(42));
  EXPECT_EQ(stack[2], Uint256(42));
}

// =============================================================================
// Large Value Tests
// =============================================================================

TEST(OpcodeDupLargeValues, PreservesFullUint256) {
  Stack stack;
  const auto large_value = Uint256(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL,
                                   0xFEDCBA0987654321ULL, 0x1111222233334444ULL);

  ASSERT_TRUE(stack.push(large_value));

  uint64_t gas = 100;
  const auto result = dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], large_value);
  EXPECT_EQ(stack[1], large_value);
}

TEST(OpcodeDupLargeValues, DupMaxValue) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256::max()));

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(stack[0], Uint256::max());
  EXPECT_EQ(stack[1], Uint256::max());
}

// =============================================================================
// Stack Overflow Edge Cases
// =============================================================================

TEST(OpcodeDupOverflow, FailsWhenStackAlmostFull) {
  Stack stack;

  // Fill stack to MAX_SIZE - 1 (room for one more)
  for (size_t i = 0; i + 1 < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  // This should succeed (fills to exactly MAX_SIZE)
  uint64_t gas = 100;
  auto result = dup_n(stack, gas, GAS_DUP, 1);
  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);

  // This should fail with StackOverflow (would exceed MAX_SIZE)
  result = dup_n(stack, gas, GAS_DUP, 1);
  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);  // Unchanged
}

// =============================================================================
// Distinct Error Type Tests
// =============================================================================

TEST(OpcodeDupErrors, UnderflowVsOverflowDistinct) {
  // Test that underflow and overflow return different error codes

  // Underflow: empty stack, try to dup
  Stack empty_stack;
  uint64_t gas = 100;
  auto result = dup_n(empty_stack, gas, GAS_DUP, 1);
  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);

  // Overflow: full stack, try to dup
  Stack full_stack;
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(full_stack.push(Uint256(i)));
  }
  gas = 100;
  result = dup_n(full_stack, gas, GAS_DUP, 1);
  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
}

TEST(OpcodeDupErrors, GasNotConsumedOnUnderflow) {
  Stack stack;  // Empty

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(gas, 100);  // Unchanged
}

TEST(OpcodeDupErrors, GasNotConsumedOnOverflow) {
  Stack stack;
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  uint64_t gas = 100;
  dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(gas, 100);  // Unchanged
}

TEST(OpcodeDupErrors, GasNotConsumedOnOutOfGas) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 1;  // Not enough
  dup_n(stack, gas, GAS_DUP, 1);

  EXPECT_EQ(gas, 1);  // Unchanged
}

TEST(OpcodeDupErrors, ErrorPriorityGasBeforeStack) {
  // When both out of gas AND stack error would occur,
  // out of gas should be returned first
  Stack empty_stack;

  uint64_t gas = 1;  // Not enough gas
  const auto result = dup_n(empty_stack, gas, GAS_DUP, 1);

  // Gas check happens first
  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeDupErrors, ErrorPriorityUnderflowBeforeOverflow) {
  // Edge case: stack has MAX_SIZE elements but we try DUP with depth > size
  // This shouldn't happen in practice, but tests check order
  Stack stack;

  // Push only 5 elements
  for (size_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  uint64_t gas = 100;
  // DUP16 needs 16 elements, we have 5 -> underflow checked first
  const auto result = dup_n(stack, gas, GAS_DUP, 16);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

}  // namespace div0::evm::opcodes
