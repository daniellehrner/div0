#include "test_bytes32.h"

#include "div0/types/bytes32.h"

#include "unity.h"

#include <string.h>

void test_bytes32_zero_is_zero(void) {
  bytes32_t z = bytes32_zero();
  TEST_ASSERT_TRUE(bytes32_is_zero(&z));

  // Verify all bytes are zero
  for (size_t i = 0; i < BYTES32_SIZE; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, z.bytes[i]);
  }
}

void test_bytes32_from_bytes_works(void) {
  uint8_t data[BYTES32_SIZE];
  for (size_t i = 0; i < BYTES32_SIZE; i++) {
    data[i] = (uint8_t)(i + 1);
  }

  bytes32_t b = bytes32_from_bytes(data);
  TEST_ASSERT_FALSE(bytes32_is_zero(&b));
  TEST_ASSERT_EQUAL_MEMORY(data, b.bytes, BYTES32_SIZE);
}

void test_bytes32_from_bytes_padded_short(void) {
  // 4 bytes should be padded with zeros on the right
  uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
  bytes32_t b = bytes32_from_bytes_padded(data, 4);

  TEST_ASSERT_EQUAL_UINT8(0xDE, b.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAD, b.bytes[1]);
  TEST_ASSERT_EQUAL_UINT8(0xBE, b.bytes[2]);
  TEST_ASSERT_EQUAL_UINT8(0xEF, b.bytes[3]);

  // Rest should be zero
  for (size_t i = 4; i < BYTES32_SIZE; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, b.bytes[i]);
  }
}

void test_bytes32_from_bytes_padded_long(void) {
  // 40 bytes should be truncated to 32
  uint8_t data[40];
  for (size_t i = 0; i < 40; i++) {
    data[i] = (uint8_t)(i + 1);
  }

  bytes32_t b = bytes32_from_bytes_padded(data, 40);

  // Should only contain first 32 bytes
  for (size_t i = 0; i < BYTES32_SIZE; i++) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, b.bytes[i]);
  }
}

void test_bytes32_equal_works(void) {
  uint8_t data1[BYTES32_SIZE] = {0};
  uint8_t data2[BYTES32_SIZE] = {0};
  data1[0] = 0x42;
  data2[0] = 0x42;

  bytes32_t a = bytes32_from_bytes(data1);
  bytes32_t b = bytes32_from_bytes(data2);
  bytes32_t c = bytes32_zero();

  TEST_ASSERT_TRUE(bytes32_equal(&a, &b));
  TEST_ASSERT_FALSE(bytes32_equal(&a, &c));
}

void test_bytes32_to_uint256_roundtrip(void) {
  // Create bytes32 with known pattern
  uint8_t data[BYTES32_SIZE];
  for (size_t i = 0; i < BYTES32_SIZE; i++) {
    data[i] = (uint8_t)(0x20 - i);
  }

  bytes32_t original = bytes32_from_bytes(data);

  // Convert to uint256 and back
  uint256_t u = bytes32_to_uint256(&original);
  bytes32_t restored = bytes32_from_uint256(&u);

  TEST_ASSERT_TRUE(bytes32_equal(&original, &restored));
}