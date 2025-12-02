// Unit tests for swap opcodes: SWAP1-SWAP16

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include <string>

#include "../src/evm/opcodes/swap.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas cost from EVM specification
constexpr uint64_t GAS_SWAP = 3;

// =============================================================================
// SWAP1 Opcode Tests
// =============================================================================

TEST(OpcodeSwap1, SwapsTopTwoElements) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  uint64_t gas = 1000;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack[0], Uint256(100));  // Was second, now top
  EXPECT_EQ(stack[1], Uint256(200));  // Was top, now second
  EXPECT_EQ(gas, 1000 - GAS_SWAP);
}

TEST(OpcodeSwap1, ConsumesGasOnSuccess) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 100 - GAS_SWAP);
}

TEST(OpcodeSwap1, OutOfGas) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));

  uint64_t gas = 2;  // Less than GAS_SWAP (3)
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(gas, 2);  // Unchanged
  // Stack should be unchanged
  EXPECT_EQ(stack[0], Uint256(2));
  EXPECT_EQ(stack[1], Uint256(1));
}

TEST(OpcodeSwap1, StackUnderflowWithOneElement) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  // Gas is NOT consumed on error
  EXPECT_EQ(gas, 100);
  EXPECT_EQ(stack[0], Uint256(42));  // Unchanged
}

TEST(OpcodeSwap1, StackUnderflowWithEmptyStack) {
  Stack stack;

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_TRUE(stack.empty());
}

// =============================================================================
// SWAP2 Opcode Tests
// =============================================================================

TEST(OpcodeSwap2, SwapsTopAndThird) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  uint64_t gas = 1000;
  const auto result = swap_n(stack, gas, GAS_SWAP, 2);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(100));  // Was third, now top
  EXPECT_EQ(stack[1], Uint256(200));  // Unchanged (middle)
  EXPECT_EQ(stack[2], Uint256(300));  // Was top, now third
}

TEST(OpcodeSwap2, StackUnderflowWithTwoElements) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 2);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

// =============================================================================
// SWAP16 Opcode Tests (Maximum Depth)
// =============================================================================

TEST(OpcodeSwap16, SwapsTopAndSeventeenth) {
  Stack stack;

  // Push 17 elements (need depth + 1 for SWAP16)
  for (size_t i = 0; i < 17; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i * 100)));
  }
  // Stack: [0, 100, 200, ..., 1600] with 1600 on top

  uint64_t gas = 1000;
  const auto result = swap_n(stack, gas, GAS_SWAP, 16);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));      // Was at depth 16, now top
  EXPECT_EQ(stack[16], Uint256(1600));  // Was top, now at depth 16
  EXPECT_EQ(gas, 1000 - GAS_SWAP);
}

TEST(OpcodeSwap16, StackUnderflowWithSixteenElements) {
  Stack stack;

  // Push only 16 elements (need 17 for SWAP16)
  for (size_t i = 0; i < 16; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 16);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

// =============================================================================
// All SWAP Depths (SWAP1-SWAP16)
// =============================================================================

TEST(OpcodeSwapAll, AllDepthsSucceed) {
  for (size_t depth = 1; depth <= 16; ++depth) {
    SCOPED_TRACE("depth = " + std::to_string(depth));
    Stack stack;

    // Push depth + 1 elements
    for (size_t i = 0; i <= depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i * 10)));
    }

    const auto top_before = stack[0];
    const auto target_before = stack[depth];

    uint64_t gas = 100;
    const auto result = swap_n(stack, gas, GAS_SWAP, depth);

    EXPECT_EQ(result, ExecutionStatus::Success);
    EXPECT_EQ(stack[0], target_before);
    EXPECT_EQ(stack[depth], top_before);
    EXPECT_EQ(gas, 100 - GAS_SWAP);
  }
}

TEST(OpcodeSwapAll, AllDepthsUnderflowWithInsufficientElements) {
  for (size_t depth = 1; depth <= 16; ++depth) {
    SCOPED_TRACE("depth = " + std::to_string(depth));
    Stack stack;

    // Push exactly depth elements (need depth + 1)
    for (size_t i = 0; i < depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i)));
    }

    uint64_t gas = 100;
    const auto result = swap_n(stack, gas, GAS_SWAP, depth);

    EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  }
}

// =============================================================================
// Gas Consumption Tests
// =============================================================================

TEST(OpcodeSwapGas, ExactGasConsumption) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));

  uint64_t gas = GAS_SWAP;  // Exactly enough
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 0);
}

TEST(OpcodeSwapGas, MultipleSwapsGas) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));
  ASSERT_TRUE(stack.push(Uint256(3)));

  uint64_t gas = 100;
  swap_n(stack, gas, GAS_SWAP, 1);
  swap_n(stack, gas, GAS_SWAP, 2);
  swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(gas, 100 - (3 * GAS_SWAP));
}

TEST(OpcodeSwapGas, ZeroGasFails) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));

  uint64_t gas = 0;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Stack Integrity Tests
// =============================================================================

TEST(OpcodeSwapIntegrity, PreservesStackSize) {
  Stack stack;
  for (size_t i = 0; i < 10; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  const auto size_before = stack.size();
  uint64_t gas = 100;
  swap_n(stack, gas, GAS_SWAP, 5);

  EXPECT_EQ(stack.size(), size_before);
}

TEST(OpcodeSwapIntegrity, PreservesMiddleElements) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));  // depth 4
  ASSERT_TRUE(stack.push(Uint256(200)));  // depth 3
  ASSERT_TRUE(stack.push(Uint256(300)));  // depth 2
  ASSERT_TRUE(stack.push(Uint256(400)));  // depth 1
  ASSERT_TRUE(stack.push(Uint256(500)));  // depth 0 (top)

  uint64_t gas = 100;
  swap_n(stack, gas, GAS_SWAP, 4);

  // Middle elements unchanged
  EXPECT_EQ(stack[1], Uint256(400));
  EXPECT_EQ(stack[2], Uint256(300));
  EXPECT_EQ(stack[3], Uint256(200));
  // Swapped elements
  EXPECT_EQ(stack[0], Uint256(100));
  EXPECT_EQ(stack[4], Uint256(500));
}

TEST(OpcodeSwapIntegrity, DoubleSwapRestoresOriginal) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(111)));
  ASSERT_TRUE(stack.push(Uint256(222)));
  ASSERT_TRUE(stack.push(Uint256(333)));

  uint64_t gas = 100;
  swap_n(stack, gas, GAS_SWAP, 2);
  swap_n(stack, gas, GAS_SWAP, 2);

  EXPECT_EQ(stack[0], Uint256(333));
  EXPECT_EQ(stack[1], Uint256(222));
  EXPECT_EQ(stack[2], Uint256(111));
}

// =============================================================================
// Large Value Tests
// =============================================================================

TEST(OpcodeSwapLargeValues, PreservesFullUint256) {
  Stack stack;
  const auto large_a = Uint256::max();
  const auto large_b = Uint256(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0, 0);

  ASSERT_TRUE(stack.push(large_a));
  ASSERT_TRUE(stack.push(large_b));

  uint64_t gas = 100;
  const auto result = swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], large_a);
  EXPECT_EQ(stack[1], large_b);
}

TEST(OpcodeSwapLargeValues, SwapMaxValues) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256::max()));
  ASSERT_TRUE(stack.push(Uint256(0)));

  uint64_t gas = 100;
  swap_n(stack, gas, GAS_SWAP, 1);

  EXPECT_EQ(stack[0], Uint256::max());
  EXPECT_EQ(stack[1], Uint256(0));
}

}  // namespace div0::evm::opcodes
