// Unit tests for hex utility functions

#include "util/test_hex.h"

#include "div0/util/hex.h"

#include "unity.h"

#include <string.h>

// =============================================================================
// hex_char_to_nibble Tests
// =============================================================================

void test_hex_char_to_nibble_digits(void) {
  uint8_t nibble;

  TEST_ASSERT_TRUE(hex_char_to_nibble('0', &nibble));
  TEST_ASSERT_EQUAL_UINT8(0, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('5', &nibble));
  TEST_ASSERT_EQUAL_UINT8(5, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('9', &nibble));
  TEST_ASSERT_EQUAL_UINT8(9, nibble);
}

void test_hex_char_to_nibble_lowercase(void) {
  uint8_t nibble;

  TEST_ASSERT_TRUE(hex_char_to_nibble('a', &nibble));
  TEST_ASSERT_EQUAL_UINT8(10, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('c', &nibble));
  TEST_ASSERT_EQUAL_UINT8(12, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('f', &nibble));
  TEST_ASSERT_EQUAL_UINT8(15, nibble);
}

void test_hex_char_to_nibble_uppercase(void) {
  uint8_t nibble;

  TEST_ASSERT_TRUE(hex_char_to_nibble('A', &nibble));
  TEST_ASSERT_EQUAL_UINT8(10, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('C', &nibble));
  TEST_ASSERT_EQUAL_UINT8(12, nibble);

  TEST_ASSERT_TRUE(hex_char_to_nibble('F', &nibble));
  TEST_ASSERT_EQUAL_UINT8(15, nibble);
}

void test_hex_char_to_nibble_invalid(void) {
  uint8_t nibble = 0xFF;

  TEST_ASSERT_FALSE(hex_char_to_nibble('g', &nibble));
  TEST_ASSERT_FALSE(hex_char_to_nibble('G', &nibble));
  TEST_ASSERT_FALSE(hex_char_to_nibble(' ', &nibble));
  TEST_ASSERT_FALSE(hex_char_to_nibble('\0', &nibble));
  TEST_ASSERT_FALSE(hex_char_to_nibble('/', &nibble));
  TEST_ASSERT_FALSE(hex_char_to_nibble(':', &nibble));
}

// =============================================================================
// hex_decode Tests
// =============================================================================

void test_hex_decode_basic(void) {
  uint8_t out[4];
  TEST_ASSERT_TRUE(hex_decode("deadbeef", out, 4));
  TEST_ASSERT_EQUAL_UINT8(0xde, out[0]);
  TEST_ASSERT_EQUAL_UINT8(0xad, out[1]);
  TEST_ASSERT_EQUAL_UINT8(0xbe, out[2]);
  TEST_ASSERT_EQUAL_UINT8(0xef, out[3]);
}

void test_hex_decode_with_prefix(void) {
  uint8_t out[4];

  // 0x prefix (lowercase)
  TEST_ASSERT_TRUE(hex_decode("0xdeadbeef", out, 4));
  TEST_ASSERT_EQUAL_UINT8(0xde, out[0]);
  TEST_ASSERT_EQUAL_UINT8(0xef, out[3]);

  // 0X prefix (uppercase)
  TEST_ASSERT_TRUE(hex_decode("0XDEADBEEF", out, 4));
  TEST_ASSERT_EQUAL_UINT8(0xde, out[0]);
  TEST_ASSERT_EQUAL_UINT8(0xef, out[3]);
}

void test_hex_decode_uppercase(void) {
  uint8_t out[4];
  TEST_ASSERT_TRUE(hex_decode("DEADBEEF", out, 4));
  TEST_ASSERT_EQUAL_UINT8(0xde, out[0]);
  TEST_ASSERT_EQUAL_UINT8(0xad, out[1]);
  TEST_ASSERT_EQUAL_UINT8(0xbe, out[2]);
  TEST_ASSERT_EQUAL_UINT8(0xef, out[3]);
}

void test_hex_decode_mixed_case(void) {
  uint8_t out[4];
  TEST_ASSERT_TRUE(hex_decode("DeAdBeEf", out, 4));
  TEST_ASSERT_EQUAL_UINT8(0xde, out[0]);
  TEST_ASSERT_EQUAL_UINT8(0xad, out[1]);
  TEST_ASSERT_EQUAL_UINT8(0xbe, out[2]);
  TEST_ASSERT_EQUAL_UINT8(0xef, out[3]);
}

void test_hex_decode_null_input(void) {
  uint8_t out[4];
  TEST_ASSERT_FALSE(hex_decode(nullptr, out, 4));
}

void test_hex_decode_null_output(void) {
  TEST_ASSERT_FALSE(hex_decode("deadbeef", nullptr, 4));
}

void test_hex_decode_wrong_length(void) {
  uint8_t out[4];

  // Too short
  TEST_ASSERT_FALSE(hex_decode("deadbe", out, 4));

  // Too long
  TEST_ASSERT_FALSE(hex_decode("deadbeefcafe", out, 4));

  // Odd length (after prefix strip)
  TEST_ASSERT_FALSE(hex_decode("0xdeadbee", out, 4));
}

void test_hex_decode_invalid_char(void) {
  uint8_t out[4];

  // Invalid character in first position
  TEST_ASSERT_FALSE(hex_decode("geadbeef", out, 4));

  // Invalid character in middle
  TEST_ASSERT_FALSE(hex_decode("deagbeef", out, 4));

  // Invalid character in last position
  TEST_ASSERT_FALSE(hex_decode("deadbeeg", out, 4));

  // Space in string
  TEST_ASSERT_FALSE(hex_decode("dead beef", out, 4));
}