#ifndef TEST_ACCOUNT_H
#define TEST_ACCOUNT_H

// Account creation tests
void test_account_empty_creation(void);
void test_account_empty_has_correct_defaults(void);
void test_account_is_empty_checks_correctly(void);

// Account RLP encoding tests
void test_account_rlp_encode_empty(void);
void test_account_rlp_encode_with_balance(void);
void test_account_rlp_encode_with_nonce(void);
void test_account_rlp_encode_full_account(void);

// Account RLP decoding tests
void test_account_rlp_decode_empty(void);
void test_account_rlp_decode_with_balance(void);
void test_account_rlp_roundtrip(void);
void test_account_rlp_decode_invalid_returns_false(void);

// Empty code hash constant test
void test_empty_code_hash_constant(void);

#endif // TEST_ACCOUNT_H
