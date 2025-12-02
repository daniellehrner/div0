// Unit tests for stack opcodes: PUSH0, PUSH1-PUSH32

#include <div0/evm/stack.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "../src/evm/opcodes/push.h"

namespace div0::evm::opcodes {

using types::Uint256;

// Gas costs from EVM specification
constexpr uint64_t GAS_PUSH0 = 2;
constexpr uint64_t GAS_PUSH = 3;

// Maximum uint64 value
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// PUSH0 Opcode Tests
// =============================================================================

TEST(OpcodePush0, PushesZero) {
  Stack stack;
  uint64_t gas = 1000;

  const auto result = push0(stack, gas, GAS_PUSH0);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0));
  EXPECT_EQ(gas, 1000 - GAS_PUSH0);
}

TEST(OpcodePush0, MultiplePushes) {
  Stack stack;
  uint64_t gas = 100;

  push0(stack, gas, GAS_PUSH0);
  push0(stack, gas, GAS_PUSH0);
  push0(stack, gas, GAS_PUSH0);

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], Uint256(0));
  EXPECT_EQ(stack[1], Uint256(0));
  EXPECT_EQ(stack[2], Uint256(0));
}

TEST(OpcodePush0, OutOfGas) {
  Stack stack;
  uint64_t gas = 1;  // Less than GAS_PUSH0 (2)

  const auto result = push0(stack, gas, GAS_PUSH0);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(gas, 1);  // Unchanged
  EXPECT_TRUE(stack.empty());
}

TEST(OpcodePush0, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;

  // Fill stack to capacity
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    (void)stack.push(Uint256(i));
  }

  const auto result = push0(stack, gas, GAS_PUSH0);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);  // Unchanged
}

// =============================================================================
// PUSH1 Opcode Tests
// =============================================================================

TEST(OpcodePush1, SingleByte) {
  Stack stack;
  uint64_t gas = 1000;
  const std::vector<uint8_t> code = {0xFF};
  uint64_t pc = 0;

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0xFF));
  EXPECT_EQ(pc, 1);
  EXPECT_EQ(gas, 1000 - GAS_PUSH);
}

TEST(OpcodePush1, ZeroByte) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0x00};
  uint64_t pc = 0;

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodePush1, TruncatedBytecode) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {};  // Empty - truncated
  uint64_t pc = 0;

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // Truncated bytecode fills with zeros (shifted left)
  EXPECT_EQ(stack[0], Uint256(0));
  EXPECT_EQ(pc, 1);  // PC still advances by 1
}

TEST(OpcodePush1, OutOfGas) {
  Stack stack;
  uint64_t gas = 2;  // Less than GAS_PUSH (3)
  const std::vector<uint8_t> code = {0xFF};
  uint64_t pc = 0;

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(pc, 0);  // PC unchanged
}

// =============================================================================
// PUSH2 Opcode Tests
// =============================================================================

TEST(OpcodePush2, TwoBytes) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0xAB, 0xCD};
  uint64_t pc = 0;

  const auto result = push_n<2>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xABCD));  // Big-endian: 0xAB * 256 + 0xCD
  EXPECT_EQ(pc, 2);
}

TEST(OpcodePush2, TruncatedOneByte) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0xAB};  // Only 1 byte available
  uint64_t pc = 0;

  const auto result = push_n<2>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  // 0xAB with MSB alignment, zeros in LSB
  EXPECT_EQ(stack[0], Uint256(0xAB00));
  EXPECT_EQ(pc, 2);
}

// =============================================================================
// PUSH4 Opcode Tests
// =============================================================================

TEST(OpcodePush4, FourBytes) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0xDE, 0xAD, 0xBE, 0xEF};
  uint64_t pc = 0;

  const auto result = push_n<4>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xDEADBEEF));
  EXPECT_EQ(pc, 4);
}

// =============================================================================
// PUSH8 Opcode Tests
// =============================================================================

TEST(OpcodePush8, EightBytes) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
  uint64_t pc = 0;

  const auto result = push_n<8>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0x123456789ABCDEF0ULL));
  EXPECT_EQ(pc, 8);
}

TEST(OpcodePush8, MaxValue) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint64_t pc = 0;

  const auto result = push_n<8>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(MAX64));
}

// =============================================================================
// PUSH16 Opcode Tests (Two Limbs)
// =============================================================================

TEST(OpcodePush16, SixteenBytes) {
  Stack stack;
  uint64_t gas = 100;
  // 16 bytes: 8 for limb1, 8 for limb0
  const std::vector<uint8_t> code = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,  // limb1
      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10   // limb0
  };
  uint64_t pc = 0;

  const auto result = push_n<16>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(pc, 16);

  constexpr auto EXPECTED = Uint256(0x090A0B0C0D0E0F10ULL, 0x0102030405060708ULL, 0, 0);
  EXPECT_EQ(stack[0], EXPECTED);
}

// =============================================================================
// PUSH20 Opcode Tests (Ethereum Address Size)
// =============================================================================

TEST(OpcodePush20, TwentyBytes) {
  Stack stack;
  uint64_t gas = 100;
  // Typical Ethereum address: 20 bytes
  const std::vector<uint8_t> code = {
      0xDE, 0xAD, 0xBE, 0xEF,                          // 4 bytes  -> limb2 (MSB)
      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,  // 8 bytes  -> limb1
      0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00   // 8 bytes  -> limb0
  };
  uint64_t pc = 0;

  const auto result = push_n<20>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(pc, 20);
  // limb0 = last 8 bytes, limb1 = middle 8 bytes, limb2 = first 4 bytes
  constexpr auto EXPECTED = Uint256(0x99AABBCCDDEEFF00ULL, 0x1122334455667788ULL, 0xDEADBEEF, 0);
  EXPECT_EQ(stack[0], EXPECTED);
}

// =============================================================================
// PUSH32 Opcode Tests (Full 256-bit Value)
// =============================================================================

TEST(OpcodePush32, ThirtyTwoBytes) {
  Stack stack;
  uint64_t gas = 100;
  // Full 32 bytes
  const std::vector<uint8_t> code = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  // limb3 (MSB)
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,  // limb2
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,  // limb1
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04   // limb0 (LSB)
  };
  uint64_t pc = 0;

  const auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(pc, 32);

  constexpr auto EXPECTED = Uint256(4, 3, 2, 1);
  EXPECT_EQ(stack[0], EXPECTED);
}

TEST(OpcodePush32, MaxValue) {
  Stack stack;
  uint64_t gas = 100;
  // All 0xFF bytes
  std::vector<uint8_t> code(32, 0xFF);
  uint64_t pc = 0;

  const auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256::max());
}

TEST(OpcodePush32, ZeroValue) {
  Stack stack;
  uint64_t gas = 100;
  std::vector<uint8_t> code(32, 0x00);
  uint64_t pc = 0;

  const auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));
}

TEST(OpcodePush32, TruncatedBytecode) {
  Stack stack;
  uint64_t gas = 100;
  // Only 16 bytes available for PUSH32
  const std::vector<uint8_t> code = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint64_t pc = 0;

  const auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(pc, 32);  // PC still advances by 32
  // First 16 bytes go to MSB positions (limb3, limb2)
  // Remaining positions (limb1, limb0) are zero
  constexpr auto EXPECTED = Uint256(0, 0, MAX64, MAX64);
  EXPECT_EQ(stack[0], EXPECTED);
}

// =============================================================================
// Program Counter Advancement Tests
// =============================================================================

TEST(OpcodePush, PcAdvancesFromMiddle) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00};
  uint64_t pc = 2;  // Start in middle

  const auto result = push_n<2>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0xABCD));
  EXPECT_EQ(pc, 4);
}

TEST(OpcodePush, PcAtEndOfBytecode) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0x60, 0x00};  // Some bytecode
  uint64_t pc = 2;                                 // At end

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));  // Truncated = zeros
  EXPECT_EQ(pc, 3);
}

// =============================================================================
// Stack Overflow Tests
// =============================================================================

TEST(OpcodePush, StackOverflow) {
  Stack stack;
  uint64_t gas = 10000;
  const std::vector<uint8_t> code = {0xFF};
  uint64_t pc = 0;

  // Fill stack to capacity
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    (void)stack.push(Uint256(i));
  }

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::StackOverflow);
  EXPECT_EQ(pc, 0);  // PC unchanged on error
}

// =============================================================================
// Gas Consumption Tests
// =============================================================================

TEST(OpcodePush, ExactGasConsumption) {
  Stack stack;
  uint64_t gas = GAS_PUSH;  // Exactly enough
  const std::vector<uint8_t> code = {0xFF};
  uint64_t pc = 0;

  const auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(gas, 0);
}

TEST(OpcodePush, MultiplePushesGas) {
  Stack stack;
  uint64_t gas = 100;
  const std::vector<uint8_t> code = {0x01, 0x02, 0x03, 0x04};
  uint64_t pc = 0;

  push_n<1>(stack, gas, GAS_PUSH, code, pc);
  push_n<1>(stack, gas, GAS_PUSH, code, pc);

  EXPECT_EQ(gas, 100 - (2 * GAS_PUSH));
  EXPECT_EQ(stack.size(), 2);
}

}  // namespace div0::evm::opcodes
