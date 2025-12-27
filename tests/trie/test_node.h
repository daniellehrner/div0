#ifndef TEST_NODE_H
#define TEST_NODE_H

// Node creation tests
void test_mpt_node_empty(void);
void test_mpt_node_leaf(void);
void test_mpt_node_extension(void);
void test_mpt_node_branch(void);

// Node reference tests
void test_node_ref_null(void);
void test_node_ref_is_null(void);

// Node encoding tests
void test_mpt_node_encode_empty(void);
void test_mpt_node_encode_leaf(void);
void test_mpt_node_encode_extension(void);
void test_mpt_node_encode_branch_empty(void);

// Node hash tests
void test_mpt_node_hash_empty(void);
void test_mpt_node_hash_leaf(void);
void test_mpt_node_hash_caching(void);

// Node ref tests
void test_mpt_node_ref_small_embeds(void);
void test_mpt_node_ref_large_hashes(void);

// Branch child count tests
void test_mpt_branch_child_count(void);

// Empty root constant test
void test_mpt_empty_root_constant(void);

#endif // TEST_NODE_H
