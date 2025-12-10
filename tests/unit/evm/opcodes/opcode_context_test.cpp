// Unit tests for context opcodes: CALLER

#include <div0/evm/stack.h>
#include <div0/types/address.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include "../src/evm/opcodes/context.h"

namespace div0::evm::opcodes {

using types::Address;
using types::Uint256;

// =============================================================================
// CALLER Opcode Tests
// =============================================================================

TEST(CallerOpcodeTest, PushesCallerAddress) {
  Stack stack;
  uint64_t gas = 100;
  const Address caller_addr{Uint256(0x1234567890ABCDEF)};

  const auto result = caller(stack, gas, 2, caller_addr);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], caller_addr.to_uint256());
  EXPECT_EQ(gas, 98);  // 2 gas consumed
}

TEST(CallerOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;  // Less than the cost of 2
  const Address caller_addr{Uint256(0x1234)};

  const auto result = caller(stack, gas, 2, caller_addr);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 0);  // Nothing pushed
  EXPECT_EQ(gas, 1);           // Gas unchanged
}

TEST(CallerOpcodeTest, ExactGas) {
  Stack stack;
  uint64_t gas = 2;  // Exactly the cost
  const Address caller_addr{Uint256(0x5678)};

  const auto result = caller(stack, gas, 2, caller_addr);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(gas, 0);  // All gas consumed
}

TEST(CallerOpcodeTest, ZeroAddress) {
  Stack stack;
  uint64_t gas = 100;
  const Address zero_addr{Uint256::zero()};

  const auto result = caller(stack, gas, 2, zero_addr);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
}

TEST(CallerOpcodeTest, MaxAddress) {
  Stack stack;
  uint64_t gas = 100;
  // Max 20-byte address (160 bits set): (1 << 160) - 1
  const Uint256 max_addr_value = (Uint256(1) << Uint256(160)) - Uint256(1);
  const Address max_addr{max_addr_value};

  const auto result = caller(stack, gas, 2, max_addr);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_TRUE(stack[0] == max_addr_value);
}

TEST(CallerOpcodeTest, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;
  const Address caller_addr{Uint256(0x1234)};

  // Fill the stack to capacity (1024 elements)
  for (size_t i = 0; i < 1024; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }
  ASSERT_EQ(stack.size(), 1024);

  const auto result = caller(stack, gas, 2, caller_addr);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), 1024);  // Stack unchanged
  // Gas was deducted before stack check
  EXPECT_EQ(gas, 9998);
}

TEST(CallerOpcodeTest, MultipleCallerCalls) {
  Stack stack;
  uint64_t gas = 100;
  const Address caller_addr{Uint256(0xABCD)};

  // Call CALLER multiple times
  EXPECT_EQ(caller(stack, gas, 2, caller_addr), ExecutionStatus::Success);
  EXPECT_EQ(caller(stack, gas, 2, caller_addr), ExecutionStatus::Success);
  EXPECT_EQ(caller(stack, gas, 2, caller_addr), ExecutionStatus::Success);

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], caller_addr.to_uint256());
  EXPECT_EQ(stack[1], caller_addr.to_uint256());
  EXPECT_EQ(stack[2], caller_addr.to_uint256());
  EXPECT_EQ(gas, 94);  // 6 gas consumed (3 * 2)
}

TEST(CallerOpcodeTest, PreservesExistingStack) {
  Stack stack;
  uint64_t gas = 100;
  const Address caller_addr{Uint256(0x9999)};

  // Push some values first
  ASSERT_TRUE(stack.push(Uint256(111)));
  ASSERT_TRUE(stack.push(Uint256(222)));

  const auto result = caller(stack, gas, 2, caller_addr);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], caller_addr.to_uint256());  // Top (newly pushed)
  EXPECT_EQ(stack[1], Uint256(222));              // Previous top
  EXPECT_EQ(stack[2], Uint256(111));              // Previous bottom
}

}  // namespace div0::evm::opcodes
