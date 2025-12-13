// Unit tests for context opcodes: CALLER, PC, GAS, CALLDATALOAD, CALLDATASIZE, CALLDATACOPY

#include <div0/evm/memory.h>
#include <div0/evm/stack.h>
#include <div0/types/address.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include <array>
#include <span>

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
  constexpr Address CALLER_ADDR{Uint256(0x1234567890ABCDEF)};

  const auto result = caller(stack, gas, 2, CALLER_ADDR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], CALLER_ADDR.to_uint256());
  EXPECT_EQ(gas, 98);  // 2 gas consumed
}

TEST(CallerOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;  // Less than the cost of 2
  constexpr Address CALLER_ADDR{Uint256(0x1234)};

  const auto result = caller(stack, gas, 2, CALLER_ADDR);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 0);  // Nothing pushed
  EXPECT_EQ(gas, 1);           // Gas unchanged
}

TEST(CallerOpcodeTest, ExactGas) {
  Stack stack;
  uint64_t gas = 2;  // Exactly the cost
  constexpr Address CALLER_ADDR{Uint256(0x5678)};

  const auto result = caller(stack, gas, 2, CALLER_ADDR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(gas, 0);  // All gas consumed
}

TEST(CallerOpcodeTest, ZeroAddress) {
  Stack stack;
  uint64_t gas = 100;
  constexpr Address ZERO_ADDR{Uint256::zero()};

  const auto result = caller(stack, gas, 2, ZERO_ADDR);

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
  constexpr Address CALLER_ADDR{Uint256(0x1234)};

  // Fill the stack to capacity (1024 elements)
  for (size_t i = 0; i < 1024; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }
  ASSERT_EQ(stack.size(), 1024);

  const auto result = caller(stack, gas, 2, CALLER_ADDR);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), 1024);  // Stack unchanged
  // Gas was deducted before stack check
  EXPECT_EQ(gas, 9998);
}

TEST(CallerOpcodeTest, MultipleCallerCalls) {
  Stack stack;
  uint64_t gas = 100;
  constexpr Address CALLER_ADDR{Uint256(0xABCD)};

  // Call CALLER multiple times
  EXPECT_EQ(caller(stack, gas, 2, CALLER_ADDR), ExecutionStatus::Success);
  EXPECT_EQ(caller(stack, gas, 2, CALLER_ADDR), ExecutionStatus::Success);
  EXPECT_EQ(caller(stack, gas, 2, CALLER_ADDR), ExecutionStatus::Success);

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], CALLER_ADDR.to_uint256());
  EXPECT_EQ(stack[1], CALLER_ADDR.to_uint256());
  EXPECT_EQ(stack[2], CALLER_ADDR.to_uint256());
  EXPECT_EQ(gas, 94);  // 6 gas consumed (3 * 2)
}

TEST(CallerOpcodeTest, PreservesExistingStack) {
  Stack stack;
  uint64_t gas = 100;
  constexpr Address CALLER_ADDR{Uint256(0x9999)};

  // Push some values first
  ASSERT_TRUE(stack.push(Uint256(111)));
  ASSERT_TRUE(stack.push(Uint256(222)));

  const auto result = caller(stack, gas, 2, CALLER_ADDR);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], CALLER_ADDR.to_uint256());  // Top (newly pushed)
  EXPECT_EQ(stack[1], Uint256(222));              // Previous top
  EXPECT_EQ(stack[2], Uint256(111));              // Previous bottom
}

// =============================================================================
// PC Opcode Tests
// =============================================================================

TEST(PCOpcodeTest, PushesProgramCounter) {
  Stack stack;
  uint64_t gas = 100;
  constexpr uint64_t PC_VALUE = 42;

  const auto result = pc(stack, gas, 2, PC_VALUE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(PC_VALUE));
  EXPECT_EQ(gas, 98);  // 2 gas consumed
}

TEST(PCOpcodeTest, PushesZeroPC) {
  Stack stack;
  uint64_t gas = 100;
  constexpr uint64_t PC_VALUE = 0;

  const auto result = pc(stack, gas, 2, PC_VALUE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
  EXPECT_EQ(gas, 98);
}

TEST(PCOpcodeTest, PushesLargePC) {
  Stack stack;
  uint64_t gas = 100;
  constexpr uint64_t PC_VALUE = 0xFFFFFFFFFFFFFFFF;

  const auto result = pc(stack, gas, 2, PC_VALUE);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(PC_VALUE));
  EXPECT_EQ(gas, 98);
}

TEST(PCOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;

  const auto result = pc(stack, gas, 2, 100);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(gas, 1);  // Gas unchanged
}

TEST(PCOpcodeTest, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;

  // Fill the stack to capacity
  for (size_t i = 0; i < 1024; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  const auto result = pc(stack, gas, 2, 100);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), 1024);
}

// =============================================================================
// GAS Opcode Tests
// =============================================================================

TEST(GASOpcodeTest, PushesRemainingGas) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = gas_op(stack, gas, 2);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  // GAS opcode pushes the remaining gas AFTER deducting the opcode cost
  EXPECT_EQ(stack[0], Uint256(98));  // 100 - 2 = 98
  EXPECT_EQ(gas, 98);
}

TEST(GASOpcodeTest, PushesExactGasRemaining) {
  Stack stack;
  uint64_t gas = 2;  // Just enough for the opcode

  const auto result = gas_op(stack, gas, 2);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());  // 2 - 2 = 0
  EXPECT_EQ(gas, 0);
}

TEST(GASOpcodeTest, LargeGasValue) {
  Stack stack;
  uint64_t gas = 1000000000;

  const auto result = gas_op(stack, gas, 2);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(999999998));  // 1000000000 - 2
  EXPECT_EQ(gas, 999999998);
}

TEST(GASOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;  // Less than the cost of 2

  const auto result = gas_op(stack, gas, 2);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(gas, 1);  // Gas unchanged
}

TEST(GASOpcodeTest, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;

  // Fill the stack to capacity
  for (size_t i = 0; i < 1024; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  const auto result = gas_op(stack, gas, 2);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), 1024);
}

TEST(GASOpcodeTest, MultipleCalls) {
  Stack stack;
  uint64_t gas = 100;

  // First call: pushes 98 (100 - 2)
  EXPECT_EQ(gas_op(stack, gas, 2), ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(98));

  // Second call: pushes 96 (98 - 2)
  EXPECT_EQ(gas_op(stack, gas, 2), ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(96));

  // Third call: pushes 94 (96 - 2)
  EXPECT_EQ(gas_op(stack, gas, 2), ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(94));

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(gas, 94);
}

// =============================================================================
// CALLDATALOAD Opcode Tests
// =============================================================================

TEST(CalldataloadOpcodeTest, LoadsFullWord) {
  Stack stack;
  uint64_t gas = 100;
  // 32 bytes of calldata
  std::array<uint8_t, 32> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                  0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                  0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

  // Push offset 0
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(gas, 97);  // 3 gas consumed

  // Verify the loaded word is big-endian
  auto loaded = stack[0];
  // First byte should be 0x01, which is the MSB
  EXPECT_EQ(loaded.limb(3) >> 56, 0x01);
}

TEST(CalldataloadOpcodeTest, LoadsFromOffset) {
  Stack stack;
  uint64_t gas = 100;
  // 64 bytes of calldata
  std::array<uint8_t, 64> data{};
  for (size_t i = 0; i < 64; ++i) {
    data.at(i) = static_cast<uint8_t>(i);
  }

  // Push offset 16
  ASSERT_TRUE(stack.push(Uint256(16)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);

  // Should load bytes 16-47, first byte is 0x10
  auto loaded = stack[0];
  EXPECT_EQ(loaded.limb(3) >> 56, 0x10);
}

TEST(CalldataloadOpcodeTest, ZeroPadsWhenPartialData) {
  Stack stack;
  uint64_t gas = 100;
  // Only 10 bytes of calldata
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Push offset 0 - will load 10 bytes + 22 zeros
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);

  // First byte should be 0xFF, and the lower bytes should be zero-padded
  auto loaded = stack[0];
  // Lower 22 bytes should all be zero
  EXPECT_EQ(loaded.limb(0), 0);
  EXPECT_EQ(loaded.limb(1) & 0xFFFF'FFFF'FFFF, 0);  // Lower 6 bytes of limb1
}

TEST(CalldataloadOpcodeTest, ZeroPadsWhenPartialDataFromMiddle) {
  Stack stack;
  uint64_t gas = 100;
  // 20 bytes of calldata
  std::array<uint8_t, 20> data{};
  for (size_t i = 0; i < 20; ++i) {
    data.at(i) = static_cast<uint8_t>(i + 1);
  }

  // Push offset 10 - will load 10 bytes + 22 zeros
  ASSERT_TRUE(stack.push(Uint256(10)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);

  // First byte should be 0x0B (11), padded with zeros
  auto loaded = stack[0];
  EXPECT_EQ(loaded.limb(3) >> 56, 0x0B);
}

TEST(CalldataloadOpcodeTest, ReturnsZeroWhenOffsetBeyondData) {
  Stack stack;
  uint64_t gas = 100;
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Push offset 100 - beyond data
  ASSERT_TRUE(stack.push(Uint256(100)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
}

TEST(CalldataloadOpcodeTest, ReturnsZeroForHugeOffset) {
  Stack stack;
  uint64_t gas = 100;
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Push offset that doesn't fit in uint64
  const Uint256 huge_offset = Uint256(1) << Uint256(128);
  ASSERT_TRUE(stack.push(huge_offset));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
}

TEST(CalldataloadOpcodeTest, EmptyCalldata) {
  Stack stack;
  uint64_t gas = 100;
  constexpr std::span<const uint8_t> EMPTY_DATA;

  // Push offset 0
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldataload(stack, gas, 3, EMPTY_DATA);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
}

TEST(CalldataloadOpcodeTest, StackUnderflow) {
  Stack stack;
  uint64_t gas = 100;
  std::array<uint8_t, 32> data{};

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(gas, 100);  // Gas unchanged
}

TEST(CalldataloadOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;  // Less than cost of 3
  std::array<uint8_t, 32> data{};

  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldataload(stack, gas, 3, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 1);  // Stack unchanged
  EXPECT_EQ(gas, 2);           // Gas unchanged
}

// =============================================================================
// CALLDATASIZE Opcode Tests
// =============================================================================

TEST(CalldatasizeOpcodeTest, PushesSize) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = calldatasize(stack, gas, 2, 42);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(42));
  EXPECT_EQ(gas, 98);
}

TEST(CalldatasizeOpcodeTest, PushesZeroForEmptyCalldata) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = calldatasize(stack, gas, 2, 0);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256::zero());
}

TEST(CalldatasizeOpcodeTest, PushesLargeSize) {
  Stack stack;
  uint64_t gas = 100;

  const auto result = calldatasize(stack, gas, 2, 1000000);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(1000000));
}

TEST(CalldatasizeOpcodeTest, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;

  const auto result = calldatasize(stack, gas, 2, 100);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(gas, 1);
}

TEST(CalldatasizeOpcodeTest, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;

  // Fill the stack to capacity
  for (size_t i = 0; i < 1024; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  const auto result = calldatasize(stack, gas, 2, 100);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), 1024);
}

// =============================================================================
// CALLDATACOPY Opcode Tests
// =============================================================================

// Helper: simple memory cost function (always returns 0 for simplicity in tests)
const auto ZERO_MEMORY_COST = [](uint64_t /*current_size*/, uint64_t /*offset*/,
                                 uint64_t /*size*/) -> uint64_t { return 0; };

TEST(CalldatacopyOpcodeTest, CopiesFullData) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 32> data{};
  for (size_t i = 0; i < 32; ++i) {
    data.at(i) = static_cast<uint8_t>(i + 1);
  }

  // Push size, srcOffset, destOffset (reverse order for stack)
  ASSERT_TRUE(stack.push(Uint256(32)));  // size
  ASSERT_TRUE(stack.push(Uint256(0)));   // srcOffset
  ASSERT_TRUE(stack.push(Uint256(0)));   // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);  // All 3 args popped

  // Verify memory contents
  EXPECT_GE(memory.size(), 32);
  auto mem_span = memory.read_span_unsafe(0, 32);
  for (size_t i = 0; i < 32; ++i) {
    EXPECT_EQ(mem_span[i], static_cast<uint8_t>(i + 1));
  }
}

TEST(CalldatacopyOpcodeTest, CopiesPartialData) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 64> data{};
  for (size_t i = 0; i < 64; ++i) {
    data.at(i) = static_cast<uint8_t>(i);
  }

  // Copy 16 bytes from offset 10 to memory offset 5
  ASSERT_TRUE(stack.push(Uint256(16)));  // size
  ASSERT_TRUE(stack.push(Uint256(10)));  // srcOffset
  ASSERT_TRUE(stack.push(Uint256(5)));   // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);

  // Verify memory contents at offset 5
  EXPECT_GE(memory.size(), 21);
  auto mem_span = memory.read_span_unsafe(5, 16);
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_EQ(mem_span[i], static_cast<uint8_t>(10 + i));
  }
}

TEST(CalldatacopyOpcodeTest, ZeroPadsWhenBeyondCalldata) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Copy 20 bytes starting at offset 5 (only 5 bytes available)
  ASSERT_TRUE(stack.push(Uint256(20)));  // size
  ASSERT_TRUE(stack.push(Uint256(5)));   // srcOffset
  ASSERT_TRUE(stack.push(Uint256(0)));   // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);

  // Verify memory contents
  auto mem_span = memory.read_span_unsafe(0, 20);
  // First 5 bytes should be from calldata
  EXPECT_EQ(mem_span[0], 0xAA);
  EXPECT_EQ(mem_span[1], 0x99);
  EXPECT_EQ(mem_span[2], 0x88);
  EXPECT_EQ(mem_span[3], 0x77);
  EXPECT_EQ(mem_span[4], 0x66);

  // Rest should be zero-padded
  for (size_t i = 5; i < 20; ++i) {
    EXPECT_EQ(mem_span[i], 0);
  }
}

TEST(CalldatacopyOpcodeTest, ZeroPadsWhenSourceBeyondCalldata) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Copy 32 bytes starting at offset 100 (entirely beyond calldata)
  ASSERT_TRUE(stack.push(Uint256(32)));   // size
  ASSERT_TRUE(stack.push(Uint256(100)));  // srcOffset (beyond data)
  ASSERT_TRUE(stack.push(Uint256(0)));    // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);

  // All 32 bytes should be zero
  auto mem_span = memory.read_span_unsafe(0, 32);
  for (size_t i = 0; i < 32; ++i) {
    EXPECT_EQ(mem_span[i], 0);
  }
}

TEST(CalldatacopyOpcodeTest, ZeroSizeCopy) {
  Stack stack;
  Memory memory;
  uint64_t gas = 100;
  std::array<uint8_t, 10> data = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66};

  // Copy 0 bytes - should be a no-op (no memory expansion)
  ASSERT_TRUE(stack.push(Uint256(0)));  // size = 0
  ASSERT_TRUE(stack.push(Uint256(0)));  // srcOffset
  ASSERT_TRUE(stack.push(Uint256(0)));  // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(memory.size(), 0);  // No memory expansion for zero-size copy
  EXPECT_EQ(gas, 97);           // Only base cost deducted
}

TEST(CalldatacopyOpcodeTest, StackUnderflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data{};

  // Only push 2 values instead of 3
  ASSERT_TRUE(stack.push(Uint256(10)));
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(stack.size(), 2);  // Stack unchanged
}

TEST(CalldatacopyOpcodeTest, OutOfGasBaseCost) {
  Stack stack;
  Memory memory;
  uint64_t gas = 2;  // Less than base cost of 3
  std::array<uint8_t, 10> data{};

  ASSERT_TRUE(stack.push(Uint256(10)));
  ASSERT_TRUE(stack.push(Uint256(0)));
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 3);  // Stack unchanged
}

TEST(CalldatacopyOpcodeTest, GasCalculationWithWordCost) {
  Stack stack;
  Memory memory;
  // 3 (base) + 3 * ceil(64/32) = 3 + 6 = 9
  uint64_t gas = 100;
  std::array<uint8_t, 64> data{};

  ASSERT_TRUE(stack.push(Uint256(64)));  // size = 64 bytes = 2 words
  ASSERT_TRUE(stack.push(Uint256(0)));
  ASSERT_TRUE(stack.push(Uint256(0)));

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Base cost (3) + word cost (3 * 2 = 6) = 9
  EXPECT_EQ(gas, 91);
}

TEST(CalldatacopyOpcodeTest, HugeSourceOffsetZeroPads) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data{};

  // Source offset that doesn't fit in uint64
  const Uint256 huge_offset = Uint256(1) << Uint256(128);
  ASSERT_TRUE(stack.push(Uint256(32)));  // size
  ASSERT_TRUE(stack.push(huge_offset));  // srcOffset
  ASSERT_TRUE(stack.push(Uint256(0)));   // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::Success);

  // All bytes should be zero
  auto mem_span = memory.read_span_unsafe(0, 32);
  for (size_t i = 0; i < 32; ++i) {
    EXPECT_EQ(mem_span[i], 0);
  }
}

TEST(CalldatacopyOpcodeTest, HugeDestOffsetOutOfGas) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data{};

  // Dest offset that doesn't fit in uint64
  const Uint256 huge_offset = Uint256(1) << Uint256(128);
  ASSERT_TRUE(stack.push(Uint256(32)));  // size
  ASSERT_TRUE(stack.push(Uint256(0)));   // srcOffset
  ASSERT_TRUE(stack.push(huge_offset));  // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(CalldatacopyOpcodeTest, HugeSizeOutOfGas) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;
  std::array<uint8_t, 10> data{};

  // Size that doesn't fit in uint64
  const Uint256 huge_size = Uint256(1) << Uint256(128);
  ASSERT_TRUE(stack.push(huge_size));   // size
  ASSERT_TRUE(stack.push(Uint256(0)));  // srcOffset
  ASSERT_TRUE(stack.push(Uint256(0)));  // destOffset

  const auto result = calldatacopy(stack, gas, 3, ZERO_MEMORY_COST, memory, std::span(data));

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

}  // namespace div0::evm::opcodes
