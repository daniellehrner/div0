#include "test_node.h"

#include "div0/crypto/keccak256.h"
#include "div0/trie/node.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

// ===========================================================================
// Node creation tests
// ===========================================================================

void test_mpt_node_empty(void) {
  mpt_node_t node = mpt_node_empty();

  TEST_ASSERT_EQUAL(MPT_NODE_EMPTY, node.type);
  TEST_ASSERT_TRUE(mpt_node_is_empty(&node));
  TEST_ASSERT_FALSE(mpt_node_is_leaf(&node));
  TEST_ASSERT_FALSE(mpt_node_is_branch(&node));
  TEST_ASSERT_FALSE(mpt_node_is_extension(&node));
  // Empty node hash should be pre-computed
  TEST_ASSERT_TRUE(node.hash_valid);
}

void test_mpt_node_leaf(void) {
  uint8_t path_data[] = {1, 2, 3, 4};
  nibbles_t path = {.data = path_data, .len = 4};

  uint8_t value_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 4);

  mpt_node_t node = mpt_node_leaf(path, value);

  TEST_ASSERT_EQUAL(MPT_NODE_LEAF, node.type);
  TEST_ASSERT_TRUE(mpt_node_is_leaf(&node));
  TEST_ASSERT_EQUAL_size_t(4, node.leaf.path.len);
  TEST_ASSERT_EQUAL_size_t(4, node.leaf.value.size);
  TEST_ASSERT_FALSE(node.hash_valid);
}

void test_mpt_node_extension(void) {
  uint8_t path_data[] = {0xA, 0xB, 0xC};
  nibbles_t path = {.data = path_data, .len = 3};
  node_ref_t child = node_ref_null();

  mpt_node_t node = mpt_node_extension(path, child);

  TEST_ASSERT_EQUAL(MPT_NODE_EXTENSION, node.type);
  TEST_ASSERT_TRUE(mpt_node_is_extension(&node));
  TEST_ASSERT_EQUAL_size_t(3, node.extension.path.len);
  TEST_ASSERT_TRUE(node_ref_is_null(&node.extension.child));
  TEST_ASSERT_FALSE(node.hash_valid);
}

void test_mpt_node_branch(void) {
  mpt_node_t node = mpt_node_branch();

  TEST_ASSERT_EQUAL(MPT_NODE_BRANCH, node.type);
  TEST_ASSERT_TRUE(mpt_node_is_branch(&node));

  // All children should be null
  for (int i = 0; i < 16; i++) {
    TEST_ASSERT_TRUE(node_ref_is_null(&node.branch.children[i]));
  }

  // Value should be empty
  TEST_ASSERT_TRUE(bytes_is_empty(&node.branch.value));
  TEST_ASSERT_FALSE(node.hash_valid);
}

// ===========================================================================
// Node reference tests
// ===========================================================================

void test_node_ref_null(void) {
  node_ref_t ref = node_ref_null();

  TEST_ASSERT_FALSE(ref.is_hash);
  TEST_ASSERT_TRUE(node_ref_is_null(&ref));
}

void test_node_ref_is_null(void) {
  // Test embedded null
  node_ref_t embedded_null = node_ref_null();
  TEST_ASSERT_TRUE(node_ref_is_null(&embedded_null));

  // Test embedded non-null
  node_ref_t embedded_non_null = {.is_hash = false};
  bytes_init_arena(&embedded_non_null.embedded, &test_arena);
  uint8_t data[] = {0xC0}; // Empty RLP list
  bytes_from_data(&embedded_non_null.embedded, data, 1);
  TEST_ASSERT_FALSE(node_ref_is_null(&embedded_non_null));

  // Test hash null (zero hash)
  node_ref_t hash_null = {.is_hash = true, .hash = hash_zero()};
  TEST_ASSERT_TRUE(node_ref_is_null(&hash_null));

  // Test hash non-null
  node_ref_t hash_non_null = {.is_hash = true, .hash = MPT_EMPTY_ROOT};
  TEST_ASSERT_FALSE(node_ref_is_null(&hash_non_null));
}

// ===========================================================================
// Node encoding tests
// ===========================================================================

void test_mpt_node_encode_empty(void) {
  mpt_node_t node = mpt_node_empty();
  bytes_t encoded = mpt_node_encode(&node, &test_arena);

  // Empty node encodes as empty RLP string: 0x80
  TEST_ASSERT_EQUAL_size_t(1, encoded.size);
  TEST_ASSERT_EQUAL_UINT8(0x80, encoded.data[0]);
}

void test_mpt_node_encode_leaf(void) {
  // Create a simple leaf with path [1, 2] and value [0xAB]
  uint8_t path_data[] = {1, 2};
  nibbles_t path = {.data = path_data, .len = 2};

  uint8_t value_data[] = {0xAB};
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 1);

  mpt_node_t node = mpt_node_leaf(path, value);
  bytes_t encoded = mpt_node_encode(&node, &test_arena);

  // Leaf encoding: [hex_prefix([1,2], leaf=true), [0xAB]]
  // hex_prefix([1,2], leaf=true) = [0x20, 0x12] (even, leaf)
  // RLP of [0x20, 0x12]: 0x82 0x20 0x12
  // RLP of [0xAB]: 0x81 0xAB
  // List: 0xC4 ... (total payload = 5)
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
  // First byte should be short list prefix (0xC0-0xF7)
  TEST_ASSERT_GREATER_OR_EQUAL(0xC0, encoded.data[0]);
  TEST_ASSERT_LESS_OR_EQUAL(0xF7, encoded.data[0]);
}

void test_mpt_node_encode_extension(void) {
  uint8_t path_data[] = {0xA};
  nibbles_t path = {.data = path_data, .len = 1};
  node_ref_t child = node_ref_null();

  mpt_node_t node = mpt_node_extension(path, child);
  bytes_t encoded = mpt_node_encode(&node, &test_arena);

  // Extension encoding: [hex_prefix([A], leaf=false), empty_ref]
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
  TEST_ASSERT_GREATER_OR_EQUAL(0xC0, encoded.data[0]);
}

void test_mpt_node_encode_branch_empty(void) {
  mpt_node_t node = mpt_node_branch();
  bytes_t encoded = mpt_node_encode(&node, &test_arena);

  // Branch encoding: [null_ref x 16, empty_value]
  // Each null_ref encodes as 0x80 (empty string)
  // Total: 17 items
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
}

// ===========================================================================
// Node hash tests
// ===========================================================================

void test_mpt_node_hash_empty(void) {
  mpt_node_t node = mpt_node_empty();
  hash_t hash = mpt_node_hash(&node, &test_arena);

  // Should match MPT_EMPTY_ROOT
  TEST_ASSERT_TRUE(hash_equal(&hash, &MPT_EMPTY_ROOT));
}

void test_mpt_node_hash_leaf(void) {
  uint8_t path_data[] = {1, 2, 3};
  nibbles_t path = {.data = path_data, .len = 3};

  uint8_t value_data[] = {0xFF};
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 1);

  mpt_node_t node = mpt_node_leaf(path, value);
  hash_t hash = mpt_node_hash(&node, &test_arena);

  // Hash should be computed from encoded node
  bytes_t encoded = mpt_node_encode(&node, &test_arena);
  hash_t expected = keccak256(encoded.data, encoded.size);

  TEST_ASSERT_TRUE(hash_equal(&hash, &expected));
}

void test_mpt_node_hash_caching(void) {
  uint8_t path_data[] = {5, 6};
  nibbles_t path = {.data = path_data, .len = 2};

  uint8_t value_data[] = {0x12, 0x34};
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 2);

  mpt_node_t node = mpt_node_leaf(path, value);

  TEST_ASSERT_FALSE(node.hash_valid);

  hash_t hash1 = mpt_node_hash(&node, &test_arena);
  TEST_ASSERT_TRUE(node.hash_valid);

  hash_t hash2 = mpt_node_hash(&node, &test_arena);
  TEST_ASSERT_TRUE(hash_equal(&hash1, &hash2));

  // Invalidate and recompute
  mpt_node_invalidate_hash(&node);
  TEST_ASSERT_FALSE(node.hash_valid);

  hash_t hash3 = mpt_node_hash(&node, &test_arena);
  TEST_ASSERT_TRUE(hash_equal(&hash1, &hash3));
}

// ===========================================================================
// Node ref tests
// ===========================================================================

void test_mpt_node_ref_small_embeds(void) {
  // Small leaf should be embedded
  uint8_t path_data[] = {1};
  nibbles_t path = {.data = path_data, .len = 1};

  uint8_t value_data[] = {0x01};
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 1);

  mpt_node_t node = mpt_node_leaf(path, value);
  node_ref_t ref = mpt_node_ref(&node, &test_arena);

  // Should be embedded (< 32 bytes)
  TEST_ASSERT_FALSE(ref.is_hash);
  TEST_ASSERT_GREATER_THAN(0, ref.embedded.size);
  TEST_ASSERT_LESS_THAN(32, ref.embedded.size);
}

void test_mpt_node_ref_large_hashes(void) {
  // Large leaf should be hashed
  uint8_t path_data[16];
  for (int i = 0; i < 16; i++) {
    path_data[i] = (uint8_t)i;
  }
  nibbles_t path = {.data = path_data, .len = 16};

  uint8_t value_data[32];
  memset(value_data, 0xFF, 32);
  bytes_t value;
  bytes_init_arena(&value, &test_arena);
  bytes_from_data(&value, value_data, 32);

  mpt_node_t node = mpt_node_leaf(path, value);
  node_ref_t ref = mpt_node_ref(&node, &test_arena);

  // Should be a hash (>= 32 bytes encoded)
  TEST_ASSERT_TRUE(ref.is_hash);
  TEST_ASSERT_FALSE(hash_is_zero(&ref.hash));
}

// ===========================================================================
// Branch child count tests
// ===========================================================================

void test_mpt_branch_child_count(void) {
  mpt_node_t node = mpt_node_branch();
  TEST_ASSERT_EQUAL_size_t(0, mpt_branch_child_count(&node.branch));

  // Add a child at index 5
  uint8_t data[] = {0xC0};
  node.branch.children[5].is_hash = false;
  bytes_init_arena(&node.branch.children[5].embedded, &test_arena);
  bytes_from_data(&node.branch.children[5].embedded, data, 1);

  TEST_ASSERT_EQUAL_size_t(1, mpt_branch_child_count(&node.branch));

  // Add another at index 10
  node.branch.children[10].is_hash = false;
  bytes_init_arena(&node.branch.children[10].embedded, &test_arena);
  bytes_from_data(&node.branch.children[10].embedded, data, 1);

  TEST_ASSERT_EQUAL_size_t(2, mpt_branch_child_count(&node.branch));
}

// ===========================================================================
// Empty root constant test
// ===========================================================================

void test_mpt_empty_root_constant(void) {
  // Verify MPT_EMPTY_ROOT is keccak256(0x80)
  uint8_t empty_rlp[] = {0x80};
  hash_t computed = keccak256(empty_rlp, 1);

  TEST_ASSERT_TRUE(hash_equal(&computed, &MPT_EMPTY_ROOT));
}
