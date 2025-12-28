// Unit tests for secp256k1 ECDSA signature recovery
// Test vectors from Ethereum transactions and the ecrecover precompile

#include "crypto/test_secp256k1.h"

#include "div0/crypto/secp256k1.h"

#include "unity.h"

#include <string.h>

// =============================================================================
// Test Vector from reth
// =============================================================================
// Hash: 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
// R: 0x28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276
// S: 0x67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83
// V: 27 (recovery_id = 0)
// Expected signer: 0x9d8a62f656a8d1615c1294fd71e9cfb3e4855a4f

static uint256_t get_test_hash(void) {
  uint256_t result;
  bool ok =
      uint256_from_hex("daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53", &result);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Failed to parse test hash");
  return result;
}

static uint256_t get_test_r(void) {
  uint256_t result;
  bool ok =
      uint256_from_hex("28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276", &result);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Failed to parse test r value");
  return result;
}

static uint256_t get_test_s(void) {
  uint256_t result;
  bool ok =
      uint256_from_hex("67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83", &result);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Failed to parse test s value");
  return result;
}

static address_t get_expected_address(void) {
  address_t result;
  bool ok = address_from_hex("9d8a62f656a8d1615c1294fd71e9cfb3e4855a4f", &result);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Failed to parse expected address");
  return result;
}

// =============================================================================
// Basic Functionality
// =============================================================================

void test_secp256k1_ctx_create_destroy(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);
  secp256k1_ctx_destroy(ctx);

  // Should be safe to destroy NULL
  secp256k1_ctx_destroy(NULL);
}

// =============================================================================
// Ecrecover Tests
// =============================================================================

void test_secp256k1_ecrecover_reth_vector(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = get_test_hash();
  uint256_t r = get_test_r();
  uint256_t s = get_test_s();
  address_t expected = get_expected_address();

  ecrecover_result_t result = secp256k1_ecrecover(ctx, &hash, 27, &r, &s, 0);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_TRUE(address_equal(&result.address, &expected));

  secp256k1_ctx_destroy(ctx);
}

void test_secp256k1_ecrecover_v28(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = get_test_hash();
  uint256_t r = get_test_r();
  uint256_t s = get_test_s();

  // Get result with v=27 (known good)
  ecrecover_result_t result_27 = secp256k1_ecrecover(ctx, &hash, 27, &r, &s, 0);
  TEST_ASSERT_TRUE(result_27.success);

  // With wrong recovery ID (v=28), either fails or returns different address
  ecrecover_result_t result_28 = secp256k1_ecrecover(ctx, &hash, 28, &r, &s, 0);
  if (result_28.success) {
    TEST_ASSERT_FALSE(address_equal(&result_27.address, &result_28.address));
  }

  secp256k1_ctx_destroy(ctx);
}

// =============================================================================
// EIP-155 Chain ID Encoding
// =============================================================================

void test_secp256k1_ecrecover_eip155(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = get_test_hash();
  uint256_t r = get_test_r();
  uint256_t s = get_test_s();

  // For chain_id = 1 (mainnet):
  // v = chain_id * 2 + 35 + recovery_id
  // v = 1 * 2 + 35 + 0 = 37 for recovery_id = 0

  // v=27 (pre-EIP-155) should give same result as v=37 with chain_id=1
  ecrecover_result_t result_pre_eip155 = secp256k1_ecrecover(ctx, &hash, 27, &r, &s, 0);
  ecrecover_result_t result_eip155 = secp256k1_ecrecover(ctx, &hash, 37, &r, &s, 1);

  TEST_ASSERT_TRUE(result_pre_eip155.success);
  TEST_ASSERT_TRUE(result_eip155.success);
  TEST_ASSERT_TRUE(address_equal(&result_pre_eip155.address, &result_eip155.address));

  secp256k1_ctx_destroy(ctx);
}

void test_secp256k1_ecrecover_eip155_wrong_chain_id(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = get_test_hash();
  uint256_t r = get_test_r();
  uint256_t s = get_test_s();

  // v=37 with wrong chain_id should fail
  // chain_id=5 expects v=45 or 46, not 37
  ecrecover_result_t result = secp256k1_ecrecover(ctx, &hash, 37, &r, &s, 5);

  TEST_ASSERT_FALSE(result.success);

  secp256k1_ctx_destroy(ctx);
}

// =============================================================================
// Invalid Inputs
// =============================================================================

void test_secp256k1_ecrecover_invalid_v(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = uint256_from_u64(1);
  uint256_t r = uint256_from_u64(1);
  uint256_t s = uint256_from_u64(1);

  // Invalid v values (note: v=0,1 are now valid for typed transactions)
  TEST_ASSERT_FALSE(secp256k1_ecrecover(ctx, &hash, 2, &r, &s, 0).success);
  TEST_ASSERT_FALSE(secp256k1_ecrecover(ctx, &hash, 26, &r, &s, 0).success);
  TEST_ASSERT_FALSE(secp256k1_ecrecover(ctx, &hash, 29, &r, &s, 0).success);
  TEST_ASSERT_FALSE(secp256k1_ecrecover(ctx, &hash, 34, &r, &s, 0).success);

  secp256k1_ctx_destroy(ctx);
}

void test_secp256k1_ecrecover_zero_signature(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint256_t hash = uint256_from_u64(1);
  uint256_t r = uint256_zero();
  uint256_t s = uint256_zero();

  // Zero r,s should fail
  ecrecover_result_t result = secp256k1_ecrecover(ctx, &hash, 27, &r, &s, 0);
  TEST_ASSERT_FALSE(result.success);

  secp256k1_ctx_destroy(ctx);
}

// =============================================================================
// recover_public_key Tests
// =============================================================================

void test_secp256k1_recover_pubkey_invalid_recovery_id(void) {
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  uint8_t hash[32] = {0};
  uint8_t sig[64] = {0};

  // Invalid recovery IDs
  TEST_ASSERT_FALSE(secp256k1_recover_pubkey(ctx, hash, -1, sig).success);
  TEST_ASSERT_FALSE(secp256k1_recover_pubkey(ctx, hash, 4, sig).success);

  secp256k1_ctx_destroy(ctx);
}