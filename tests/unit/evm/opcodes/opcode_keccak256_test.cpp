// Unit tests for KECCAK256 (SHA3) opcode

#include <div0/evm/gas/dynamic_costs.h>
#include <div0/evm/memory.h>
#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include "../src/evm/opcodes/keccak256.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_SHA3_STATIC = 30;  // Base cost
constexpr uint64_t GAS_SHA3_WORD = 6;     // Per 32-byte word

// Known hash constants from Ethereum
// keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
constexpr auto EMPTY_HASH = Uint256(0x7bfad8045d85a470ULL, 0xe500b653ca82273bULL,
                                    0x927e7db2dcc703c0ULL, 0xc5d2460186f7233cULL);

// keccak256(32 zeros) = 290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563
constexpr auto ZEROS_32_HASH = Uint256(0x362f93160ef3e563ULL, 0x4ba6bc95484008f6ULL,
                                       0xd60345a988386fc8ULL, 0x290decd9548b62a8ULL);

// =============================================================================
// Basic Hash Tests
// =============================================================================

TEST(OpcodeKeccak256, EmptyInput) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));  // length
  (void)stack.push(Uint256(0));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], EMPTY_HASH);
  // Only static cost charged (no words, no memory expansion)
  EXPECT_EQ(gas, 1000 - GAS_SHA3_STATIC);
}

TEST(OpcodeKeccak256, HashZeroMemory) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Pre-expand memory with zeros
  memory.expand(32);

  (void)stack.push(Uint256(32));  // length
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], ZEROS_32_HASH);
}

TEST(OpcodeKeccak256, HashAtOffset) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Pre-expand memory
  memory.expand(96);
  memory.store_word_unsafe(64, Uint256(0));  // 32 zeros at offset 64

  (void)stack.push(Uint256(32));  // length
  (void)stack.push(Uint256(64));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZEROS_32_HASH);
}

TEST(OpcodeKeccak256, MemoryExpansion) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Memory is empty, should expand
  EXPECT_EQ(memory.size(), 0);

  (void)stack.push(Uint256(32));  // length
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(memory.size(), 32);  // Memory expanded to 32 bytes
  EXPECT_EQ(stack[0], ZEROS_32_HASH);
}

// =============================================================================
// Stack Operation Tests
// =============================================================================

TEST(OpcodeKeccak256, StackUnderflowEmpty) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 1000);  // Gas unchanged on error
}

TEST(OpcodeKeccak256, StackUnderflowOneElement) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));  // Only offset, missing length

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 1000);
}

TEST(OpcodeKeccak256, StackPopsCorrectly) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Push extra items to verify correct popping
  (void)stack.push(Uint256(999));  // Will remain
  (void)stack.push(Uint256(0));    // length
  (void)stack.push(Uint256(0));    // offset

  EXPECT_EQ(stack.size(), 3);

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 2);         // offset and length popped, hash pushed
  EXPECT_EQ(stack[0], EMPTY_HASH);    // Top is the hash
  EXPECT_EQ(stack[1], Uint256(999));  // Original value preserved
}

// =============================================================================
// Gas Calculation Tests
// =============================================================================

TEST(OpcodeKeccak256, GasCalculationZeroLength) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  (void)stack.push(Uint256(0));  // length = 0
  (void)stack.push(Uint256(0));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (30) + 0 words + 0 memory = 30
  EXPECT_EQ(gas, 1000 - 30);
}

TEST(OpcodeKeccak256, GasCalculationOneWord) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  memory.expand(32);  // Pre-expand to avoid memory cost

  (void)stack.push(Uint256(32));  // length = 32 = 1 word
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (30) + 1 word * 6 = 36
  EXPECT_EQ(gas, 10000 - 36);
}

TEST(OpcodeKeccak256, GasCalculationPartialWord) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  memory.expand(32);  // Pre-expand to avoid memory cost

  (void)stack.push(Uint256(1));  // length = 1 byte = ceil(1/32) = 1 word
  (void)stack.push(Uint256(0));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (30) + 1 word * 6 = 36
  EXPECT_EQ(gas, 10000 - 36);
}

TEST(OpcodeKeccak256, GasCalculationMultipleWords) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  memory.expand(100);  // Pre-expand to avoid memory cost

  (void)stack.push(Uint256(65));  // length = 65 bytes = ceil(65/32) = 3 words
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (30) + 3 words * 6 = 48
  EXPECT_EQ(gas, 10000 - 48);
}

TEST(OpcodeKeccak256, GasWithMemoryExpansion) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Memory is empty, will need expansion
  (void)stack.push(Uint256(32));  // length
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = static (30) + 1 word * 6 + memory expansion for 1 word (3) = 39
  EXPECT_EQ(gas, 10000 - 39);
}

// =============================================================================
// Out of Gas Tests
// =============================================================================

TEST(OpcodeKeccak256, OutOfGasStatic) {
  Stack stack;
  Memory memory;
  uint64_t gas = 29;  // Less than static cost (30)

  (void)stack.push(Uint256(0));  // length
  (void)stack.push(Uint256(0));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeKeccak256, OutOfGasWordCost) {
  Stack stack;
  Memory memory;
  uint64_t gas = 35;  // Enough for static (30) but not static + 1 word (36)

  memory.expand(32);  // Pre-expand to avoid memory cost

  (void)stack.push(Uint256(32));  // length = 1 word
  (void)stack.push(Uint256(0));   // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeKeccak256, OutOfGasMemoryExpansion) {
  Stack stack;
  Memory memory;
  uint64_t gas = 1000;

  // Massive offset that would require enormous memory expansion
  (void)stack.push(Uint256(32));
  (void)stack.push(Uint256(UINT64_MAX - 100));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Overflow Tests
// =============================================================================

TEST(OpcodeKeccak256, OffsetUint256Overflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Offset > UINT64_MAX
  (void)stack.push(Uint256(32));
  (void)stack.push(Uint256(1, 1, 0, 0));  // offset > 2^64

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeKeccak256, LengthUint256Overflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  // Length > UINT64_MAX
  (void)stack.push(Uint256(1, 1, 0, 0));  // length > 2^64
  (void)stack.push(Uint256(0));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeKeccak256, OffsetPlusLengthOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  // offset + length would overflow uint64
  (void)stack.push(Uint256(100));              // length
  (void)stack.push(Uint256(UINT64_MAX - 50));  // offset

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

TEST(OpcodeKeccak256, WordCountOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  // length + 31 would overflow
  (void)stack.push(Uint256(UINT64_MAX));
  (void)stack.push(Uint256(0));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// Regression test for word_cost * words overflow
TEST(OpcodeKeccak256, DynamicCostOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  memory.expand(128);

  // 64 bytes = 2 words. With huge_word_cost, 2 * huge_word_cost overflows
  (void)stack.push(Uint256(64));
  (void)stack.push(Uint256(0));

  // Custom word cost that would overflow when multiplied by 2
  constexpr uint64_t HUGE_WORD_COST = UINT64_MAX / 2 + 1;

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, HUGE_WORD_COST, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// Regression test for static_cost + dynamic_cost overflow
TEST(OpcodeKeccak256, StaticPlusDynamicOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  memory.expand(32);

  (void)stack.push(Uint256(32));
  (void)stack.push(Uint256(0));

  // Custom costs that would overflow when added
  constexpr uint64_t HUGE_STATIC = UINT64_MAX - 5;
  constexpr uint64_t WORD_COST = 10;  // 1 word * 10 = 10, which overflows with huge_static

  const auto result =
      keccak256(stack, gas, HUGE_STATIC, WORD_COST, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// Regression test for total_cost + mem_cost overflow
TEST(OpcodeKeccak256, TotalPlusMemCostOverflow) {
  Stack stack;
  Memory memory;
  uint64_t gas = UINT64_MAX;

  (void)stack.push(Uint256(32));
  (void)stack.push(Uint256(0));

  // Custom memory cost function that returns a value causing overflow
  auto overflow_mem_cost_fn = [](uint64_t, uint64_t, uint64_t) -> uint64_t {
    return UINT64_MAX - 10;  // Adding static (30) + word (6) would overflow
  };

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, overflow_mem_cost_fn, memory);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(OpcodeKeccak256, ExactlyOneWord) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  memory.expand(32);

  (void)stack.push(Uint256(32));  // Exactly 32 bytes = 1 word
  (void)stack.push(Uint256(0));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], ZEROS_32_HASH);
}

TEST(OpcodeKeccak256, JustOverOneWord) {
  Stack stack;
  Memory memory;
  uint64_t gas = 10000;

  memory.expand(64);

  (void)stack.push(Uint256(33));  // 33 bytes = 2 words
  (void)stack.push(Uint256(0));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = 30 + 2*6 = 42
  EXPECT_EQ(gas, 10000 - 42);
}

TEST(OpcodeKeccak256, LargeInputMultipleBlocks) {
  Stack stack;
  Memory memory;
  uint64_t gas = 100000;

  // 272 bytes = 2 Keccak blocks (136 bytes each)
  memory.expand(272);

  (void)stack.push(Uint256(272));  // 272 bytes = ceil(272/32) = 9 words
  (void)stack.push(Uint256(0));

  const auto result =
      keccak256(stack, gas, GAS_SHA3_STATIC, GAS_SHA3_WORD, gas::memory::access_cost, memory);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Cost = 30 + 9*6 = 84
  EXPECT_EQ(gas, 100000 - 84);
}

}  // namespace div0::evm::opcodes
