#ifndef TEST_MPT_H
#define TEST_MPT_H

// MPT initialization and lifecycle tests
void test_mpt_init(void);
void test_mpt_empty_is_empty(void);
void test_mpt_empty_root_hash(void);

// MPT insert tests
void test_mpt_insert_single(void);
void test_mpt_insert_overwrite(void);
void test_mpt_insert_two_different_keys(void);
void test_mpt_insert_common_prefix(void);
void test_mpt_insert_branch_creation(void);

// MPT get tests
void test_mpt_get_not_found(void);
void test_mpt_get_after_insert(void);
void test_mpt_contains_works(void);

// MPT root hash tests
void test_mpt_root_hash_single_entry(void);
void test_mpt_root_hash_deterministic(void);
void test_mpt_root_hash_order_independent(void);

// Memory backend tests
void test_mpt_memory_backend_create(void);
void test_mpt_memory_backend_alloc_node(void);

// Ethereum test vectors (from trieanyorder.json)
void test_mpt_ethereum_vector_dogs(void);
void test_mpt_ethereum_vector_puppy(void);
void test_mpt_ethereum_vector_foo(void);
void test_mpt_ethereum_vector_small_values(void);
void test_mpt_ethereum_vector_testy(void);
void test_mpt_ethereum_vector_hex(void);

// Ethereum test vectors (from trietest.json)
void test_mpt_ethereum_vector_insert_middle_leaf(void);
void test_mpt_ethereum_vector_branch_value_update(void);

// Edge case tests
void test_mpt_empty_value(void);
void test_mpt_contains_empty_value(void);
void test_mpt_long_key(void);
void test_mpt_binary_keys(void);
void test_mpt_shared_prefix_keys(void);
void test_mpt_many_keys(void);

// Delete tests
void test_mpt_delete_single(void);
void test_mpt_delete_not_found(void);
void test_mpt_delete_from_branch(void);
void test_mpt_delete_collapses_branch(void);
void test_mpt_delete_and_reinsert(void);

#endif // TEST_MPT_H
