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
class EvmCallTest : public ::testing::Test {
 protected:
  db::MemoryDatabase database_;
  state::MemoryStorageProvider storage_{database_};
  state::MemoryAccountProvider accounts_;
  state::MemoryCodeProvider code_provider_;
  state::StateContext state_context_{storage_, accounts_, code_provider_};

  // Caller contract address
  types::Address caller_addr_{types::Uint256(0x1000)};
  // Callee contract address
  types::Address callee_addr_{types::Uint256(0x2000)};

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

  ExecutionEnvironment make_env(std::span<const uint8_t> code, uint64_t gas) {
    return {
        .block = &block_ctx_,
        .tx = {.origin = caller_addr_, .gas_price = types::Uint256::zero(), .blob_hashes = {}},
        .call = {.value = types::Uint256::zero(),
                 .gas = gas,
                 .code = code,
                 .input = {},
                 .caller = types::Address::zero(),
                 .address = caller_addr_,
                 .is_static = false},
    };
  }
};
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

// Helper to build a PUSH20 instruction (address)
void push_address(std::vector<uint8_t>& code, const types::Address& addr) {
  code.push_back(0x73);  // PUSH20
  // Convert address to 20 bytes big-endian
  // Address stores limbs in little-endian order, so we need to reverse
  const auto uint_val = addr.to_uint256();
  for (int i = 19; i >= 0; --i) {
    const int limb_idx = i / 8;
    const int byte_idx = i % 8;
    code.push_back(static_cast<uint8_t>(
        (uint_val.limb(static_cast<size_t>(limb_idx)) >> (byte_idx * 8)) & 0xFF));
  }
}

TEST_F(EvmCallTest, CallToEmptyCodeSucceeds) {
  // Callee has no code - CALL should succeed and push 1
  code_provider_.set_code(callee_addr_, {});

  // Build caller code:
  // PUSH1 0 (retSize)
  // PUSH1 0 (retOffset)
  // PUSH1 0 (argsSize)
  // PUSH1 0 (argsOffset)
  // PUSH1 0 (value)
  // PUSH20 <callee_addr> (address)
  // PUSH2 10000 (gas)
  // CALL
  // PUSH1 0
  // SSTORE (store result in slot 0)
  // STOP
  std::vector<uint8_t> code;
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (retSize)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (retOffset)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (argsSize)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (argsOffset)
  code.push_back(0x60);
  code.push_back(0x00);              // PUSH1 0 (value)
  push_address(code, callee_addr_);  // PUSH20 callee
  code.push_back(0x61);
  code.push_back(0x27);
  code.push_back(0x10);  // PUSH2 10000 (gas)
  code.push_back(0xF1);  // CALL
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (slot)
  code.push_back(0x55);  // SSTORE
  code.push_back(0x00);  // STOP

  EVM evm(state_context_, Fork::Shanghai);
  const auto env = make_env(code, 500000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  // CALL to empty code should succeed (push 1)
  const auto stored = storage_.load(caller_addr_, ethereum::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256(1));
}

TEST_F(EvmCallTest, CallToCodeThatStops) {
  // Callee just executes STOP
  code_provider_.set_code(callee_addr_, {0x00});  // STOP

  std::vector<uint8_t> code;
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (retSize)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (retOffset)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (argsSize)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (argsOffset)
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (value)
  push_address(code, callee_addr_);
  code.push_back(0x61);
  code.push_back(0x27);
  code.push_back(0x10);  // PUSH2 10000
  code.push_back(0xF1);  // CALL
  code.push_back(0x60);
  code.push_back(0x00);  // PUSH1 0 (slot)
  code.push_back(0x55);  // SSTORE
  code.push_back(0x00);  // STOP

  EVM evm(state_context_, Fork::Shanghai);
  const auto env = make_env(code, 500000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  const auto stored = storage_.load(caller_addr_, ethereum::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256(1));
}

TEST_F(EvmCallTest, CallToCodeThatWritesStorage) {
  // Callee writes 42 to slot 0:
  // PUSH1 42, PUSH1 0, SSTORE, STOP
  code_provider_.set_code(callee_addr_, {
                                            0x60, 0x2A,  // PUSH1 42
                                            0x60, 0x00,  // PUSH1 0
                                            0x55,        // SSTORE
                                            0x00         // STOP
                                        });

  std::vector<uint8_t> code;
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  push_address(code, callee_addr_);
  code.push_back(0x61);
  code.push_back(0x27);
  code.push_back(0x10);
  code.push_back(0xF1);
  code.push_back(0x00);

  EVM evm(state_context_, Fork::Shanghai);
  const auto env = make_env(code, 500000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  // Callee wrote to its own storage (callee_addr_, slot 0)
  const auto stored = storage_.load(callee_addr_, ethereum::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256(42));
}

TEST_F(EvmCallTest, CallStackUnderflowReturnsError) {
  EVM evm(state_context_, Fork::Shanghai);

  // CALL with insufficient stack items
  const std::vector<uint8_t> code = {0xF1};  // CALL
  const auto env = make_env(code, 100000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::StackUnderflow);
}

TEST_F(EvmCallTest, CallToCodeThatFails) {
  // Callee executes INVALID opcode
  code_provider_.set_code(callee_addr_, {0xFE});  // INVALID

  std::vector<uint8_t> code;
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  push_address(code, callee_addr_);
  code.push_back(0x61);
  code.push_back(0x27);
  code.push_back(0x10);
  code.push_back(0xF1);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x55);
  code.push_back(0x00);

  EVM evm(state_context_, Fork::Shanghai);
  const auto env = make_env(code, 500000);
  const auto result = evm.execute(env);

  // Caller should succeed, but CALL returns 0 (failure)
  EXPECT_EQ(result.status, ExecutionStatus::Success);

  const auto stored = storage_.load(caller_addr_, ethereum::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(stored.get(), types::Uint256::zero());
}

TEST_F(EvmCallTest, CallMemoryExpansionNotDoubleCharged) {
  // Test that memory expansion is charged once for max(args_end, ret_end),
  // not independently for args and ret regions.
  //
  // If double-charged, a CALL with overlapping input/output regions would
  // cost more gas than one with non-overlapping regions of the same max size.
  //
  // We compare two scenarios with max_end = 64 bytes:
  // - Scenario A: args [0, 64), ret [0, 64) - fully overlapping
  // - Scenario B: args [0, 32), ret [32, 64) - adjacent, no overlap
  //
  // If memory cost is computed correctly (once for max_end), both cost the same.
  // If double-charged, scenario B would cost more (two separate expansions).

  code_provider_.set_code(callee_addr_, {0x00});  // STOP

  // Build CALL with specific memory params - all bytecode is same length
  auto build_call_code = [this](uint8_t args_offset, uint8_t args_size, uint8_t ret_offset,
                                uint8_t ret_size) -> std::vector<uint8_t> {
    std::vector<uint8_t> code;
    code.push_back(0x60);
    code.push_back(ret_size);  // PUSH1 retSize
    code.push_back(0x60);
    code.push_back(ret_offset);  // PUSH1 retOffset
    code.push_back(0x60);
    code.push_back(args_size);  // PUSH1 argsSize
    code.push_back(0x60);
    code.push_back(args_offset);  // PUSH1 argsOffset
    code.push_back(0x60);
    code.push_back(0x00);  // PUSH1 0 (value)
    push_address(code, callee_addr_);
    code.push_back(0x62);
    code.push_back(0x0F);
    code.push_back(0x42);
    code.push_back(0x40);  // PUSH3 1000000 (gas - plenty)
    code.push_back(0xF1);  // CALL
    code.push_back(0x00);  // STOP
    return code;
  };

  // Both have max_end = 64, but different region layouts
  const auto code_overlap = build_call_code(0, 64, 0, 64);    // args [0,64), ret [0,64) -> max=64
  const auto code_adjacent = build_call_code(0, 32, 32, 32);  // args [0,32), ret [32,64) -> max=64

  const uint64_t test_gas = 1000000;

  // Reset warm addresses between tests to ensure cold access both times
  accounts_.begin_transaction();

  EVM evm1(state_context_, Fork::Shanghai);
  const auto result_overlap = evm1.execute(make_env(code_overlap, test_gas));

  // Reset again before second test
  accounts_.begin_transaction();

  EVM evm2(state_context_, Fork::Shanghai);
  const auto result_adjacent = evm2.execute(make_env(code_adjacent, test_gas));

  // Both should succeed
  ASSERT_EQ(result_overlap.status, ExecutionStatus::Success);
  ASSERT_EQ(result_adjacent.status, ExecutionStatus::Success);

  // Both should use the same gas - same bytecode length, same max memory end
  // The key test: if memory expansion were double-charged, adjacent would cost more
  EXPECT_EQ(result_overlap.gas_used, result_adjacent.gas_used)
      << "Overlapping [0,64)+[0,64) and adjacent [0,32)+[32,64) regions with same max_end=64 "
         "should cost the same memory expansion gas";
}

TEST_F(EvmCallTest, NestedCallSucceeds) {
  // Third contract that just stops
  const types::Address third_addr{types::Uint256(0x3000)};
  code_provider_.set_code(third_addr, {0x00});  // STOP

  // Callee calls the third contract
  std::vector<uint8_t> callee_code;
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  push_address(callee_code, third_addr);
  callee_code.push_back(0x61);
  callee_code.push_back(0x13);
  callee_code.push_back(0x88);  // PUSH2 5000
  callee_code.push_back(0xF1);
  callee_code.push_back(0x60);
  callee_code.push_back(0x00);
  callee_code.push_back(0x55);
  callee_code.push_back(0x00);
  code_provider_.set_code(callee_addr_, callee_code);

  // Caller calls callee
  std::vector<uint8_t> code;
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  code.push_back(0x60);
  code.push_back(0x00);
  push_address(code, callee_addr_);
  code.push_back(0x61);
  code.push_back(0x27);
  code.push_back(0x10);
  code.push_back(0xF1);
  code.push_back(0x60);
  code.push_back(0x01);  // slot 1
  code.push_back(0x55);
  code.push_back(0x00);

  EVM evm(state_context_, Fork::Shanghai);
  const auto env = make_env(code, 500000);
  const auto result = evm.execute(env);

  EXPECT_EQ(result.status, ExecutionStatus::Success);

  // Both caller and callee should have stored success (1)
  const auto caller_stored = storage_.load(caller_addr_, ethereum::StorageSlot(types::Uint256(1)));
  EXPECT_EQ(caller_stored.get(), types::Uint256(1));

  const auto callee_stored =
      storage_.load(callee_addr_, ethereum::StorageSlot(types::Uint256::zero()));
  EXPECT_EQ(callee_stored.get(), types::Uint256(1));
}

}  // namespace
}  // namespace div0::evm
