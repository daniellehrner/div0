// Unit tests for storage opcodes: SLOAD, SSTORE

#include <div0/db/memory_database.h>
#include <div0/evm/gas/dynamic_costs.h>
#include <div0/evm/stack.h>
#include <div0/state/memory_storage_provider.h>
#include <div0/types/address.h>
#include <div0/types/uint256.h>
#include <gtest/gtest.h>

#include "../src/evm/opcodes/storage.h"

namespace div0::evm::opcodes {

using types::Address;
using types::Uint256;

// Test fixture with storage setup
class StorageOpcodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    storage_ = std::make_unique<state::MemoryStorageProvider>(database_);
    storage_->begin_transaction();
  }

  [[nodiscard]] const Address& get_test_address() const { return test_address_; }

  [[nodiscard]] state::StorageProvider& get_storage_provider() const { return *storage_; }

 private:
  db::MemoryDatabase database_;
  std::unique_ptr<state::MemoryStorageProvider> storage_;
  const Address test_address_{Uint256(0x1234)};
};

// =============================================================================
// SLOAD Opcode Tests
// =============================================================================

TEST_F(StorageOpcodeTest, SloadEmptySlot) {
  Stack stack;
  uint64_t gas = 10000;
  (void)stack.push(Uint256(42));  // slot key

  const auto result =
      sload(get_test_address(), stack, gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack[0], Uint256(0));  // Empty slot returns 0
}

TEST_F(StorageOpcodeTest, SloadAfterSstore) {
  // First store a value
  Stack store_stack;
  uint64_t store_gas = 100000;
  (void)store_stack.push(Uint256(999));  // value
  (void)store_stack.push(Uint256(42));   // slot key

  const auto store_result = sstore(get_test_address(), store_stack, store_gas,
                                   get_storage_provider(), gas::sstore::eip2929);
  EXPECT_EQ(store_result, ExecutionStatus::Success);

  // Now load from the same slot
  Stack load_stack;
  uint64_t load_gas = 10000;
  (void)load_stack.push(Uint256(42));  // same slot key

  const auto load_result =
      sload(get_test_address(), load_stack, load_gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(load_result, ExecutionStatus::Success);
  EXPECT_EQ(load_stack.size(), 1);
  EXPECT_EQ(load_stack[0], Uint256(999));  // Loaded value matches stored value
}

TEST_F(StorageOpcodeTest, SloadColdAccess) {
  Stack stack;
  uint64_t gas = 10000;
  const uint64_t initial_gas = gas;
  (void)stack.push(Uint256(1));

  const auto result =
      sload(get_test_address(), stack, gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(initial_gas - gas, 2100);  // Cold access cost
}

TEST_F(StorageOpcodeTest, SloadWarmAccess) {
  // First access (cold)
  Stack stack1;
  uint64_t gas1 = 10000;
  (void)stack1.push(Uint256(1));
  (void)sload(get_test_address(), stack1, gas1, get_storage_provider(), gas::sload::eip2929);

  // Second access (warm)
  Stack stack2;
  uint64_t gas2 = 10000;
  const uint64_t initial_gas = gas2;
  (void)stack2.push(Uint256(1));  // Same slot

  const auto result =
      sload(get_test_address(), stack2, gas2, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(initial_gas - gas2, 100);  // Warm access cost
}

TEST_F(StorageOpcodeTest, SloadStackUnderflow) {
  Stack stack;  // Empty stack
  uint64_t gas = 10000;

  const auto result =
      sload(get_test_address(), stack, gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
  EXPECT_EQ(gas, 10000);  // Gas unchanged
}

TEST_F(StorageOpcodeTest, SloadOutOfGas) {
  Stack stack;
  uint64_t gas = 50;  // Not enough for cold access (2100)
  (void)stack.push(Uint256(1));

  const auto result =
      sload(get_test_address(), stack, gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(gas, 50);  // Gas unchanged on failure
}

TEST_F(StorageOpcodeTest, SloadLargeSlotKey) {
  Stack stack;
  uint64_t gas = 10000;
  constexpr auto LARGE_KEY = Uint256::max();
  (void)stack.push(LARGE_KEY);

  const auto result =
      sload(get_test_address(), stack, gas, get_storage_provider(), gas::sload::eip2929);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack[0], Uint256(0));  // Empty slot
}

// =============================================================================
// SSTORE Opcode Tests
// =============================================================================

TEST_F(StorageOpcodeTest, SstoreBasic) {
  Stack stack;
  uint64_t gas = 100000;
  (void)stack.push(Uint256(42));   // value
  (void)stack.push(Uint256(100));  // slot key

  const auto result =
      sstore(get_test_address(), stack, gas, get_storage_provider(), gas::sstore::eip2929);

  EXPECT_EQ(result, ExecutionStatus::Success);
  EXPECT_EQ(stack.size(), 0);  // Both values popped
}

TEST_F(StorageOpcodeTest, SstoreZeroValue) {
  // First store a non-zero value
  Stack store_stack;
  uint64_t store_gas = 100000;
  (void)store_stack.push(Uint256(999));
  (void)store_stack.push(Uint256(42));
  (void)sstore(get_test_address(), store_stack, store_gas, get_storage_provider(),
               gas::sstore::eip2929);

  // Then store zero to clear it
  Stack clear_stack;
  uint64_t clear_gas = 100000;
  (void)clear_stack.push(Uint256(0));
  (void)clear_stack.push(Uint256(42));

  const auto result = sstore(get_test_address(), clear_stack, clear_gas, get_storage_provider(),
                             gas::sstore::eip2929);
  EXPECT_EQ(result, ExecutionStatus::Success);

  // Verify slot is now zero
  Stack load_stack;
  uint64_t load_gas = 10000;
  (void)load_stack.push(Uint256(42));
  (void)sload(get_test_address(), load_stack, load_gas, get_storage_provider(),
              gas::sload::eip2929);

  EXPECT_EQ(load_stack[0], Uint256(0));
}

TEST_F(StorageOpcodeTest, SstoreStackUnderflowEmpty) {
  Stack stack;  // Empty stack
  uint64_t gas = 100000;

  const auto result =
      sstore(get_test_address(), stack, gas, get_storage_provider(), gas::sstore::eip2929);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST_F(StorageOpcodeTest, SstoreStackUnderflowOneItem) {
  Stack stack;
  uint64_t gas = 100000;
  (void)stack.push(Uint256(42));  // Only one item

  const auto result =
      sstore(get_test_address(), stack, gas, get_storage_provider(), gas::sstore::eip2929);

  EXPECT_EQ(result, ExecutionStatus::StackUnderflow);
}

TEST_F(StorageOpcodeTest, SstoreOutOfGas) {
  Stack stack;
  uint64_t gas = 50;  // Not enough gas
  (void)stack.push(Uint256(42));
  (void)stack.push(Uint256(100));

  const auto result =
      sstore(get_test_address(), stack, gas, get_storage_provider(), gas::sstore::eip2929);

  EXPECT_EQ(result, ExecutionStatus::OutOfGas);
  EXPECT_EQ(stack.size(), 2);  // Stack unchanged on failure
}

TEST_F(StorageOpcodeTest, SstoreMultipleSlots) {
  // Store to slot 1
  Stack stack1;
  uint64_t gas1 = 100000;
  (void)stack1.push(Uint256(111));
  (void)stack1.push(Uint256(1));
  (void)sstore(get_test_address(), stack1, gas1, get_storage_provider(), gas::sstore::eip2929);

  // Store to slot 2
  Stack stack2;
  uint64_t gas2 = 100000;
  (void)stack2.push(Uint256(222));
  (void)stack2.push(Uint256(2));
  (void)sstore(get_test_address(), stack2, gas2, get_storage_provider(), gas::sstore::eip2929);

  // Store to slot 3
  Stack stack3;
  uint64_t gas3 = 100000;
  (void)stack3.push(Uint256(333));
  (void)stack3.push(Uint256(3));
  (void)sstore(get_test_address(), stack3, gas3, get_storage_provider(), gas::sstore::eip2929);

  // Verify all slots
  Stack load1, load2, load3;
  uint64_t load_gas = 10000;

  (void)load1.push(Uint256(1));
  (void)sload(get_test_address(), load1, load_gas, get_storage_provider(), gas::sload::eip2929);
  EXPECT_EQ(load1[0], Uint256(111));

  load_gas = 10000;
  (void)load2.push(Uint256(2));
  (void)sload(get_test_address(), load2, load_gas, get_storage_provider(), gas::sload::eip2929);
  EXPECT_EQ(load2[0], Uint256(222));

  load_gas = 10000;
  (void)load3.push(Uint256(3));
  (void)sload(get_test_address(), load3, load_gas, get_storage_provider(), gas::sload::eip2929);
  EXPECT_EQ(load3[0], Uint256(333));
}

TEST_F(StorageOpcodeTest, SstoreOverwrite) {
  // Store initial value
  Stack stack1;
  uint64_t gas1 = 100000;
  (void)stack1.push(Uint256(100));
  (void)stack1.push(Uint256(42));
  (void)sstore(get_test_address(), stack1, gas1, get_storage_provider(), gas::sstore::eip2929);

  // Overwrite with new value
  Stack stack2;
  uint64_t gas2 = 100000;
  (void)stack2.push(Uint256(200));
  (void)stack2.push(Uint256(42));  // Same slot
  (void)sstore(get_test_address(), stack2, gas2, get_storage_provider(), gas::sstore::eip2929);

  // Verify new value
  Stack load_stack;
  uint64_t load_gas = 10000;
  (void)load_stack.push(Uint256(42));
  (void)sload(get_test_address(), load_stack, load_gas, get_storage_provider(),
              gas::sload::eip2929);

  EXPECT_EQ(load_stack[0], Uint256(200));  // New value, not old
}

TEST_F(StorageOpcodeTest, SstoreDifferentAddresses) {
  constexpr Address ADDRESS1{Uint256(0x1111)};
  constexpr Address ADDRESS2{Uint256(0x2222)};

  // Store to address1
  Stack stack1;
  uint64_t gas1 = 100000;
  (void)stack1.push(Uint256(111));
  (void)stack1.push(Uint256(1));
  (void)sstore(ADDRESS1, stack1, gas1, get_storage_provider(), gas::sstore::eip2929);

  // Store to address2 (same slot, different address)
  Stack stack2;
  uint64_t gas2 = 100000;
  (void)stack2.push(Uint256(222));
  (void)stack2.push(Uint256(1));
  (void)sstore(ADDRESS2, stack2, gas2, get_storage_provider(), gas::sstore::eip2929);

  // Verify address isolation
  Stack load1;
  uint64_t load_gas1 = 10000;
  (void)load1.push(Uint256(1));
  (void)sload(ADDRESS1, load1, load_gas1, get_storage_provider(), gas::sload::eip2929);
  EXPECT_EQ(load1[0], Uint256(111));

  Stack load2;
  uint64_t load_gas2 = 10000;
  (void)load2.push(Uint256(1));
  (void)sload(ADDRESS2, load2, load_gas2, get_storage_provider(), gas::sload::eip2929);
  EXPECT_EQ(load2[0], Uint256(222));
}

TEST_F(StorageOpcodeTest, SstoreLargeValues) {
  Stack stack;
  uint64_t gas = 100000;
  constexpr auto LARGE_VALUE = Uint256::max();
  constexpr auto LARGE_KEY = Uint256(UINT64_MAX, UINT64_MAX, 0, 0);
  (void)stack.push(LARGE_VALUE);
  (void)stack.push(LARGE_KEY);

  const auto result =
      sstore(get_test_address(), stack, gas, get_storage_provider(), gas::sstore::eip2929);
  EXPECT_EQ(result, ExecutionStatus::Success);

  // Verify
  Stack load_stack;
  uint64_t load_gas = 10000;
  (void)load_stack.push(LARGE_KEY);
  (void)sload(get_test_address(), load_stack, load_gas, get_storage_provider(),
              gas::sload::eip2929);

  EXPECT_EQ(load_stack[0], LARGE_VALUE);
}

}  // namespace div0::evm::opcodes
