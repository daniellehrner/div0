#ifndef TEST_KECCAK256_H
#define TEST_KECCAK256_H

// Ethereum test vectors
void test_keccak256_empty(void);
void test_keccak256_single_zero(void);
void test_keccak256_five_zeros(void);
void test_keccak256_ten_zeros(void);
void test_keccak256_32_zeros(void);

// Incremental API
void test_keccak256_hasher_empty(void);
void test_keccak256_hasher_single_update(void);
void test_keccak256_hasher_multiple_updates(void);
void test_keccak256_hasher_byte_by_byte(void);
void test_keccak256_hasher_reuse(void);
void test_keccak256_hasher_reset(void);

// Consistency
void test_keccak256_one_shot_matches_incremental(void);
void test_keccak256_deterministic(void);
void test_keccak256_avalanche(void);

#endif // TEST_KECCAK256_H