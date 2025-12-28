#include "test_account.h"

#include "div0/crypto/keccak256.h"
#include "div0/state/account.h"
#include "div0/trie/node.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

// ===========================================================================
// Account creation tests
// ===========================================================================

void test_account_empty_creation(void) {
  account_t acc = account_empty();

  TEST_ASSERT_EQUAL_UINT64(0, acc.nonce);
  TEST_ASSERT_TRUE(uint256_is_zero(acc.balance));
}

void test_account_empty_has_correct_defaults(void) {
  account_t acc = account_empty();

  // Storage root should be empty MPT root
  TEST_ASSERT_TRUE(hash_equal(&acc.storage_root, &MPT_EMPTY_ROOT));

  // Code hash should be empty code hash
  TEST_ASSERT_TRUE(hash_equal(&acc.code_hash, &EMPTY_CODE_HASH));
}

void test_account_is_empty_checks_correctly(void) {
  account_t acc = account_empty();
  TEST_ASSERT_TRUE(account_is_empty(&acc));

  // Non-zero nonce makes it non-empty
  acc.nonce = 1;
  TEST_ASSERT_FALSE(account_is_empty(&acc));
  acc.nonce = 0;

  // Non-zero balance makes it non-empty
  acc.balance = uint256_from_u64(1);
  TEST_ASSERT_FALSE(account_is_empty(&acc));
  acc.balance = uint256_zero();

  // Different code hash makes it non-empty
  acc.code_hash = hash_zero();
  TEST_ASSERT_FALSE(account_is_empty(&acc));
  acc.code_hash = EMPTY_CODE_HASH;

  // Storage root does NOT affect emptiness (by EIP-161)
  acc.storage_root = hash_zero();
  TEST_ASSERT_TRUE(account_is_empty(&acc));
}

// ===========================================================================
// Account RLP encoding tests
// ===========================================================================

void test_account_rlp_encode_empty(void) {
  account_t acc = account_empty();

  bytes_t encoded = account_rlp_encode(&acc, &test_arena);
  TEST_ASSERT_NOT_NULL(encoded.data);
  TEST_ASSERT_GREATER_THAN(0, encoded.size);

  // Should be a list with 4 items
  // First byte should be list prefix (0xc0 + length for short list)
  TEST_ASSERT_GREATER_OR_EQUAL(0xc0, encoded.data[0]);
}

void test_account_rlp_encode_with_balance(void) {
  account_t acc = account_empty();
  acc.balance = uint256_from_u64(1000000);

  bytes_t encoded = account_rlp_encode(&acc, &test_arena);
  TEST_ASSERT_NOT_NULL(encoded.data);
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
}

void test_account_rlp_encode_with_nonce(void) {
  account_t acc = account_empty();
  acc.nonce = 42;

  bytes_t encoded = account_rlp_encode(&acc, &test_arena);
  TEST_ASSERT_NOT_NULL(encoded.data);
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
}

void test_account_rlp_encode_full_account(void) {
  account_t acc = {
      .nonce = 100,
      .balance = uint256_from_u64(1000000000000ULL),
      .storage_root = MPT_EMPTY_ROOT,
      .code_hash = EMPTY_CODE_HASH,
  };

  bytes_t encoded = account_rlp_encode(&acc, &test_arena);
  TEST_ASSERT_NOT_NULL(encoded.data);
  TEST_ASSERT_GREATER_THAN(0, encoded.size);
}

// ===========================================================================
// Account RLP decoding tests
// ===========================================================================

void test_account_rlp_decode_empty(void) {
  account_t original = account_empty();
  bytes_t encoded = account_rlp_encode(&original, &test_arena);

  account_t decoded;
  bool result = account_rlp_decode(encoded.data, encoded.size, &decoded);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT64(original.nonce, decoded.nonce);
  TEST_ASSERT_TRUE(uint256_eq(original.balance, decoded.balance));
  TEST_ASSERT_TRUE(hash_equal(&original.storage_root, &decoded.storage_root));
  TEST_ASSERT_TRUE(hash_equal(&original.code_hash, &decoded.code_hash));
}

void test_account_rlp_decode_with_balance(void) {
  account_t original = account_empty();
  original.balance = uint256_from_u64(123456789);

  bytes_t encoded = account_rlp_encode(&original, &test_arena);

  account_t decoded;
  bool result = account_rlp_decode(encoded.data, encoded.size, &decoded);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_TRUE(uint256_eq(original.balance, decoded.balance));
}

void test_account_rlp_roundtrip(void) {
  // Test with various account configurations
  account_t accounts[] = {
      account_empty(),
      {.nonce = 1,
       .balance = uint256_from_u64(1),
       .storage_root = MPT_EMPTY_ROOT,
       .code_hash = EMPTY_CODE_HASH},
      {.nonce = UINT64_MAX,
       .balance = uint256_from_limbs(UINT64_MAX, UINT64_MAX, 0, 0),
       .storage_root = MPT_EMPTY_ROOT,
       .code_hash = EMPTY_CODE_HASH},
  };

  for (size_t i = 0; i < sizeof(accounts) / sizeof(accounts[0]); i++) {
    bytes_t encoded = account_rlp_encode(&accounts[i], &test_arena);
    TEST_ASSERT_NOT_NULL(encoded.data);

    account_t decoded;
    bool result = account_rlp_decode(encoded.data, encoded.size, &decoded);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT64(accounts[i].nonce, decoded.nonce);
    TEST_ASSERT_TRUE(uint256_eq(accounts[i].balance, decoded.balance));
    TEST_ASSERT_TRUE(hash_equal(&accounts[i].storage_root, &decoded.storage_root));
    TEST_ASSERT_TRUE(hash_equal(&accounts[i].code_hash, &decoded.code_hash));
  }
}

void test_account_rlp_decode_invalid_returns_false(void) {
  account_t decoded;

  // nullptr data
  TEST_ASSERT_FALSE(account_rlp_decode(nullptr, 0, &decoded));

  // Empty data
  uint8_t empty[] = {};
  TEST_ASSERT_FALSE(account_rlp_decode(empty, 0, &decoded));

  // Invalid RLP (not a list)
  uint8_t not_list[] = {0x80}; // Empty string
  TEST_ASSERT_FALSE(account_rlp_decode(not_list, sizeof(not_list), &decoded));

  // Truncated data
  uint8_t truncated[] = {0xc4, 0x01, 0x02}; // List header says 4 bytes, but only 2 items
  TEST_ASSERT_FALSE(account_rlp_decode(truncated, sizeof(truncated), &decoded));
}

// ===========================================================================
// Empty code hash constant test
// ===========================================================================

void test_empty_code_hash_constant(void) {
  // Verify EMPTY_CODE_HASH is keccak256("")
  hash_t computed = keccak256(nullptr, 0);

  TEST_ASSERT_EQUAL_MEMORY(computed.bytes, EMPTY_CODE_HASH.bytes, 32);

  // Also check the known value
  uint8_t expected[] = {0xc5, 0xd2, 0x46, 0x01, 0x86, 0xf7, 0x23, 0x3c, 0x92, 0x7e, 0x7d,
                        0xb2, 0xdc, 0xc7, 0x03, 0xc0, 0xe5, 0x00, 0xb6, 0x53, 0xca, 0x82,
                        0x27, 0x3b, 0x7b, 0xfa, 0xd8, 0x04, 0x5d, 0x85, 0xa4, 0x70};
  TEST_ASSERT_EQUAL_MEMORY(expected, EMPTY_CODE_HASH.bytes, 32);
}
