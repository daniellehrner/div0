#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "div0/db/memory_database.h"
#include "div0/evm/block_context.h"
#include "div0/evm/call_params.h"
#include "div0/evm/evm.h"
#include "div0/evm/execution_environment.h"
#include "div0/evm/forks.h"
#include "div0/evm/tx_context.h"
#include "div0/state/memory_account_provider.h"
#include "div0/state/memory_code_provider.h"
#include "div0/state/memory_storage_provider.h"
#include "div0/state/state_context.h"

namespace div0::evm {
namespace {

// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
class EvmExecutionTest : public ::testing::Test {
 protected:
  db::MemoryDatabase database_;
  state::MemoryStorageProvider storage_{database_};
  state::MemoryAccountProvider accounts_;
  state::MemoryCodeProvider code_provider_;
  state::StateContext state_context_{storage_, accounts_, code_provider_};

  // Default block context for tests
  BlockContext block_ctx_{
      .number = 1,
      .timestamp = 1000,
      .gas_limit = 30000000,
      .chain_id = 1,
      .base_fee = types::Uint256::zero(),
      .blob_base_fee = types::Uint256::zero(),
      .prev_randao = types::Uint256::zero(),
      .coinbase = types::Address(types::Uint256::zero()),
      .get_block_hash = nullptr,
      .block_hash_user_data = nullptr,
  };

  // Helper to create ExecutionEnvironment from code and gas
  ExecutionEnvironment make_env(std::span<const uint8_t> code, uint64_t gas) {
    return {
        .block = &block_ctx_,
        .tx = {.origin = types::Address(types::Uint256::zero()),
               .gas_price = types::Uint256::zero(),
               .blob_hashes = {}},
        .call = {.value = types::Uint256::zero(),
                 .gas = gas,
                 .code = code,
                 .input = {},
                 .caller = types::Address(types::Uint256::zero()),
                 .address = types::Address(types::Uint256::zero()),
                 .is_static = false},
    };
  }
};
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

TEST_F(EvmExecutionTest, EmptyCodeSucceeds) {
  EVM evm(state_context_, Fork::Shanghai);

  const std::vector<uint8_t> code;
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);
}

TEST_F(EvmExecutionTest, StopOpcode) {
  EVM evm(state_context_, Fork::Shanghai);

  // STOP
  const std::vector<uint8_t> code = {0x00};
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);
}

TEST_F(EvmExecutionTest, AdditionAndStoreResult) {
  EVM evm(state_context_, Fork::Shanghai);

  // PUSH1 2, PUSH1 3, ADD (=5), PUSH0 (slot=0), SSTORE, STOP
  const std::vector<uint8_t> code = {
      0x60, 0x02,  // PUSH1 2
      0x60, 0x03,  // PUSH1 3
      0x01,        // ADD
      0x5F,        // PUSH0 (slot)
      0x55,        // SSTORE
      0x00         // STOP
  };
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  // Verify storage: slot 0 should contain 5
  const auto stored = storage_.load(types::Address(types::Uint256::zero()),
                                    types::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256(5));
}

TEST_F(EvmExecutionTest, MultiplicationAndStoreResult) {
  EVM evm(state_context_, Fork::Shanghai);

  // PUSH1 7, PUSH1 6, MUL (=42), PUSH0 (slot=0), SSTORE, STOP
  const std::vector<uint8_t> code = {
      0x60, 0x07,  // PUSH1 7
      0x60, 0x06,  // PUSH1 6
      0x02,        // MUL
      0x5F,        // PUSH0 (slot)
      0x55,        // SSTORE
      0x00         // STOP
  };
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  // Verify storage: slot 0 should contain 42
  const auto stored = storage_.load(types::Address(types::Uint256::zero()),
                                    types::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256(42));
}

TEST_F(EvmExecutionTest, InvalidOpcodeReturnsError) {
  EVM evm(state_context_, Fork::Shanghai);

  // 0xFE is INVALID opcode
  const std::vector<uint8_t> code = {0xFE};
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::InvalidOpcode);
}

TEST_F(EvmExecutionTest, OutOfGasReturnsError) {
  EVM evm(state_context_, Fork::Shanghai);

  // PUSH1 1 costs 3 gas, give only 2
  const std::vector<uint8_t> code = {0x60, 0x01};
  const auto env = make_env(code, 2);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::OutOfGas);
}

TEST_F(EvmExecutionTest, StackUnderflowReturnsError) {
  EVM evm(state_context_, Fork::Shanghai);

  // ADD without any values on stack
  const std::vector<uint8_t> code = {0x01};
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::StackUnderflow);
}

}  // namespace
}  // namespace div0::evm
