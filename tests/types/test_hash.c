#include "test_hash.h"

#include "div0/types/hash.h"

#include "unity.h"

#include <stdalign.h>
#include <stdint.h>
#include <string.h>

void test_hash_zero_is_zero(void) {
  hash_t z = hash_zero();
  TEST_ASSERT_TRUE(hash_is_zero(&z));

  // Verify all bytes are zero
  for (size_t i = 0; i < HASH_SIZE; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, z.bytes[i]);
  }
}

void test_hash_from_bytes_works(void) {
  uint8_t data[HASH_SIZE];
  for (size_t i = 0; i < HASH_SIZE; i++) {
    data[i] = (uint8_t)(i + 1);
  }

  hash_t h = hash_from_bytes(data);
  TEST_ASSERT_FALSE(hash_is_zero(&h));
  TEST_ASSERT_EQUAL_MEMORY(data, h.bytes, HASH_SIZE);
}

void test_hash_equal_works(void) {
  uint8_t data1[HASH_SIZE] = {0};
  uint8_t data2[HASH_SIZE] = {0};
  data1[0] = 0x42;
  data2[0] = 0x42;

  hash_t a = hash_from_bytes(data1);
  hash_t b = hash_from_bytes(data2);
  hash_t c = hash_zero();

  TEST_ASSERT_TRUE(hash_equal(&a, &b));
  TEST_ASSERT_FALSE(hash_equal(&a, &c));
}

void test_hash_to_uint256_roundtrip(void) {
  // Create hash with known pattern
  uint8_t data[HASH_SIZE];
  for (size_t i = 0; i < HASH_SIZE; i++) {
    data[i] = (uint8_t)(0xFF - i);
  }

  hash_t original = hash_from_bytes(data);

  // Convert to uint256 and back
  uint256_t u = hash_to_uint256(&original);
  hash_t restored = hash_from_uint256(&u);

  TEST_ASSERT_TRUE(hash_equal(&original, &restored));
}

void test_hash_alignment(void) {
  hash_t h = hash_zero();
  // Verify hash is 32-byte aligned
  TEST_ASSERT_EQUAL_size_t(32, alignof(hash_t));
  // Verify address is aligned (pointer must be divisible by 32)
  TEST_ASSERT_EQUAL_UINT64(0, ((uintptr_t)&h) % 32);
}