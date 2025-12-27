#include "test_hex_prefix.h"

#include "div0/trie/hex_prefix.h"

#include "unity.h"

// External arena from main test file
extern div0_arena_t test_arena;

// ===========================================================================
// hex_prefix_encode tests
// ===========================================================================

void test_hex_prefix_encode_odd_extension(void) {
  // [1, 2, 3, 4, 5], leaf=false -> [0x11, 0x23, 0x45]
  uint8_t nibble_data[] = {1, 2, 3, 4, 5};
  nibbles_t nibbles = {.data = nibble_data, .len = 5};

  bytes_t result = hex_prefix_encode(&nibbles, false, &test_arena);

  TEST_ASSERT_EQUAL_size_t(3, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x11, result.data[0]); // flags=0x01 (odd), first nibble=1
  TEST_ASSERT_EQUAL_UINT8(0x23, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x45, result.data[2]);
}

void test_hex_prefix_encode_even_extension(void) {
  // [0, 1, 2, 3, 4, 5], leaf=false -> [0x00, 0x01, 0x23, 0x45]
  uint8_t nibble_data[] = {0, 1, 2, 3, 4, 5};
  nibbles_t nibbles = {.data = nibble_data, .len = 6};

  bytes_t result = hex_prefix_encode(&nibbles, false, &test_arena);

  TEST_ASSERT_EQUAL_size_t(4, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x00, result.data[0]); // flags=0x00 (even, extension)
  TEST_ASSERT_EQUAL_UINT8(0x01, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x23, result.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0x45, result.data[3]);
}

void test_hex_prefix_encode_odd_leaf(void) {
  // [1, 2, 3, 4, 5], leaf=true -> [0x31, 0x23, 0x45]
  uint8_t nibble_data[] = {1, 2, 3, 4, 5};
  nibbles_t nibbles = {.data = nibble_data, .len = 5};

  bytes_t result = hex_prefix_encode(&nibbles, true, &test_arena);

  TEST_ASSERT_EQUAL_size_t(3, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x31, result.data[0]); // flags=0x03 (odd+leaf), first nibble=1
  TEST_ASSERT_EQUAL_UINT8(0x23, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x45, result.data[2]);
}

void test_hex_prefix_encode_even_leaf(void) {
  // [0, 1, 2, 3, 4, 5], leaf=true -> [0x20, 0x01, 0x23, 0x45]
  uint8_t nibble_data[] = {0, 1, 2, 3, 4, 5};
  nibbles_t nibbles = {.data = nibble_data, .len = 6};

  bytes_t result = hex_prefix_encode(&nibbles, true, &test_arena);

  TEST_ASSERT_EQUAL_size_t(4, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x20, result.data[0]); // flags=0x02 (even, leaf)
  TEST_ASSERT_EQUAL_UINT8(0x01, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x23, result.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0x45, result.data[3]);
}

void test_hex_prefix_encode_empty(void) {
  // [], leaf=true -> [0x20] (even, leaf, no nibbles)
  nibbles_t nibbles = NIBBLES_EMPTY;

  bytes_t result = hex_prefix_encode(&nibbles, true, &test_arena);

  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x20, result.data[0]); // flags=0x02 (even, leaf)
}

void test_hex_prefix_encode_single_nibble(void) {
  // [0x0F], leaf=true -> [0x3F] (odd, leaf, nibble=F)
  uint8_t nibble_data[] = {0x0F};
  nibbles_t nibbles = {.data = nibble_data, .len = 1};

  bytes_t result = hex_prefix_encode(&nibbles, true, &test_arena);

  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x3F, result.data[0]); // flags=0x03 (odd+leaf), nibble=F
}

// ===========================================================================
// hex_prefix_decode tests
// ===========================================================================

void test_hex_prefix_decode_odd_extension(void) {
  // [0x11, 0x23, 0x45] -> [1, 2, 3, 4, 5], leaf=false
  uint8_t data[] = {0x11, 0x23, 0x45};

  hex_prefix_result_t result = hex_prefix_decode(data, 3, &test_arena);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_FALSE(result.is_leaf);
  TEST_ASSERT_EQUAL_size_t(5, result.nibbles.len);
  TEST_ASSERT_EQUAL_UINT8(1, result.nibbles.data[0]);
  TEST_ASSERT_EQUAL_UINT8(2, result.nibbles.data[1]);
  TEST_ASSERT_EQUAL_UINT8(3, result.nibbles.data[2]);
  TEST_ASSERT_EQUAL_UINT8(4, result.nibbles.data[3]);
  TEST_ASSERT_EQUAL_UINT8(5, result.nibbles.data[4]);
}

void test_hex_prefix_decode_even_extension(void) {
  // [0x00, 0x01, 0x23, 0x45] -> [0, 1, 2, 3, 4, 5], leaf=false
  uint8_t data[] = {0x00, 0x01, 0x23, 0x45};

  hex_prefix_result_t result = hex_prefix_decode(data, 4, &test_arena);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_FALSE(result.is_leaf);
  TEST_ASSERT_EQUAL_size_t(6, result.nibbles.len);
  TEST_ASSERT_EQUAL_UINT8(0, result.nibbles.data[0]);
  TEST_ASSERT_EQUAL_UINT8(1, result.nibbles.data[1]);
  TEST_ASSERT_EQUAL_UINT8(2, result.nibbles.data[2]);
  TEST_ASSERT_EQUAL_UINT8(3, result.nibbles.data[3]);
  TEST_ASSERT_EQUAL_UINT8(4, result.nibbles.data[4]);
  TEST_ASSERT_EQUAL_UINT8(5, result.nibbles.data[5]);
}

void test_hex_prefix_decode_odd_leaf(void) {
  // [0x31, 0x23, 0x45] -> [1, 2, 3, 4, 5], leaf=true
  uint8_t data[] = {0x31, 0x23, 0x45};

  hex_prefix_result_t result = hex_prefix_decode(data, 3, &test_arena);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_TRUE(result.is_leaf);
  TEST_ASSERT_EQUAL_size_t(5, result.nibbles.len);
  TEST_ASSERT_EQUAL_UINT8(1, result.nibbles.data[0]);
  TEST_ASSERT_EQUAL_UINT8(2, result.nibbles.data[1]);
  TEST_ASSERT_EQUAL_UINT8(3, result.nibbles.data[2]);
  TEST_ASSERT_EQUAL_UINT8(4, result.nibbles.data[3]);
  TEST_ASSERT_EQUAL_UINT8(5, result.nibbles.data[4]);
}

void test_hex_prefix_decode_even_leaf(void) {
  // [0x20, 0x01, 0x23, 0x45] -> [0, 1, 2, 3, 4, 5], leaf=true
  uint8_t data[] = {0x20, 0x01, 0x23, 0x45};

  hex_prefix_result_t result = hex_prefix_decode(data, 4, &test_arena);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_TRUE(result.is_leaf);
  TEST_ASSERT_EQUAL_size_t(6, result.nibbles.len);
  TEST_ASSERT_EQUAL_UINT8(0, result.nibbles.data[0]);
  TEST_ASSERT_EQUAL_UINT8(1, result.nibbles.data[1]);
  TEST_ASSERT_EQUAL_UINT8(2, result.nibbles.data[2]);
  TEST_ASSERT_EQUAL_UINT8(3, result.nibbles.data[3]);
  TEST_ASSERT_EQUAL_UINT8(4, result.nibbles.data[4]);
  TEST_ASSERT_EQUAL_UINT8(5, result.nibbles.data[5]);
}

void test_hex_prefix_decode_empty(void) {
  // [0x20] -> [], leaf=true
  uint8_t data[] = {0x20};

  hex_prefix_result_t result = hex_prefix_decode(data, 1, &test_arena);

  TEST_ASSERT_TRUE(result.success);
  TEST_ASSERT_TRUE(result.is_leaf);
  TEST_ASSERT_EQUAL_size_t(0, result.nibbles.len);
}

void test_hex_prefix_decode_null_input(void) {
  hex_prefix_result_t result = hex_prefix_decode(NULL, 0, &test_arena);

  TEST_ASSERT_FALSE(result.success);
}

// ===========================================================================
// roundtrip tests
// ===========================================================================

void test_hex_prefix_roundtrip_various(void) {
  // Test various nibble sequences with both leaf and extension
  struct {
    uint8_t nibbles[10];
    size_t len;
  } test_cases[] = {
      {{}, 0},                        // Empty
      {{5}, 1},                       // Single nibble
      {{1, 2}, 2},                    // Even pair
      {{1, 2, 3}, 3},                 // Odd triple
      {{0, 1, 2, 3, 4, 5, 6, 7}, 8},  // Even long
      {{0xA, 0xB, 0xC, 0xD, 0xE}, 5}, // Odd with high nibbles
  };

  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
    nibbles_t original = {.data = test_cases[i].nibbles, .len = test_cases[i].len};

    // Test as extension node
    bytes_t encoded_ext = hex_prefix_encode(&original, false, &test_arena);
    hex_prefix_result_t decoded_ext =
        hex_prefix_decode(encoded_ext.data, encoded_ext.size, &test_arena);

    TEST_ASSERT_TRUE(decoded_ext.success);
    TEST_ASSERT_FALSE(decoded_ext.is_leaf);
    TEST_ASSERT_EQUAL_size_t(original.len, decoded_ext.nibbles.len);
    for (size_t j = 0; j < original.len; j++) {
      TEST_ASSERT_EQUAL_UINT8(original.data[j], decoded_ext.nibbles.data[j]);
    }

    // Test as leaf node
    bytes_t encoded_leaf = hex_prefix_encode(&original, true, &test_arena);
    hex_prefix_result_t decoded_leaf =
        hex_prefix_decode(encoded_leaf.data, encoded_leaf.size, &test_arena);

    TEST_ASSERT_TRUE(decoded_leaf.success);
    TEST_ASSERT_TRUE(decoded_leaf.is_leaf);
    TEST_ASSERT_EQUAL_size_t(original.len, decoded_leaf.nibbles.len);
    for (size_t j = 0; j < original.len; j++) {
      TEST_ASSERT_EQUAL_UINT8(original.data[j], decoded_leaf.nibbles.data[j]);
    }
  }
}
