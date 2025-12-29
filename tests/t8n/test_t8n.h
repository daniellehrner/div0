#ifndef TEST_T8N_H
#define TEST_T8N_H

// Alloc parsing tests
void test_alloc_parse_empty(void);
void test_alloc_parse_single_account(void);
void test_alloc_parse_with_storage(void);
void test_alloc_parse_with_code(void);
void test_alloc_roundtrip(void);

// Env parsing tests
void test_env_parse_required_fields(void);
void test_env_parse_optional_fields(void);
void test_env_parse_block_hashes(void);
void test_env_parse_withdrawals(void);

// Txs parsing tests
void test_txs_parse_legacy(void);
void test_txs_parse_eip1559(void);
void test_txs_parse_empty_array(void);

#endif // TEST_T8N_H
