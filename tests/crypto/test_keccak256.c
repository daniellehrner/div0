// Unit tests for Keccak-256 cryptographic hash function
// Test vectors from official Ethereum test suite:
// https://github.com/ethereum/tests/blob/develop/src/GeneralStateTestsFiller/VMTests/vmTests/sha3Filler.yml

#include "crypto/test_keccak256.h"

#include "div0/crypto/keccak256.h"

#include "unity.h"

// Helper macro: parse hex and assert success
#define HASH_FROM_HEX(var, hex_str) \
  hash_t var;                       \
  TEST_ASSERT_TRUE_MESSAGE(hash_from_hex(hex_str, &var), "Failed to parse: " hex_str)

// =============================================================================
// Ethereum Test Vectors: Zero-Filled Inputs
// =============================================================================

void test_keccak256_empty(void) {
  // sha3(0, 0) - empty input
  // Hash: 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
  hash_t result = keccak256(NULL, 0);
  HASH_FROM_HEX(expected, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");

  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_single_zero(void) {
  // sha3(offset, 1) where memory[offset] = 0x00
  // Hash: 0xbc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a
  uint8_t input[1] = {0x00};
  hash_t result = keccak256(input, 1);
  HASH_FROM_HEX(expected, "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");

  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_five_zeros(void) {
  // sha3(4, 5) - 5 bytes of zeros
  // Hash: 0xc41589e7559804ea4a2080dad19d876a024ccb05117835447d72ce08c1d020ec
  uint8_t input[5] = {0};
  hash_t result = keccak256(input, 5);
  HASH_FROM_HEX(expected, "c41589e7559804ea4a2080dad19d876a024ccb05117835447d72ce08c1d020ec");

  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_ten_zeros(void) {
  // sha3(10, 10) - 10 bytes of zeros
  // Hash: 0x6bd2dd6bd408cbee33429358bf24fdc64612fbf8b1b4db604518f40ffd34b607
  uint8_t input[10] = {0};
  hash_t result = keccak256(input, 10);
  HASH_FROM_HEX(expected, "6bd2dd6bd408cbee33429358bf24fdc64612fbf8b1b4db604518f40ffd34b607");

  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_32_zeros(void) {
  // sha3(2016, 32) - 32 bytes of zeros
  // Hash: 0x290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563
  uint8_t input[32] = {0};
  hash_t result = keccak256(input, 32);
  HASH_FROM_HEX(expected, "290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563");

  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

// =============================================================================
// Incremental API: Keccak256Hasher
// =============================================================================

void test_keccak256_hasher_empty(void) {
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  HASH_FROM_HEX(expected, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_hasher_single_update(void) {
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);

  uint8_t input[1] = {0x00};
  keccak256_update(&hasher, input, 1);
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  HASH_FROM_HEX(expected, "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");
  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_hasher_multiple_updates(void) {
  // Hash 10 zeros in chunks of 5
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);

  uint8_t chunk[5] = {0};
  keccak256_update(&hasher, chunk, 5);
  keccak256_update(&hasher, chunk, 5);
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  HASH_FROM_HEX(expected, "6bd2dd6bd408cbee33429358bf24fdc64612fbf8b1b4db604518f40ffd34b607");
  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_hasher_byte_by_byte(void) {
  // Hash 5 zeros one byte at a time
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);

  uint8_t zero = 0x00;
  for (int i = 0; i < 5; i++) {
    keccak256_update(&hasher, &zero, 1);
  }
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  HASH_FROM_HEX(expected, "c41589e7559804ea4a2080dad19d876a024ccb05117835447d72ce08c1d020ec");
  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

void test_keccak256_hasher_reuse(void) {
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);

  // First hash: empty
  hash_t result1 = keccak256_finalize(&hasher);

  // Second hash: single zero (hasher auto-resets after finalize)
  uint8_t input[1] = {0x00};
  keccak256_update(&hasher, input, 1);
  hash_t result2 = keccak256_finalize(&hasher);

  // Third hash: empty again
  hash_t result3 = keccak256_finalize(&hasher);

  keccak256_destroy(&hasher);

  TEST_ASSERT_FALSE(hash_equal(&result1, &result2));
  TEST_ASSERT_TRUE(hash_equal(&result1, &result3));
}

void test_keccak256_hasher_reset(void) {
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);

  // Start hashing something
  uint8_t garbage[100] = {0};
  keccak256_update(&hasher, garbage, 100);

  // Reset before finalize
  keccak256_reset(&hasher);

  // Now hash single zero
  uint8_t input[1] = {0x00};
  keccak256_update(&hasher, input, 1);
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  // Should match clean single-zero hash
  HASH_FROM_HEX(expected, "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");
  TEST_ASSERT_TRUE(hash_equal(&result, &expected));
}

// =============================================================================
// Consistency: One-Shot vs Incremental
// =============================================================================

void test_keccak256_one_shot_matches_incremental(void) {
  // Test with input at Keccak block boundary (136 bytes)
  uint8_t input[136];
  for (size_t i = 0; i < 136; i++) {
    input[i] = (uint8_t)(i & 0xFF);
  }

  // One-shot
  hash_t one_shot = keccak256(input, 136);

  // Incremental with 32-byte chunks
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);
  for (size_t offset = 0; offset < 136; offset += 32) {
    size_t remaining = 136 - offset;
    size_t chunk = remaining < 32 ? remaining : 32;
    keccak256_update(&hasher, input + offset, chunk);
  }
  hash_t incremental = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);

  TEST_ASSERT_TRUE(hash_equal(&one_shot, &incremental));
}

void test_keccak256_deterministic(void) {
  // Same input must always produce same output
  uint8_t input[256];
  for (size_t i = 0; i < 256; i++) {
    input[i] = (uint8_t)i;
  }

  hash_t result1 = keccak256(input, 256);
  hash_t result2 = keccak256(input, 256);
  hash_t result3 = keccak256(input, 256);

  TEST_ASSERT_TRUE(hash_equal(&result1, &result2));
  TEST_ASSERT_TRUE(hash_equal(&result2, &result3));
}

void test_keccak256_avalanche(void) {
  // Changing a single bit should drastically change the hash
  uint8_t input1[32] = {0};
  uint8_t input2[32] = {0};
  input2[0] = 0x01; // Flip one bit

  hash_t hash1 = keccak256(input1, 32);
  hash_t hash2 = keccak256(input2, 32);

  TEST_ASSERT_FALSE(hash_equal(&hash1, &hash2));

  // Verify 32 zeros matches known Ethereum vector
  HASH_FROM_HEX(expected, "290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563");
  TEST_ASSERT_TRUE(hash_equal(&hash1, &expected));
}