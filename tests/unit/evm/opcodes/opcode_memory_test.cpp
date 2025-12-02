// Unit tests for memory opcodes: MLOAD, MSTORE, MSTORE8, MSIZE

#include <div0/evm/gas/dynamic_costs.h>
#include <div0/evm/memory.h>
#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include "../src/evm/opcodes/memory.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_MLOAD = 3;
constexpr uint64_t GAS_MSTORE = 3;
constexpr uint64_t GAS_MSTORE8 = 3;
constexpr uint64_t GAS_MSIZE = 2;

// =============================================================================
// MLOAD Opcode Tests
// =============================================================================

TEST(OpcodeMload, BasicLoad) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  // Pre-expand and store a value
  memory.expand(32);
  memory.store_word_unsafe(0, Uint256(0x12345678));

  (void)stack.push(Uint256(0));  // offset

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0x12345678));
}

TEST(OpcodeMload, LoadFromUnexpandedMemory) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));  // offset

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256::zero());  // Uninitialized memory is zero
  EXPECT_EQ(memory.size(), 32);          // Memory expanded
}

TEST(OpcodeMload, LoadAtOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  memory.expand(96);
  memory.store_word_unsafe(64, Uint256(0xDEADBEEF));

  (void)stack.push(Uint256(64));  // offset

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xDEADBEEF));
}

TEST(OpcodeMload, StackUnderflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 1000);  // Gas unchanged on error
}

TEST(OpcodeMload, OutOfGasStatic) {
  Stack stack;
  Memory memory;
  uint64_t gas = 2;  // Less than GAS_MLOAD (3)

  memory.expand(32);
  (void)stack.push(Uint256(0));

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeMload, OutOfGasLargeOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  // Offset that would require massive memory expansion
  (void)stack.push(Uint256(UINT64_MAX - 100));

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeMload, OutOfGasUint256Offset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  // Offset > UINT64_MAX
  (void)stack.push(Uint256(1, 1, 0, 0));

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeMload, GasCalculation) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));

  const auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (3) + expansion cost for 1 word (3)
  EXPECT_EQ(gas, 1000 - 6);
}

// Regression test for overflow in static_cost + mem_cost
TEST(OpcodeMload, TotalCostOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  memory.expand(32);
  (void)stack.push(Uint256(0));

  // Custom cost function that returns a value that would overflow when added to static_cost
  auto overflow_cost_fn = [](uint64_t, uint64_t, uint64_t) -> uint64_t {
    return UINT64_MAX - 1;  // Adding 3 to this would overflow
  };

  const auto result = mload(stack, gas, GAS_MLOAD, overflow_cost_fn, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// MSTORE Opcode Tests
// =============================================================================

TEST(OpcodeMstore, BasicStore) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0xCAFEBABE));  // value
  (void)stack.push(Uint256(0));           // offset

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);
  EXPECT_EQ(memory.load_word_unsafe(0), Uint256(0xCAFEBABE));
}

TEST(OpcodeMstore, StoreAtOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0x12345678));
  (void)stack.push(Uint256(64));

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(memory.size(), 96);  // 64 + 32 = 96, aligned
  EXPECT_EQ(memory.load_word_unsafe(64), Uint256(0x12345678));
}

TEST(OpcodeMstore, StackUnderflowEmpty) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeMstore, StackUnderflowOneElement) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeMstore, OutOfGasStatic) {
  Stack stack;
  Memory memory;
  uint64_t gas = 2;

  memory.expand(32);
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(0));

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeMstore, OutOfGasLargeOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(UINT64_MAX - 100));

  const auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// Regression test for overflow in static_cost + mem_cost
TEST(OpcodeMstore, TotalCostOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  memory.expand(32);
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(0));

  auto overflow_cost_fn = [](uint64_t, uint64_t, uint64_t) -> uint64_t { return UINT64_MAX - 1; };

  const auto result = mstore(stack, gas, GAS_MSTORE, overflow_cost_fn, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// MSTORE8 Opcode Tests
// =============================================================================

TEST(OpcodeMstore8, BasicStoreByte) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0xFF));  // value (only LSB used)
  (void)stack.push(Uint256(0));     // offset

  const auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);

  auto span = memory.read_span_unsafe(0, 1);
  EXPECT_EQ(span[0], 0xFF);
}

TEST(OpcodeMstore8, StoresOnlyLSB) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  // Value with multiple bytes set, only 0xAB (LSB) should be stored
  (void)stack.push(Uint256(0x123456AB));
  (void)stack.push(Uint256(0));

  const auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  auto span = memory.read_span_unsafe(0, 1);
  EXPECT_EQ(span[0], 0xAB);
}

TEST(OpcodeMstore8, StoreAtOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0x42));
  (void)stack.push(Uint256(31));

  const auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(memory.size(), 32);

  auto span = memory.read_span_unsafe(31, 1);
  EXPECT_EQ(span[0], 0x42);
}

TEST(OpcodeMstore8, StackUnderflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  const auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST(OpcodeMstore8, OutOfGasStatic) {
  Stack stack;
  Memory memory;
  uint64_t gas = 2;

  memory.expand(32);
  (void)stack.push(Uint256(0xFF));
  (void)stack.push(Uint256(0));

  const auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// Regression test for overflow in static_cost + mem_cost
TEST(OpcodeMstore8, TotalCostOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  memory.expand(32);
  (void)stack.push(Uint256(0xFF));
  (void)stack.push(Uint256(0));

  auto overflow_cost_fn = [](uint64_t, uint64_t, uint64_t) -> uint64_t { return UINT64_MAX - 1; };

  const auto result = mstore8(stack, gas, GAS_MSTORE8, overflow_cost_fn, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// MSIZE Opcode Tests
// =============================================================================

TEST(OpcodeMsize, EmptyMemory) {
  Stack stack;
  const Memory memory;
  uint64_t gas = 1000;

  const auto result = msize(stack, gas, GAS_MSIZE, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0));
  EXPECT_EQ(gas, 1000 - GAS_MSIZE);
}

TEST(OpcodeMsize, AfterExpansion) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  memory.expand(100);  // Expands to 128 (word-aligned)

  const auto result = msize(stack, gas, GAS_MSIZE, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(128));
}

TEST(OpcodeMsize, StackOverflow) {
  Stack stack;
  const Memory memory;
  uint64_t gas = 1000;

  // Fill stack to capacity
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    (void)stack.push(Uint256(i));
  }

  const auto result = msize(stack, gas, GAS_MSIZE, memory);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
}

TEST(OpcodeMsize, OutOfGas) {
  Stack stack;
  const Memory memory;
  uint64_t gas = 1;  // Less than GAS_MSIZE (2)

  const auto result = msize(stack, gas, GAS_MSIZE, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Integration Tests - MLOAD/MSTORE round-trip
// =============================================================================

TEST(OpcodeMemoryIntegration, StoreAndLoadRoundTrip) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  const auto value = Uint256(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0, 0);

  // MSTORE
  (void)stack.push(value);
  (void)stack.push(Uint256(0));
  ASSERT_EQ(mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory),
            ExecutionStatus::Success);

  // MLOAD
  (void)stack.push(Uint256(0));
  ASSERT_EQ(mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory),
            ExecutionStatus::Success);

  EXPECT_EQ(stack[0], value);
}

TEST(OpcodeMemoryIntegration, MultipleStoresAndLoads) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Store at offset 0
  (void)stack.push(Uint256(111));
  (void)stack.push(Uint256(0));
  ASSERT_EQ(mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory),
            ExecutionStatus::Success);

  // Store at offset 32
  (void)stack.push(Uint256(222));
  (void)stack.push(Uint256(32));
  ASSERT_EQ(mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory),
            ExecutionStatus::Success);

  // Load from offset 0
  (void)stack.push(Uint256(0));
  ASSERT_EQ(mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory),
            ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(111));

  // Load from offset 32
  (void)stack.push(Uint256(32));
  ASSERT_EQ(mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory),
            ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(222));
}

}  // namespace div0::evm::opcodes
