#ifndef TEST_WORLD_STATE_H
#define TEST_WORLD_STATE_H

// World state creation tests
void test_world_state_create(void);
void test_world_state_empty_root(void);

// Account operations tests
void test_world_state_get_nonexistent_account(void);
void test_world_state_set_and_get_account(void);
void test_world_state_delete_empty_account(void);

// Balance operations tests
void test_world_state_balance_operations(void);
void test_world_state_add_balance(void);
void test_world_state_sub_balance(void);

// Nonce operations tests
void test_world_state_nonce_operations(void);
void test_world_state_increment_nonce(void);

// Code operations tests
void test_world_state_code_operations(void);
void test_world_state_code_hash(void);

// Storage operations tests
void test_world_state_storage_operations(void);
void test_world_state_storage_multiple_slots(void);

// EIP-2929 warm/cold tracking tests
void test_world_state_warm_address(void);
void test_world_state_warm_slot(void);

// State root tests
void test_world_state_root_changes(void);

// State access interface tests
void test_world_state_access_interface(void);

#endif // TEST_WORLD_STATE_H
