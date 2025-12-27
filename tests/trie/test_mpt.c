#include "test_mpt.h"

#include "div0/trie/mpt.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

// Helper to create a fresh MPT for each test
static mpt_t create_test_mpt(void) {
  mpt_backend_t *backend = mpt_memory_backend_create(&test_arena);
  mpt_t mpt;
  mpt_init(&mpt, backend, &test_arena);
  return mpt;
}

// ===========================================================================
// MPT initialization and lifecycle tests
// ===========================================================================

void test_mpt_init(void) {
  mpt_backend_t *backend = mpt_memory_backend_create(&test_arena);
  TEST_ASSERT_NOT_NULL(backend);

  mpt_t mpt;
  mpt_init(&mpt, backend, &test_arena);

  TEST_ASSERT_EQUAL_PTR(backend, mpt.backend);
  TEST_ASSERT_EQUAL_PTR(&test_arena, mpt.work_arena);

  mpt_destroy(&mpt);
}

void test_mpt_empty_is_empty(void) {
  mpt_t mpt = create_test_mpt();

  TEST_ASSERT_TRUE(mpt_is_empty(&mpt));

  mpt_destroy(&mpt);
}

void test_mpt_empty_root_hash(void) {
  mpt_t mpt = create_test_mpt();

  hash_t root = mpt_root_hash(&mpt);

  // Empty trie root hash is keccak256(0x80) = MPT_EMPTY_ROOT
  TEST_ASSERT_TRUE(hash_equal(&root, &MPT_EMPTY_ROOT));

  mpt_destroy(&mpt);
}

// ===========================================================================
// MPT insert tests
// ===========================================================================

void test_mpt_insert_single(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02, 0x03};
  uint8_t value[] = {0xAB, 0xCD, 0xEF};

  bool result = mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_FALSE(mpt_is_empty(&mpt));

  mpt_destroy(&mpt);
}

void test_mpt_insert_overwrite(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02};
  uint8_t value1[] = {0xAA};
  uint8_t value2[] = {0xBB, 0xCC};

  mpt_insert(&mpt, key, sizeof(key), value1, sizeof(value1));
  hash_t hash1 = mpt_root_hash(&mpt);

  mpt_insert(&mpt, key, sizeof(key), value2, sizeof(value2));
  hash_t hash2 = mpt_root_hash(&mpt);

  // Root hash should change after overwrite
  TEST_ASSERT_FALSE(hash_equal(&hash1, &hash2));

  mpt_destroy(&mpt);
}

void test_mpt_insert_two_different_keys(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key1[] = {0x01};
  uint8_t key2[] = {0x02};
  uint8_t value[] = {0xFF};

  mpt_insert(&mpt, key1, sizeof(key1), value, sizeof(value));
  hash_t hash1 = mpt_root_hash(&mpt);

  mpt_insert(&mpt, key2, sizeof(key2), value, sizeof(value));
  hash_t hash2 = mpt_root_hash(&mpt);

  // Adding second key should change root
  TEST_ASSERT_FALSE(hash_equal(&hash1, &hash2));

  mpt_destroy(&mpt);
}

void test_mpt_insert_common_prefix(void) {
  mpt_t mpt = create_test_mpt();

  // Keys with common prefix: 0xAB, 0xAC
  uint8_t key1[] = {0xAB};
  uint8_t key2[] = {0xAC};
  uint8_t value[] = {0x01};

  mpt_insert(&mpt, key1, sizeof(key1), value, sizeof(value));
  mpt_insert(&mpt, key2, sizeof(key2), value, sizeof(value));

  // Should have created a branch node
  TEST_ASSERT_FALSE(mpt_is_empty(&mpt));

  mpt_destroy(&mpt);
}

void test_mpt_insert_branch_creation(void) {
  mpt_t mpt = create_test_mpt();

  // Insert keys that differ in first nibble
  uint8_t key1[] = {0x10};
  uint8_t key2[] = {0x20};
  uint8_t value[] = {0xFF};

  mpt_insert(&mpt, key1, sizeof(key1), value, sizeof(value));
  mpt_insert(&mpt, key2, sizeof(key2), value, sizeof(value));

  // Both keys should be accessible
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key1, sizeof(key1)));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key2, sizeof(key2)));

  mpt_destroy(&mpt);
}

// ===========================================================================
// MPT get tests
// ===========================================================================

void test_mpt_get_not_found(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02};
  bytes_t result = mpt_get(&mpt, key, sizeof(key));

  TEST_ASSERT_EQUAL_size_t(0, result.size);

  mpt_destroy(&mpt);
}

void test_mpt_get_after_insert(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0xDE, 0xAD};
  uint8_t value[] = {0xBE, 0xEF};

  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  bytes_t result = mpt_get(&mpt, key, sizeof(key));

  TEST_ASSERT_EQUAL_size_t(sizeof(value), result.size);
  TEST_ASSERT_EQUAL_MEMORY(value, result.data, sizeof(value));

  mpt_destroy(&mpt);
}

void test_mpt_contains_works(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x11, 0x22, 0x33};
  uint8_t value[] = {0xAA};

  TEST_ASSERT_FALSE(mpt_contains(&mpt, key, sizeof(key)));

  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));

  TEST_ASSERT_TRUE(mpt_contains(&mpt, key, sizeof(key)));

  mpt_destroy(&mpt);
}

// ===========================================================================
// MPT root hash tests
// ===========================================================================

void test_mpt_root_hash_single_entry(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01};
  uint8_t value[] = {0x02};

  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  hash_t root = mpt_root_hash(&mpt);

  // Should not be empty root
  TEST_ASSERT_FALSE(hash_equal(&root, &MPT_EMPTY_ROOT));
  // Should not be zero
  TEST_ASSERT_FALSE(hash_is_zero(&root));

  mpt_destroy(&mpt);
}

void test_mpt_root_hash_deterministic(void) {
  // Create first trie
  mpt_backend_t *backend1 = mpt_memory_backend_create(&test_arena);
  mpt_t mpt1;
  mpt_init(&mpt1, backend1, &test_arena);

  uint8_t key[] = {0xCA, 0xFE};
  uint8_t value[] = {0xBA, 0xBE};
  mpt_insert(&mpt1, key, sizeof(key), value, sizeof(value));
  hash_t hash1 = mpt_root_hash(&mpt1);

  // Create second trie with same data
  mpt_backend_t *backend2 = mpt_memory_backend_create(&test_arena);
  mpt_t mpt2;
  mpt_init(&mpt2, backend2, &test_arena);
  mpt_insert(&mpt2, key, sizeof(key), value, sizeof(value));
  hash_t hash2 = mpt_root_hash(&mpt2);

  // Root hashes should be identical
  TEST_ASSERT_TRUE(hash_equal(&hash1, &hash2));

  mpt_destroy(&mpt1);
  mpt_destroy(&mpt2);
}

void test_mpt_root_hash_order_independent(void) {
  // Create first trie, insert A then B
  mpt_backend_t *backend1 = mpt_memory_backend_create(&test_arena);
  mpt_t mpt1;
  mpt_init(&mpt1, backend1, &test_arena);

  uint8_t keyA[] = {0x0A};
  uint8_t keyB[] = {0x0B};
  uint8_t valueA[] = {0xAA};
  uint8_t valueB[] = {0xBB};

  mpt_insert(&mpt1, keyA, sizeof(keyA), valueA, sizeof(valueA));
  mpt_insert(&mpt1, keyB, sizeof(keyB), valueB, sizeof(valueB));
  hash_t hash1 = mpt_root_hash(&mpt1);

  // Create second trie, insert B then A
  mpt_backend_t *backend2 = mpt_memory_backend_create(&test_arena);
  mpt_t mpt2;
  mpt_init(&mpt2, backend2, &test_arena);

  mpt_insert(&mpt2, keyB, sizeof(keyB), valueB, sizeof(valueB));
  mpt_insert(&mpt2, keyA, sizeof(keyA), valueA, sizeof(valueA));
  hash_t hash2 = mpt_root_hash(&mpt2);

  // Order should not matter - same root hash
  TEST_ASSERT_TRUE(hash_equal(&hash1, &hash2));

  mpt_destroy(&mpt1);
  mpt_destroy(&mpt2);
}

// ===========================================================================
// Memory backend tests
// ===========================================================================

void test_mpt_memory_backend_create(void) {
  mpt_backend_t *backend = mpt_memory_backend_create(&test_arena);

  TEST_ASSERT_NOT_NULL(backend);
  TEST_ASSERT_NOT_NULL(backend->vtable);
  TEST_ASSERT_NOT_NULL(backend->vtable->get_root);
  TEST_ASSERT_NOT_NULL(backend->vtable->set_root);
  TEST_ASSERT_NOT_NULL(backend->vtable->alloc_node);

  // Initially no root
  TEST_ASSERT_NULL(backend->vtable->get_root(backend));
}

void test_mpt_memory_backend_alloc_node(void) {
  mpt_backend_t *backend = mpt_memory_backend_create(&test_arena);

  mpt_node_t *node = backend->vtable->alloc_node(backend);

  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL(MPT_NODE_EMPTY, node->type);
  TEST_ASSERT_TRUE(node->hash_valid);
}

// ===========================================================================
// Ethereum test vectors (from trieanyorder.json)
// https://github.com/ethereum/tests/tree/develop/TrieTests
// ===========================================================================

void test_mpt_ethereum_vector_dogs(void) {
  // dogs test: doe->reindeer, dog->puppy, dogglesworth->cat
  // Root: 0x8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"doe", 3, (const uint8_t *)"reindeer", 8);
  mpt_insert(&mpt, (const uint8_t *)"dog", 3, (const uint8_t *)"puppy", 5);
  mpt_insert(&mpt, (const uint8_t *)"dogglesworth", 12, (const uint8_t *)"cat", 3);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_puppy(void) {
  // puppy test: do->verb, horse->stallion, doge->coin, dog->puppy
  // Root: 0x5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"do", 2, (const uint8_t *)"verb", 4);
  mpt_insert(&mpt, (const uint8_t *)"horse", 5, (const uint8_t *)"stallion", 8);
  mpt_insert(&mpt, (const uint8_t *)"doge", 4, (const uint8_t *)"coin", 4);
  mpt_insert(&mpt, (const uint8_t *)"dog", 3, (const uint8_t *)"puppy", 5);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_foo(void) {
  // foo test: foo->bar, food->bass
  // Root: 0x17beaa1648bafa633cda809c90c04af50fc8aed3cb40d16efbddee6fdf63c4c3
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"foo", 3, (const uint8_t *)"bar", 3);
  mpt_insert(&mpt, (const uint8_t *)"food", 4, (const uint8_t *)"bass", 4);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("17beaa1648bafa633cda809c90c04af50fc8aed3cb40d16efbddee6fdf63c4c3", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_small_values(void) {
  // smallValues test: be->e, dog->puppy, bed->d
  // Root: 0x3f67c7a47520f79faa29255d2d3c084a7a6df0453116ed7232ff10277a8be68b
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"be", 2, (const uint8_t *)"e", 1);
  mpt_insert(&mpt, (const uint8_t *)"dog", 3, (const uint8_t *)"puppy", 5);
  mpt_insert(&mpt, (const uint8_t *)"bed", 3, (const uint8_t *)"d", 1);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("3f67c7a47520f79faa29255d2d3c084a7a6df0453116ed7232ff10277a8be68b", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_testy(void) {
  // testy test: test->test, te->testy
  // Root: 0x8452568af70d8d140f58d941338542f645fcca50094b20f3c3d8c3df49337928
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"test", 4, (const uint8_t *)"test", 4);
  mpt_insert(&mpt, (const uint8_t *)"te", 2, (const uint8_t *)"testy", 5);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("8452568af70d8d140f58d941338542f645fcca50094b20f3c3d8c3df49337928", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_hex(void) {
  // hex test: 0x0045->0x0123456789, 0x4500->0x9876543210
  // Root: 0x285505fcabe84badc8aa310e2aae17eddc7d120aabec8a476902c8184b3a3503
  mpt_t mpt = create_test_mpt();

  uint8_t key1[] = {0x00, 0x45};
  uint8_t value1[] = {0x01, 0x23, 0x45, 0x67, 0x89};
  uint8_t key2[] = {0x45, 0x00};
  uint8_t value2[] = {0x98, 0x76, 0x54, 0x32, 0x10};

  mpt_insert(&mpt, key1, sizeof(key1), value1, sizeof(value1));
  mpt_insert(&mpt, key2, sizeof(key2), value2, sizeof(value2));

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("285505fcabe84badc8aa310e2aae17eddc7d120aabec8a476902c8184b3a3503", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

// ===========================================================================
// Ethereum test vectors (from trietest.json)
// ===========================================================================

void test_mpt_ethereum_vector_insert_middle_leaf(void) {
  // insert-middle-leaf test: Tests inserting keys that share prefixes
  // Root: 0xcb65032e2f76c48b82b5c24b3db8f670ce73982869d38cd39a624f23d62a9e89
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"key1aa", 6,
             (const uint8_t *)"0123456789012345678901234567890123456789xxx", 43);
  mpt_insert(&mpt, (const uint8_t *)"key1", 4,
             (const uint8_t *)"0123456789012345678901234567890123456789Very_Long", 49);
  mpt_insert(&mpt, (const uint8_t *)"key2bb", 6, (const uint8_t *)"aval3", 5);
  mpt_insert(&mpt, (const uint8_t *)"key2", 4, (const uint8_t *)"short", 5);
  mpt_insert(&mpt, (const uint8_t *)"key3cc", 6, (const uint8_t *)"aval3", 5);
  mpt_insert(&mpt, (const uint8_t *)"key3", 4, (const uint8_t *)"1234567890123456789012345678901",
             31);

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("cb65032e2f76c48b82b5c24b3db8f670ce73982869d38cd39a624f23d62a9e89", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

void test_mpt_ethereum_vector_branch_value_update(void) {
  // branch-value-update test: Tests updating a value at a branch point
  // abc->123, abcd->abcd, abc->abc (update)
  // Root: 0x7a320748f780ad9ad5b0837302075ce0eeba6c26e3d8562c67ccc0f1b273298a
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"abc", 3, (const uint8_t *)"123", 3);
  mpt_insert(&mpt, (const uint8_t *)"abcd", 4, (const uint8_t *)"abcd", 4);
  mpt_insert(&mpt, (const uint8_t *)"abc", 3, (const uint8_t *)"abc", 3); // Update

  hash_t root = mpt_root_hash(&mpt);
  hash_t expected;
  hash_from_hex("7a320748f780ad9ad5b0837302075ce0eeba6c26e3d8562c67ccc0f1b273298a", &expected);

  TEST_ASSERT_TRUE(hash_equal(&root, &expected));

  mpt_destroy(&mpt);
}

// ===========================================================================
// Edge case tests
// ===========================================================================

void test_mpt_empty_value(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02, 0x03};
  // Insert with empty value
  mpt_insert(&mpt, key, sizeof(key), NULL, 0);

  // Should still be stored (key exists)
  TEST_ASSERT_FALSE(mpt_is_empty(&mpt));

  mpt_destroy(&mpt);
}

void test_mpt_long_key(void) {
  // Test with a 32-byte key (typical for Ethereum storage keys)
  mpt_t mpt = create_test_mpt();

  uint8_t key[32];
  memset(key, 0, 32);
  key[31] = 0x01; // 0x00...01

  uint8_t value[] = {'s', 't', 'o', 'r', 'a', 'g', 'e'};

  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  bytes_t result = mpt_get(&mpt, key, sizeof(key));

  TEST_ASSERT_EQUAL_size_t(sizeof(value), result.size);
  TEST_ASSERT_EQUAL_MEMORY(value, result.data, sizeof(value));

  mpt_destroy(&mpt);
}

void test_mpt_binary_keys(void) {
  // Test with binary (non-ASCII) keys
  mpt_t mpt = create_test_mpt();

  uint8_t key00[] = {0x00};
  uint8_t keyff[] = {0xff};
  uint8_t key0f[] = {0x0f};
  uint8_t keyf0[] = {0xf0};

  mpt_insert(&mpt, key00, 1, (const uint8_t *)"zero", 4);
  mpt_insert(&mpt, keyff, 1, (const uint8_t *)"max", 3);
  mpt_insert(&mpt, key0f, 1, (const uint8_t *)"fifteen", 7);
  mpt_insert(&mpt, keyf0, 1, (const uint8_t *)"two-forty", 9);

  // All keys should be accessible
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key00, 1));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, keyff, 1));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key0f, 1));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, keyf0, 1));

  mpt_destroy(&mpt);
}

void test_mpt_shared_prefix_keys(void) {
  // Keys with shared prefixes: a, ab, abc, abcd, abcde
  mpt_t mpt = create_test_mpt();

  mpt_insert(&mpt, (const uint8_t *)"a", 1, (const uint8_t *)"1", 1);
  mpt_insert(&mpt, (const uint8_t *)"ab", 2, (const uint8_t *)"2", 1);
  mpt_insert(&mpt, (const uint8_t *)"abc", 3, (const uint8_t *)"3", 1);
  mpt_insert(&mpt, (const uint8_t *)"abcd", 4, (const uint8_t *)"4", 1);
  mpt_insert(&mpt, (const uint8_t *)"abcde", 5, (const uint8_t *)"5", 1);

  // All keys should be accessible
  TEST_ASSERT_TRUE(mpt_contains(&mpt, (const uint8_t *)"a", 1));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, (const uint8_t *)"ab", 2));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, (const uint8_t *)"abc", 3));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, (const uint8_t *)"abcd", 4));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, (const uint8_t *)"abcde", 5));

  // Verify values
  bytes_t r1 = mpt_get(&mpt, (const uint8_t *)"a", 1);
  bytes_t r5 = mpt_get(&mpt, (const uint8_t *)"abcde", 5);
  TEST_ASSERT_EQUAL_size_t(1, r1.size);
  TEST_ASSERT_EQUAL_UINT8('1', r1.data[0]);
  TEST_ASSERT_EQUAL_size_t(1, r5.size);
  TEST_ASSERT_EQUAL_UINT8('5', r5.data[0]);

  mpt_destroy(&mpt);
}

void test_mpt_many_keys(void) {
  // Test with many keys to exercise branch nodes
  mpt_t mpt = create_test_mpt();

  // Insert 50 keys
  for (int i = 0; i < 50; i++) {
    uint8_t key[4];
    key[0] = 'k';
    key[1] = 'e';
    key[2] = 'y';
    key[3] = (uint8_t)i;

    uint8_t value[2];
    value[0] = 'v';
    value[1] = (uint8_t)i;

    mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  }

  // Verify all keys are accessible
  for (int i = 0; i < 50; i++) {
    uint8_t key[4];
    key[0] = 'k';
    key[1] = 'e';
    key[2] = 'y';
    key[3] = (uint8_t)i;

    TEST_ASSERT_TRUE(mpt_contains(&mpt, key, sizeof(key)));
  }

  mpt_destroy(&mpt);
}

void test_mpt_contains_empty_value(void) {
  // Test that mpt_contains returns true for keys with empty values
  mpt_t mpt = create_test_mpt();

  uint8_t key1[] = {0x01, 0x02, 0x03};
  uint8_t key2[] = {0x04, 0x05};

  // Insert key1 with empty value
  mpt_insert(&mpt, key1, sizeof(key1), NULL, 0);

  // Insert key2 with non-empty value
  uint8_t value2[] = {0xAB};
  mpt_insert(&mpt, key2, sizeof(key2), value2, sizeof(value2));

  // Both keys should exist
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key1, sizeof(key1)));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key2, sizeof(key2)));

  // Non-existent key should not exist
  uint8_t key3[] = {0xFF};
  TEST_ASSERT_FALSE(mpt_contains(&mpt, key3, sizeof(key3)));

  mpt_destroy(&mpt);
}

// ===========================================================================
// Delete tests
// ===========================================================================

void test_mpt_delete_single(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02};
  uint8_t value[] = {0xAB, 0xCD};

  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key, sizeof(key)));

  bool deleted = mpt_delete(&mpt, key, sizeof(key));
  TEST_ASSERT_TRUE(deleted);
  TEST_ASSERT_FALSE(mpt_contains(&mpt, key, sizeof(key)));
  TEST_ASSERT_TRUE(mpt_is_empty(&mpt));

  mpt_destroy(&mpt);
}

void test_mpt_delete_not_found(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02};

  // Delete from empty trie
  bool deleted = mpt_delete(&mpt, key, sizeof(key));
  TEST_ASSERT_FALSE(deleted);

  // Insert a different key
  uint8_t other_key[] = {0x03, 0x04};
  uint8_t value[] = {0xFF};
  mpt_insert(&mpt, other_key, sizeof(other_key), value, sizeof(value));

  // Try to delete non-existent key
  deleted = mpt_delete(&mpt, key, sizeof(key));
  TEST_ASSERT_FALSE(deleted);

  // Other key should still exist
  TEST_ASSERT_TRUE(mpt_contains(&mpt, other_key, sizeof(other_key)));

  mpt_destroy(&mpt);
}

void test_mpt_delete_from_branch(void) {
  mpt_t mpt = create_test_mpt();

  // Insert multiple keys that create a branch
  uint8_t key1[] = {0x10};
  uint8_t key2[] = {0x20};
  uint8_t key3[] = {0x30};
  uint8_t value[] = {0xFF};

  mpt_insert(&mpt, key1, sizeof(key1), value, sizeof(value));
  mpt_insert(&mpt, key2, sizeof(key2), value, sizeof(value));
  mpt_insert(&mpt, key3, sizeof(key3), value, sizeof(value));

  // Delete one key
  bool deleted = mpt_delete(&mpt, key2, sizeof(key2));
  TEST_ASSERT_TRUE(deleted);

  // Other keys should still exist
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key1, sizeof(key1)));
  TEST_ASSERT_FALSE(mpt_contains(&mpt, key2, sizeof(key2)));
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key3, sizeof(key3)));

  mpt_destroy(&mpt);
}

void test_mpt_delete_collapses_branch(void) {
  mpt_t mpt = create_test_mpt();

  // Insert two keys that create a branch, then delete one
  // The branch should collapse to an extension or leaf
  uint8_t key1[] = {0x10};
  uint8_t key2[] = {0x20};
  uint8_t value[] = {0xFF};

  mpt_insert(&mpt, key1, sizeof(key1), value, sizeof(value));
  mpt_insert(&mpt, key2, sizeof(key2), value, sizeof(value));

  hash_t hash_before = mpt_root_hash(&mpt);

  // Delete one key - branch should collapse
  bool deleted = mpt_delete(&mpt, key2, sizeof(key2));
  TEST_ASSERT_TRUE(deleted);

  // Root hash should have changed
  hash_t hash_after = mpt_root_hash(&mpt);
  TEST_ASSERT_FALSE(hash_equal(&hash_before, &hash_after));

  // Remaining key should still be accessible
  TEST_ASSERT_TRUE(mpt_contains(&mpt, key1, sizeof(key1)));

  // The trie with one key should have same hash as fresh single insert
  mpt_t mpt2 = create_test_mpt();
  mpt_insert(&mpt2, key1, sizeof(key1), value, sizeof(value));
  hash_t hash_single = mpt_root_hash(&mpt2);

  TEST_ASSERT_TRUE(hash_equal(&hash_after, &hash_single));

  mpt_destroy(&mpt);
  mpt_destroy(&mpt2);
}

// Test that deleting and re-inserting same key-value produces same hash
void test_mpt_delete_and_reinsert(void) {
  mpt_t mpt = create_test_mpt();

  uint8_t key[] = {0x01, 0x02};
  uint8_t value[] = {0xAA, 0xBB};

  // Insert and get initial hash
  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  hash_t hash_original = mpt_root_hash(&mpt);

  // Delete the key
  bool deleted = mpt_delete(&mpt, key, sizeof(key));
  TEST_ASSERT_TRUE(deleted);
  TEST_ASSERT_TRUE(mpt_is_empty(&mpt));

  // Re-insert the same key-value
  mpt_insert(&mpt, key, sizeof(key), value, sizeof(value));
  hash_t hash_after = mpt_root_hash(&mpt);

  // Hashes should match
  TEST_ASSERT_TRUE(hash_equal(&hash_original, &hash_after));

  // Value should be retrievable
  bytes_t retrieved = mpt_get(&mpt, key, sizeof(key));
  TEST_ASSERT_EQUAL_size_t(sizeof(value), retrieved.size);
  TEST_ASSERT_EQUAL_MEMORY(value, retrieved.data, sizeof(value));

  mpt_destroy(&mpt);
}
