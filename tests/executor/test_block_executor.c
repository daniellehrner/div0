#include "test_block_executor.h"

#include "div0/executor/block_executor.h"
#include "div0/state/world_state.h"

#include "unity.h"

// External arena from main test file
extern div0_arena_t test_arena;

// Helper to create a test address
static address_t make_test_address(uint8_t seed) {
  address_t addr = {0};
  for (size_t i = 0; i < 20; i++) {
    addr.bytes[i] = (uint8_t)(seed + i);
  }
  return addr;
}

// Helper to create a simple legacy transaction
static void make_legacy_tx(transaction_t *tx, uint64_t nonce, uint64_t gas_limit, uint256_t value,
                           const address_t *to) {
  tx->type = TX_TYPE_LEGACY;
  legacy_tx_init(&tx->legacy);
  tx->legacy.nonce = nonce;
  tx->legacy.gas_limit = gas_limit;
  tx->legacy.gas_price = uint256_from_u64(1000000000); // 1 gwei
  tx->legacy.value = value;
  if (to) {
    tx->legacy.to = (address_t *)div0_arena_alloc(&test_arena, sizeof(address_t));
    *tx->legacy.to = *to;
  } else {
    tx->legacy.to = nullptr;
  }
}

// ===========================================================================
// Intrinsic gas calculation tests
// ===========================================================================

void test_intrinsic_gas_simple_transfer(void) {
  transaction_t tx;
  address_t to = make_test_address(0x01);
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000), &to);

  uint64_t gas = tx_intrinsic_gas(&tx);
  TEST_ASSERT_EQUAL_UINT64(21000, gas); // Base tx cost only
}

void test_intrinsic_gas_with_calldata(void) {
  transaction_t tx;
  address_t to = make_test_address(0x02);
  make_legacy_tx(&tx, 0, 100000, uint256_zero(), &to);

  // Add calldata: 4 zero bytes + 4 non-zero bytes
  uint8_t calldata[] = {0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78};
  tx.legacy.data.data = calldata;
  tx.legacy.data.size = sizeof(calldata);

  uint64_t gas = tx_intrinsic_gas(&tx);
  // 21000 + (4 * 4) + (4 * 16) = 21000 + 16 + 64 = 21080
  TEST_ASSERT_EQUAL_UINT64(21080, gas);
}

void test_intrinsic_gas_contract_creation(void) {
  transaction_t tx;
  make_legacy_tx(&tx, 0, 100000, uint256_zero(), nullptr); // nullptr = contract creation

  // Add initcode
  uint8_t initcode[64];
  __builtin___memset_chk(initcode, 0x60, sizeof(initcode), sizeof(initcode)); // Non-zero bytes
  tx.legacy.data.data = initcode;
  tx.legacy.data.size = sizeof(initcode);

  uint64_t gas = tx_intrinsic_gas(&tx);
  // 21000 base + 32000 create + (64 * 16) calldata + 2*ceil(64/32) initcode cost
  // = 21000 + 32000 + 1024 + 4 = 54028
  TEST_ASSERT_EQUAL_UINT64(54028, gas);
}

void test_intrinsic_gas_with_access_list(void) {
  transaction_t tx;
  tx.type = TX_TYPE_EIP2930;
  eip2930_tx_init(&tx.eip2930);

  address_t to = make_test_address(0x03);
  tx.eip2930.nonce = 0;
  tx.eip2930.gas_limit = 100000;
  tx.eip2930.gas_price = uint256_from_u64(1000000000);
  tx.eip2930.to = (address_t *)div0_arena_alloc(&test_arena, sizeof(address_t));
  *tx.eip2930.to = to;

  // Create access list with 1 address and 2 storage keys
  access_list_alloc_entries(&tx.eip2930.access_list, 1, &test_arena);
  tx.eip2930.access_list.entries[0].address = make_test_address(0x04);
  access_list_entry_alloc_keys(&tx.eip2930.access_list.entries[0], 2, &test_arena);

  uint64_t gas = tx_intrinsic_gas(&tx);
  // 21000 base + 2400 (1 address) + 3800 (2 keys * 1900)
  TEST_ASSERT_EQUAL_UINT64(27200, gas);
}

// ===========================================================================
// Transaction validation tests
// ===========================================================================

void test_validation_valid_tx(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Set up sender with sufficient balance (need gas_limit * gas_price + value)
  // gas_limit=21000, gas_price=1e9, value=1000 => need ~21e12 + 1000
  address_t sender = make_test_address(0x10);
  state_set_balance(state, &sender, uint256_from_u64(100000000000000)); // 100e12 = 100k gwei

  // Create block context
  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000); // 0.1 gwei

  // Create simple transfer transaction
  transaction_t tx;
  address_t to = make_test_address(0x11);
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000), &to);

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  // Create executor
  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_VALID, err);

  world_state_destroy(ws);
}

void test_validation_nonce_too_low(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  address_t sender = make_test_address(0x20);
  state_set_balance(state, &sender, uint256_from_u64(1000000000000));
  state_set_nonce(state, &sender, 5); // Sender nonce is 5

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000);

  transaction_t tx;
  address_t to = make_test_address(0x21);
  make_legacy_tx(&tx, 3, 21000, uint256_from_u64(1000), &to); // TX nonce is 3 (too low)

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_ERR_NONCE_TOO_LOW, err);

  world_state_destroy(ws);
}

void test_validation_nonce_too_high(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  address_t sender = make_test_address(0x30);
  state_set_balance(state, &sender, uint256_from_u64(1000000000000));
  state_set_nonce(state, &sender, 5); // Sender nonce is 5

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000);

  transaction_t tx;
  address_t to = make_test_address(0x31);
  make_legacy_tx(&tx, 10, 21000, uint256_from_u64(1000), &to); // TX nonce is 10 (too high)

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_ERR_NONCE_TOO_HIGH, err);

  world_state_destroy(ws);
}

void test_validation_insufficient_balance(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  address_t sender = make_test_address(0x40);
  state_set_balance(state, &sender, uint256_from_u64(1000)); // Very low balance

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000);

  transaction_t tx;
  address_t to = make_test_address(0x41);
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000000000000), &to); // High value

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_ERR_INSUFFICIENT_BALANCE, err);

  world_state_destroy(ws);
}

void test_validation_intrinsic_gas_too_low(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  address_t sender = make_test_address(0x50);
  state_set_balance(state, &sender, uint256_from_u64(1000000000000));

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000);

  transaction_t tx;
  address_t to = make_test_address(0x51);
  make_legacy_tx(&tx, 0, 20000, uint256_from_u64(1000), &to); // Gas limit below intrinsic

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_ERR_INTRINSIC_GAS, err);

  world_state_destroy(ws);
}

void test_validation_gas_limit_exceeded(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  address_t sender = make_test_address(0x60);
  state_set_balance(state, &sender, uint256_from_u64(1000000000000000)); // Large balance

  block_context_t block = {0};
  block.gas_limit = 1000000; // Low block gas limit
  block.base_fee = uint256_from_u64(100000000);

  transaction_t tx;
  address_t to = make_test_address(0x61);
  make_legacy_tx(&tx, 0, 2000000, uint256_from_u64(1000), &to); // TX gas > block gas

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  tx_validation_error_t err = block_executor_validate_tx(&exec, &btx, 0);
  TEST_ASSERT_EQUAL(TX_ERR_GAS_LIMIT_EXCEEDED, err);

  world_state_destroy(ws);
}

// ===========================================================================
// Block executor tests
// ===========================================================================

void test_block_executor_empty_block(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(100000000);

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, nullptr, 0, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_EQUAL_UINT64(0, result.gas_used);
  TEST_ASSERT_EQUAL_size_t(0, result.receipt_count);
  TEST_ASSERT_EQUAL_size_t(0, result.rejected_count);

  world_state_destroy(ws);
}

void test_block_executor_single_tx(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Set up sender with sufficient balance (gas_limit * gas_price + value)
  // gas_limit=21000, gas_price=1e9, value=1000 => need ~21e12 + 1000
  address_t sender = make_test_address(0x70);
  state_set_balance(state, &sender, uint256_from_u64(100000000000000)); // 100e12 = 100k gwei

  // Set up recipient
  address_t recipient = make_test_address(0x71);

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(1000000000); // 1 gwei base fee
  block.coinbase = make_test_address(0x72);

  transaction_t tx;
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000), &recipient);

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, &btx, 1, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_EQUAL_size_t(1, result.receipt_count);
  TEST_ASSERT_EQUAL_size_t(0, result.rejected_count);
  TEST_ASSERT_TRUE(result.receipts[0].success);
  TEST_ASSERT_EQUAL_UINT64(21000, result.receipts[0].gas_used);

  // Verify sender balance decreased (gas_cost = 21000 * 1e9 = 2.1e10 + value)
  uint256_t sender_balance = state_get_balance(state, &sender);
  // Started with 100e12, paid ~21e9 gas + 1000 value, balance should be less
  TEST_ASSERT_TRUE(uint256_lt(sender_balance, uint256_from_u64(100000000000000)));

  world_state_destroy(ws);
}

void test_block_executor_recipient_balance(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Set up sender with sufficient balance
  address_t sender = make_test_address(0x80);
  state_set_balance(state, &sender, uint256_from_u64(100000000000000)); // 100e12

  // Recipient starts with zero balance
  address_t recipient = make_test_address(0x81);
  uint256_t initial_recipient_balance = state_get_balance(state, &recipient);
  TEST_ASSERT_TRUE(uint256_is_zero(initial_recipient_balance));

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(1000000000); // 1 gwei
  block.coinbase = make_test_address(0x82);

  // Transfer 1000 wei to recipient
  transaction_t tx;
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000), &recipient);

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, &btx, 1, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_TRUE(result.receipts[0].success);

  // Verify recipient received the 1000 wei
  uint256_t recipient_balance = state_get_balance(state, &recipient);
  TEST_ASSERT_TRUE(uint256_eq(recipient_balance, uint256_from_u64(1000)));

  world_state_destroy(ws);
}

void test_block_executor_coinbase_fee(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Set up sender
  address_t sender = make_test_address(0x90);
  state_set_balance(state, &sender, uint256_from_u64(100000000000000)); // 100e12

  // Coinbase starts with zero balance
  address_t coinbase = make_test_address(0x91);
  uint256_t initial_coinbase_balance = state_get_balance(state, &coinbase);
  TEST_ASSERT_TRUE(uint256_is_zero(initial_coinbase_balance));

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(500000000); // 0.5 gwei base fee
  block.coinbase = coinbase;

  // Create tx with gas_price = 1 gwei (priority fee = 0.5 gwei)
  address_t recipient = make_test_address(0x92);
  transaction_t tx;
  make_legacy_tx(&tx, 0, 21000, uint256_from_u64(1000), &recipient);

  block_tx_t btx = {.tx = &tx, .sender = sender, .sender_recovered = true, .original_index = 0};

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, &btx, 1, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_TRUE(result.receipts[0].success);

  // Verify coinbase received priority fee: (1 gwei - 0.5 gwei) * 21000 = 10.5e9 wei
  uint256_t coinbase_balance = state_get_balance(state, &coinbase);
  uint256_t expected_fee = uint256_from_u64(500000000ULL * 21000ULL); // 10.5e9
  TEST_ASSERT_TRUE(uint256_eq(coinbase_balance, expected_fee));

  world_state_destroy(ws);
}

void test_block_executor_multiple_txs(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Set up sender with enough balance for 3 transactions
  address_t sender = make_test_address(0xA0);
  state_set_balance(state, &sender, uint256_from_u64(300000000000000)); // 300e12

  address_t recipient = make_test_address(0xA1);

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(1000000000); // 1 gwei
  block.coinbase = make_test_address(0xA2);

  // Create 3 transactions
  transaction_t tx0, tx1, tx2;
  make_legacy_tx(&tx0, 0, 21000, uint256_from_u64(1000), &recipient);
  make_legacy_tx(&tx1, 1, 21000, uint256_from_u64(2000), &recipient);
  make_legacy_tx(&tx2, 2, 21000, uint256_from_u64(3000), &recipient);

  block_tx_t btxs[3] = {
      {.tx = &tx0, .sender = sender, .sender_recovered = true, .original_index = 0},
      {.tx = &tx1, .sender = sender, .sender_recovered = true, .original_index = 1},
      {.tx = &tx2, .sender = sender, .sender_recovered = true, .original_index = 2},
  };

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, btxs, 3, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_EQUAL_size_t(3, result.receipt_count);
  TEST_ASSERT_EQUAL_size_t(0, result.rejected_count);
  TEST_ASSERT_EQUAL_UINT64(63000, result.gas_used); // 3 * 21000

  // Verify cumulative gas in receipts
  TEST_ASSERT_EQUAL_UINT64(21000, result.receipts[0].cumulative_gas);
  TEST_ASSERT_EQUAL_UINT64(42000, result.receipts[1].cumulative_gas);
  TEST_ASSERT_EQUAL_UINT64(63000, result.receipts[2].cumulative_gas);

  // Verify recipient received all value transfers
  uint256_t recipient_balance = state_get_balance(state, &recipient);
  TEST_ASSERT_TRUE(uint256_eq(recipient_balance, uint256_from_u64(6000))); // 1000+2000+3000

  // Verify sender nonce incremented 3 times
  uint64_t sender_nonce = state_get_nonce(state, &sender);
  TEST_ASSERT_EQUAL_UINT64(3, sender_nonce);

  world_state_destroy(ws);
}

void test_block_executor_mixed_valid_rejected(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *state = world_state_access(ws);

  // Sender 1 has enough balance
  address_t sender1 = make_test_address(0xB0);
  state_set_balance(state, &sender1, uint256_from_u64(100000000000000)); // 100e12

  // Sender 2 has insufficient balance
  address_t sender2 = make_test_address(0xB1);
  state_set_balance(state, &sender2, uint256_from_u64(1000)); // Very low balance

  // Sender 3 has wrong nonce
  address_t sender3 = make_test_address(0xB2);
  state_set_balance(state, &sender3, uint256_from_u64(100000000000000));
  state_set_nonce(state, &sender3, 5); // Nonce is 5

  address_t recipient = make_test_address(0xB3);

  block_context_t block = {0};
  block.gas_limit = 30000000;
  block.base_fee = uint256_from_u64(1000000000);
  block.coinbase = make_test_address(0xB4);

  // tx0: valid transaction from sender1
  transaction_t tx0;
  make_legacy_tx(&tx0, 0, 21000, uint256_from_u64(1000), &recipient);

  // tx1: insufficient balance from sender2
  transaction_t tx1;
  make_legacy_tx(&tx1, 0, 21000, uint256_from_u64(1000000000000000), &recipient);

  // tx2: wrong nonce from sender3 (expects 5, tx has 0)
  transaction_t tx2;
  make_legacy_tx(&tx2, 0, 21000, uint256_from_u64(1000), &recipient);

  // tx3: another valid transaction from sender1
  transaction_t tx3;
  make_legacy_tx(&tx3, 1, 21000, uint256_from_u64(2000), &recipient);

  block_tx_t btxs[4] = {
      {.tx = &tx0, .sender = sender1, .sender_recovered = true, .original_index = 0},
      {.tx = &tx1, .sender = sender2, .sender_recovered = true, .original_index = 1},
      {.tx = &tx2, .sender = sender3, .sender_recovered = true, .original_index = 2},
      {.tx = &tx3, .sender = sender1, .sender_recovered = true, .original_index = 3},
  };

  evm_t evm;
  evm_init(&evm, &test_arena);
  block_executor_t exec;
  block_executor_init(&exec, state, &block, &evm, &test_arena, 1);

  block_exec_result_t result;
  bool success = block_executor_run(&exec, btxs, 4, &result);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_EQUAL_size_t(2, result.receipt_count);  // tx0 and tx3
  TEST_ASSERT_EQUAL_size_t(2, result.rejected_count); // tx1 and tx2

  // Verify rejected errors
  TEST_ASSERT_EQUAL(TX_ERR_INSUFFICIENT_BALANCE, result.rejected[0].error);
  TEST_ASSERT_EQUAL_size_t(1, result.rejected[0].index); // Original index 1

  TEST_ASSERT_EQUAL(TX_ERR_NONCE_TOO_LOW, result.rejected[1].error);
  TEST_ASSERT_EQUAL_size_t(2, result.rejected[1].index); // Original index 2

  // Verify gas used only counts successful transactions
  TEST_ASSERT_EQUAL_UINT64(42000, result.gas_used);

  world_state_destroy(ws);
}
