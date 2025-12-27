#ifndef TEST_SECP256K1_H
#define TEST_SECP256K1_H

// Basic functionality
void test_secp256k1_ctx_create_destroy(void);

// Ecrecover tests
void test_secp256k1_ecrecover_reth_vector(void);
void test_secp256k1_ecrecover_v28(void);

// EIP-155 tests
void test_secp256k1_ecrecover_eip155(void);
void test_secp256k1_ecrecover_eip155_wrong_chain_id(void);

// Invalid inputs
void test_secp256k1_ecrecover_invalid_v(void);
void test_secp256k1_ecrecover_zero_signature(void);

// recover_public_key tests
void test_secp256k1_recover_pubkey_invalid_recovery_id(void);

#endif // TEST_SECP256K1_H