#include "test_world_state.h"

#include "div0/state/state_access.h"
#include "div0/state/world_state.h"
#include "div0/trie/node.h"

#include "unity.h"

#include <string.h>

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

// ===========================================================================
// World state creation tests
// ===========================================================================

void test_world_state_create(void) {
  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  world_state_destroy(ws);
}

void test_world_state_empty_root(void) {
  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  // Empty world state should have empty MPT root
  hash_t root = world_state_root(ws);
  TEST_ASSERT_TRUE(hash_equal(&root, &MPT_EMPTY_ROOT));

  world_state_destroy(ws);
}

// ===========================================================================
// Account operations tests
// ===========================================================================

void test_world_state_get_nonexistent_account(void) {
  world_state_t *ws = world_state_create(&test_arena);
  address_t addr = make_test_address(0x42);

  account_t acc;
  bool exists = world_state_get_account(ws, &addr, &acc);

  TEST_ASSERT_FALSE(exists);
  // Output should be empty account
  TEST_ASSERT_TRUE(account_is_empty(&acc));

  world_state_destroy(ws);
}

void test_world_state_set_and_get_account(void) {
  world_state_t *ws = world_state_create(&test_arena);
  address_t addr = make_test_address(0x01);

  // Set account with some values
  account_t acc = {
      .nonce = 10,
      .balance = uint256_from_u64(1000000),
      .storage_root = MPT_EMPTY_ROOT,
      .code_hash = EMPTY_CODE_HASH,
  };
  bool result = world_state_set_account(ws, &addr, &acc);
  TEST_ASSERT_TRUE(result);

  // Retrieve and verify
  account_t retrieved;
  bool exists = world_state_get_account(ws, &addr, &retrieved);

  TEST_ASSERT_TRUE(exists);
  TEST_ASSERT_EQUAL_UINT64(acc.nonce, retrieved.nonce);
  TEST_ASSERT_TRUE(uint256_eq(acc.balance, retrieved.balance));

  world_state_destroy(ws);
}

void test_world_state_delete_empty_account(void) {
  world_state_t *ws = world_state_create(&test_arena);
  address_t addr = make_test_address(0x02);

  // Create non-empty account
  account_t acc = {
      .nonce = 1,
      .balance = uint256_from_u64(100),
      .storage_root = MPT_EMPTY_ROOT,
      .code_hash = EMPTY_CODE_HASH,
  };
  world_state_set_account(ws, &addr, &acc);

  // Setting empty account should delete it (EIP-161)
  account_t empty = account_empty();
  world_state_set_account(ws, &addr, &empty);

  // Should no longer exist
  account_t retrieved;
  bool exists = world_state_get_account(ws, &addr, &retrieved);
  TEST_ASSERT_FALSE(exists);

  world_state_destroy(ws);
}

// ===========================================================================
// Balance operations tests
// ===========================================================================

void test_world_state_balance_operations(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x10);

  // Initially zero balance (account doesn't exist)
  uint256_t bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_is_zero(bal));

  // Set balance
  uint256_t new_bal = uint256_from_u64(12345);
  access->vtable->set_balance(access, &addr, new_bal);

  // Verify
  bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, new_bal));

  world_state_destroy(ws);
}

void test_world_state_add_balance(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x11);

  // Set initial balance
  access->vtable->set_balance(access, &addr, uint256_from_u64(100));

  // Add balance
  bool result = access->vtable->add_balance(access, &addr, uint256_from_u64(50));
  TEST_ASSERT_TRUE(result);

  // Verify
  uint256_t bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, uint256_from_u64(150)));

  world_state_destroy(ws);
}

void test_world_state_sub_balance(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x12);

  // Set initial balance
  access->vtable->set_balance(access, &addr, uint256_from_u64(100));

  // Subtract balance
  bool result = access->vtable->sub_balance(access, &addr, uint256_from_u64(30));
  TEST_ASSERT_TRUE(result);

  // Verify
  uint256_t bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, uint256_from_u64(70)));

  // Try to subtract more than available
  result = access->vtable->sub_balance(access, &addr, uint256_from_u64(100));
  TEST_ASSERT_FALSE(result);

  // Balance should be unchanged
  bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, uint256_from_u64(70)));

  world_state_destroy(ws);
}

// ===========================================================================
// Nonce operations tests
// ===========================================================================

void test_world_state_nonce_operations(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x20);

  // Initially zero nonce
  uint64_t nonce = access->vtable->get_nonce(access, &addr);
  TEST_ASSERT_EQUAL_UINT64(0, nonce);

  // Set nonce
  access->vtable->set_nonce(access, &addr, 42);

  // Verify
  nonce = access->vtable->get_nonce(access, &addr);
  TEST_ASSERT_EQUAL_UINT64(42, nonce);

  world_state_destroy(ws);
}

void test_world_state_increment_nonce(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x21);

  // Set initial nonce
  access->vtable->set_nonce(access, &addr, 5);

  // Increment
  uint64_t old_nonce = access->vtable->increment_nonce(access, &addr);
  TEST_ASSERT_EQUAL_UINT64(5, old_nonce);

  // Verify new nonce
  uint64_t nonce = access->vtable->get_nonce(access, &addr);
  TEST_ASSERT_EQUAL_UINT64(6, nonce);

  world_state_destroy(ws);
}

// ===========================================================================
// Code operations tests
// ===========================================================================

void test_world_state_code_operations(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x30);

  // Initially no code
  bytes_t code = access->vtable->get_code(access, &addr);
  TEST_ASSERT_EQUAL_size_t(0, code.size);

  size_t size = access->vtable->get_code_size(access, &addr);
  TEST_ASSERT_EQUAL_size_t(0, size);

  // Set code
  uint8_t bytecode[] = {0x60, 0x00, 0x60, 0x00, 0xf3}; // PUSH1 0 PUSH1 0 RETURN
  access->vtable->set_code(access, &addr, bytecode, sizeof(bytecode));

  // Verify code
  code = access->vtable->get_code(access, &addr);
  TEST_ASSERT_EQUAL_size_t(sizeof(bytecode), code.size);
  TEST_ASSERT_EQUAL_MEMORY(bytecode, code.data, sizeof(bytecode));

  // Verify code size
  size = access->vtable->get_code_size(access, &addr);
  TEST_ASSERT_EQUAL_size_t(sizeof(bytecode), size);

  world_state_destroy(ws);
}

void test_world_state_code_hash(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x31);

  // Empty account has empty code hash
  hash_t hash = access->vtable->get_code_hash(access, &addr);
  TEST_ASSERT_TRUE(hash_equal(&hash, &EMPTY_CODE_HASH));

  // Set some code
  uint8_t bytecode[] = {0x60, 0x42}; // PUSH1 0x42
  access->vtable->set_code(access, &addr, bytecode, sizeof(bytecode));

  // Code hash should change
  hash = access->vtable->get_code_hash(access, &addr);
  TEST_ASSERT_FALSE(hash_equal(&hash, &EMPTY_CODE_HASH));

  world_state_destroy(ws);
}

// ===========================================================================
// Storage operations tests
// ===========================================================================

void test_world_state_storage_operations(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x40);
  uint256_t slot = uint256_from_u64(1);

  // Initially zero
  uint256_t val = access->vtable->get_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_is_zero(val));

  // Set storage
  uint256_t new_val = uint256_from_u64(0xDEADBEEF);
  access->vtable->set_storage(access, &addr, slot, new_val);

  // Verify
  val = access->vtable->get_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(val, new_val));

  // Clear storage (set to zero)
  access->vtable->set_storage(access, &addr, slot, uint256_zero());
  val = access->vtable->get_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_is_zero(val));

  world_state_destroy(ws);
}

void test_world_state_storage_multiple_slots(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x41);

  // Set multiple slots
  for (uint64_t i = 0; i < 10; i++) {
    uint256_t slot = uint256_from_u64(i);
    uint256_t val = uint256_from_u64(i * 100);
    access->vtable->set_storage(access, &addr, slot, val);
  }

  // Verify all
  for (uint64_t i = 0; i < 10; i++) {
    uint256_t slot = uint256_from_u64(i);
    uint256_t val = access->vtable->get_storage(access, &addr, slot);
    TEST_ASSERT_TRUE(uint256_eq(val, uint256_from_u64(i * 100)));
  }

  world_state_destroy(ws);
}

// ===========================================================================
// EIP-2929 warm/cold tracking tests
// ===========================================================================

void test_world_state_warm_address(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x50);

  // Initially cold
  TEST_ASSERT_FALSE(access->vtable->is_address_warm(access, &addr));

  // Warm it up - returns true because address was cold (first access)
  bool was_cold = access->vtable->warm_address(access, &addr);
  TEST_ASSERT_TRUE(was_cold);

  // Now it's warm
  TEST_ASSERT_TRUE(access->vtable->is_address_warm(access, &addr));

  // Warming again returns false (already warm, not cold)
  was_cold = access->vtable->warm_address(access, &addr);
  TEST_ASSERT_FALSE(was_cold);

  world_state_destroy(ws);
}

void test_world_state_warm_slot(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x51);
  uint256_t slot = uint256_from_u64(123);

  // Initially cold
  TEST_ASSERT_FALSE(access->vtable->is_slot_warm(access, &addr, slot));

  // Warm it up - returns true because slot was cold (first access)
  bool was_cold = access->vtable->warm_slot(access, &addr, slot);
  TEST_ASSERT_TRUE(was_cold);

  // Now it's warm
  TEST_ASSERT_TRUE(access->vtable->is_slot_warm(access, &addr, slot));

  // Warming again returns false (already warm)
  was_cold = access->vtable->warm_slot(access, &addr, slot);
  TEST_ASSERT_FALSE(was_cold);

  // Different slot is still cold
  uint256_t slot2 = uint256_from_u64(456);
  TEST_ASSERT_FALSE(access->vtable->is_slot_warm(access, &addr, slot2));

  world_state_destroy(ws);
}

// ===========================================================================
// State root tests
// ===========================================================================

void test_world_state_root_changes(void) {
  world_state_t *ws = world_state_create(&test_arena);
  address_t addr = make_test_address(0x60);

  // Get initial root
  hash_t root1 = world_state_root(ws);
  TEST_ASSERT_TRUE(hash_equal(&root1, &MPT_EMPTY_ROOT));

  // Add an account
  account_t acc = {
      .nonce = 1,
      .balance = uint256_from_u64(1000),
      .storage_root = MPT_EMPTY_ROOT,
      .code_hash = EMPTY_CODE_HASH,
  };
  world_state_set_account(ws, &addr, &acc);

  // Root should change
  hash_t root2 = world_state_root(ws);
  TEST_ASSERT_FALSE(hash_equal(&root2, &MPT_EMPTY_ROOT));
  TEST_ASSERT_FALSE(hash_equal(&root2, &root1));

  // Modify balance
  acc.balance = uint256_from_u64(2000);
  world_state_set_account(ws, &addr, &acc);

  // Root should change again
  hash_t root3 = world_state_root(ws);
  TEST_ASSERT_FALSE(hash_equal(&root3, &root2));

  world_state_destroy(ws);
}

// ===========================================================================
// State access interface tests
// ===========================================================================

void test_world_state_access_interface(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x70);

  // Test through interface - initially no account
  TEST_ASSERT_FALSE(access->vtable->account_exists(access, &addr));

  // Set a balance to create a non-empty account
  // (EIP-161: empty accounts are not stored in trie)
  access->vtable->set_balance(access, &addr, uint256_from_u64(100));
  TEST_ASSERT_TRUE(access->vtable->account_exists(access, &addr));

  // Verify balance
  uint256_t bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, uint256_from_u64(100)));

  // Delete account
  access->vtable->delete_account(access, &addr);
  TEST_ASSERT_FALSE(access->vtable->account_exists(access, &addr));

  // Test create_contract - creates with nonce=1 (non-empty)
  address_t contract_addr = make_test_address(0x71);
  access->vtable->create_contract(access, &contract_addr);
  TEST_ASSERT_TRUE(access->vtable->account_exists(access, &contract_addr));
  TEST_ASSERT_EQUAL_UINT64(1, access->vtable->get_nonce(access, &contract_addr));

  // Test state_root through interface
  hash_t root = access->vtable->state_root(access);
  TEST_ASSERT_FALSE(hash_equal(&root, &MPT_EMPTY_ROOT)); // Not empty, has contract

  world_state_destroy(ws);
}

// ===========================================================================
// Transaction boundary tests
// ===========================================================================

void test_world_state_begin_transaction(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x80);
  uint256_t slot = uint256_from_u64(42);

  // Warm up an address and slot
  access->vtable->warm_address(access, &addr);
  access->vtable->warm_slot(access, &addr, slot);

  // Set some storage (to populate original_storage tracking)
  access->vtable->set_storage(access, &addr, slot, uint256_from_u64(100));

  // Verify they're warm
  TEST_ASSERT_TRUE(access->vtable->is_address_warm(access, &addr));
  TEST_ASSERT_TRUE(access->vtable->is_slot_warm(access, &addr, slot));

  // Begin new transaction - should clear warm sets
  access->vtable->begin_transaction(access);

  // Address and slot should be cold again
  TEST_ASSERT_FALSE(access->vtable->is_address_warm(access, &addr));
  TEST_ASSERT_FALSE(access->vtable->is_slot_warm(access, &addr, slot));

  world_state_destroy(ws);
}

// ===========================================================================
// Original storage tests (EIP-2200)
// ===========================================================================

void test_world_state_get_original_storage(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x81);
  uint256_t slot = uint256_from_u64(1);

  // Set initial storage value
  access->vtable->set_storage(access, &addr, slot, uint256_from_u64(100));

  // Begin a new transaction to establish "original" values
  access->vtable->begin_transaction(access);

  // Original storage should be 100 (value at start of tx)
  uint256_t original = access->vtable->get_original_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(original, uint256_from_u64(100)));

  // Modify storage during transaction
  access->vtable->set_storage(access, &addr, slot, uint256_from_u64(200));

  // Current value should be 200
  uint256_t current = access->vtable->get_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(current, uint256_from_u64(200)));

  // But original storage should still be 100
  original = access->vtable->get_original_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(original, uint256_from_u64(100)));

  // Modify again
  access->vtable->set_storage(access, &addr, slot, uint256_from_u64(300));

  // Original should still be 100 (first value at tx start)
  original = access->vtable->get_original_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(original, uint256_from_u64(100)));

  world_state_destroy(ws);
}

void test_world_state_original_storage_unset_slot(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0x82);
  uint256_t slot = uint256_from_u64(99);

  // Slot has never been set, original should be zero
  uint256_t original = access->vtable->get_original_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_is_zero(original));

  // Set it for the first time
  access->vtable->set_storage(access, &addr, slot, uint256_from_u64(50));

  // Original should still be zero (value before first write this tx)
  original = access->vtable->get_original_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_is_zero(original));

  world_state_destroy(ws);
}

// ===========================================================================
// Multi-account isolation tests
// ===========================================================================

void test_world_state_multi_account_storage_isolation(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);

  address_t addr1 = make_test_address(0x90);
  address_t addr2 = make_test_address(0x91);
  uint256_t slot = uint256_from_u64(1); // Same slot number

  // Set different values for same slot in different accounts
  access->vtable->set_storage(access, &addr1, slot, uint256_from_u64(111));
  access->vtable->set_storage(access, &addr2, slot, uint256_from_u64(222));

  // Verify isolation
  uint256_t val1 = access->vtable->get_storage(access, &addr1, slot);
  uint256_t val2 = access->vtable->get_storage(access, &addr2, slot);

  TEST_ASSERT_TRUE(uint256_eq(val1, uint256_from_u64(111)));
  TEST_ASSERT_TRUE(uint256_eq(val2, uint256_from_u64(222)));

  // Modify one, verify other unchanged
  access->vtable->set_storage(access, &addr1, slot, uint256_from_u64(999));

  val1 = access->vtable->get_storage(access, &addr1, slot);
  val2 = access->vtable->get_storage(access, &addr2, slot);

  TEST_ASSERT_TRUE(uint256_eq(val1, uint256_from_u64(999)));
  TEST_ASSERT_TRUE(uint256_eq(val2, uint256_from_u64(222))); // Unchanged

  world_state_destroy(ws);
}

void test_world_state_multi_account_balance_isolation(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);

  address_t addr1 = make_test_address(0x92);
  address_t addr2 = make_test_address(0x93);

  // Set different balances
  access->vtable->set_balance(access, &addr1, uint256_from_u64(1000));
  access->vtable->set_balance(access, &addr2, uint256_from_u64(2000));

  // Verify isolation
  uint256_t bal1 = access->vtable->get_balance(access, &addr1);
  uint256_t bal2 = access->vtable->get_balance(access, &addr2);

  TEST_ASSERT_TRUE(uint256_eq(bal1, uint256_from_u64(1000)));
  TEST_ASSERT_TRUE(uint256_eq(bal2, uint256_from_u64(2000)));

  // Transfer between accounts
  access->vtable->sub_balance(access, &addr1, uint256_from_u64(500));
  access->vtable->add_balance(access, &addr2, uint256_from_u64(500));

  bal1 = access->vtable->get_balance(access, &addr1);
  bal2 = access->vtable->get_balance(access, &addr2);

  TEST_ASSERT_TRUE(uint256_eq(bal1, uint256_from_u64(500)));
  TEST_ASSERT_TRUE(uint256_eq(bal2, uint256_from_u64(2500)));

  world_state_destroy(ws);
}

// ===========================================================================
// Large value tests
// ===========================================================================

void test_world_state_large_balance(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xA0);

  // Create a large balance (all limbs set)
  uint256_t large_balance = uint256_from_limbs(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
                                               0xFFFFFFFFFFFFFFFFULL, 0x0FFFFFFFFFFFFFFFULL);

  access->vtable->set_balance(access, &addr, large_balance);

  uint256_t retrieved = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(retrieved, large_balance));

  world_state_destroy(ws);
}

void test_world_state_large_storage_key(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xA1);

  // Use a large 256-bit slot key
  uint256_t large_slot = uint256_from_limbs(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL,
                                            0xFEDCBA0987654321ULL, 0x0ULL);

  uint256_t value = uint256_from_u64(42);

  access->vtable->set_storage(access, &addr, large_slot, value);

  uint256_t retrieved = access->vtable->get_storage(access, &addr, large_slot);
  TEST_ASSERT_TRUE(uint256_eq(retrieved, value));

  // Different large slot should be zero
  uint256_t other_slot = uint256_from_limbs(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL,
                                            0xFEDCBA0987654321ULL, 0x1ULL);
  uint256_t other_val = access->vtable->get_storage(access, &addr, other_slot);
  TEST_ASSERT_TRUE(uint256_is_zero(other_val));

  world_state_destroy(ws);
}

void test_world_state_large_storage_value(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xA2);
  uint256_t slot = uint256_from_u64(1);

  // Store a large 256-bit value
  uint256_t large_value = uint256_from_limbs(0xAAAAAAAAAAAAAAAAULL, 0xBBBBBBBBBBBBBBBBULL,
                                             0xCCCCCCCCCCCCCCCCULL, 0xDDDDDDDDDDDDDDDDULL);

  access->vtable->set_storage(access, &addr, slot, large_value);

  uint256_t retrieved = access->vtable->get_storage(access, &addr, slot);
  TEST_ASSERT_TRUE(uint256_eq(retrieved, large_value));

  world_state_destroy(ws);
}

// ===========================================================================
// Edge case tests
// ===========================================================================

void test_world_state_add_balance_overflow(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xB0);

  // Set balance to max value
  uint256_t max_balance = uint256_from_limbs(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
                                             0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
  access->vtable->set_balance(access, &addr, max_balance);

  // Try to add 1 - should fail (overflow)
  bool result = access->vtable->add_balance(access, &addr, uint256_from_u64(1));
  TEST_ASSERT_FALSE(result);

  // Balance should be unchanged
  uint256_t bal = access->vtable->get_balance(access, &addr);
  TEST_ASSERT_TRUE(uint256_eq(bal, max_balance));

  world_state_destroy(ws);
}

void test_world_state_clear(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xB1);

  // Set up some state
  access->vtable->set_balance(access, &addr, uint256_from_u64(1000));
  access->vtable->set_storage(access, &addr, uint256_from_u64(1), uint256_from_u64(42));
  access->vtable->warm_address(access, &addr);

  // Verify state exists
  TEST_ASSERT_TRUE(access->vtable->account_exists(access, &addr));

  // Clear all state
  world_state_clear(ws);

  // Account should no longer exist
  TEST_ASSERT_FALSE(access->vtable->account_exists(access, &addr));

  // State root should be empty again
  hash_t root = world_state_root(ws);
  TEST_ASSERT_TRUE(hash_equal(&root, &MPT_EMPTY_ROOT));

  // Address should be cold
  TEST_ASSERT_FALSE(access->vtable->is_address_warm(access, &addr));

  world_state_destroy(ws);
}

void test_world_state_account_is_empty_interface(void) {
  world_state_t *ws = world_state_create(&test_arena);
  state_access_t *access = world_state_access(ws);
  address_t addr = make_test_address(0xB2);

  // Non-existent account is empty
  TEST_ASSERT_TRUE(access->vtable->account_is_empty(access, &addr));

  // Account with only balance is not empty
  access->vtable->set_balance(access, &addr, uint256_from_u64(1));
  TEST_ASSERT_FALSE(access->vtable->account_is_empty(access, &addr));

  // Set balance to zero - account becomes empty (and deleted per EIP-161)
  access->vtable->set_balance(access, &addr, uint256_zero());
  TEST_ASSERT_TRUE(access->vtable->account_is_empty(access, &addr));

  // Contract account (nonce=1) is not empty
  access->vtable->create_contract(access, &addr);
  TEST_ASSERT_FALSE(access->vtable->account_is_empty(access, &addr));

  world_state_destroy(ws);
}
