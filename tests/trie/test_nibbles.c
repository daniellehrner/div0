#include "test_nibbles.h"

#include "div0/trie/nibbles.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

// ===========================================================================
// nibbles_from_bytes tests
// ===========================================================================

void test_nibbles_from_bytes_empty(void) {
  nibbles_t result = nibbles_from_bytes(NULL, 0, &test_arena);
  TEST_ASSERT_EQUAL_size_t(0, result.len);
  TEST_ASSERT_NULL(result.data);
}

void test_nibbles_from_bytes_single(void) {
  // 0xAB -> [0x0A, 0x0B]
  uint8_t input[] = {0xAB};
  nibbles_t result = nibbles_from_bytes(input, 1, &test_arena);

  TEST_ASSERT_EQUAL_size_t(2, result.len);
  TEST_ASSERT_NOT_NULL(result.data);
  TEST_ASSERT_EQUAL_UINT8(0x0A, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x0B, result.data[1]);
}

void test_nibbles_from_bytes_multiple(void) {
  // 0x12 0x34 0x56 -> [0x01, 0x02, 0x03, 0x04, 0x05, 0x06]
  uint8_t input[] = {0x12, 0x34, 0x56};
  nibbles_t result = nibbles_from_bytes(input, 3, &test_arena);

  TEST_ASSERT_EQUAL_size_t(6, result.len);
  TEST_ASSERT_NOT_NULL(result.data);
  TEST_ASSERT_EQUAL_UINT8(0x01, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x02, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x03, result.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0x04, result.data[3]);
  TEST_ASSERT_EQUAL_UINT8(0x05, result.data[4]);
  TEST_ASSERT_EQUAL_UINT8(0x06, result.data[5]);
}

// ===========================================================================
// nibbles_to_bytes tests
// ===========================================================================

void test_nibbles_to_bytes_empty(void) {
  nibbles_t input = NIBBLES_EMPTY;
  uint8_t output[1] = {0xFF};
  nibbles_to_bytes(&input, output);
  // Should not modify output when input is empty
  TEST_ASSERT_EQUAL_UINT8(0xFF, output[0]);
}

void test_nibbles_to_bytes_single(void) {
  // [0x0A, 0x0B] -> 0xAB
  uint8_t nibble_data[] = {0x0A, 0x0B};
  nibbles_t input = {.data = nibble_data, .len = 2};
  uint8_t output[1] = {0};

  nibbles_to_bytes(&input, output);
  TEST_ASSERT_EQUAL_UINT8(0xAB, output[0]);
}

void test_nibbles_to_bytes_multiple(void) {
  // [0x01, 0x02, 0x03, 0x04, 0x05, 0x06] -> 0x12 0x34 0x56
  uint8_t nibble_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  nibbles_t input = {.data = nibble_data, .len = 6};
  uint8_t output[3] = {0};

  nibbles_to_bytes(&input, output);
  TEST_ASSERT_EQUAL_UINT8(0x12, output[0]);
  TEST_ASSERT_EQUAL_UINT8(0x34, output[1]);
  TEST_ASSERT_EQUAL_UINT8(0x56, output[2]);
}

// ===========================================================================
// nibbles_to_bytes_alloc tests
// ===========================================================================

void test_nibbles_to_bytes_alloc_works(void) {
  uint8_t nibble_data[] = {0x0A, 0x0B, 0x0C, 0x0D};
  nibbles_t input = {.data = nibble_data, .len = 4};

  uint8_t *output = nibbles_to_bytes_alloc(&input, &test_arena);
  TEST_ASSERT_NOT_NULL(output);
  TEST_ASSERT_EQUAL_UINT8(0xAB, output[0]);
  TEST_ASSERT_EQUAL_UINT8(0xCD, output[1]);
}

// ===========================================================================
// nibbles_common_prefix tests
// ===========================================================================

void test_nibbles_common_prefix_none(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03};
  uint8_t data_b[] = {0x04, 0x05, 0x06};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};

  size_t common = nibbles_common_prefix(&a, &b);
  TEST_ASSERT_EQUAL_size_t(0, common);
}

void test_nibbles_common_prefix_partial(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03, 0x04};
  uint8_t data_b[] = {0x01, 0x02, 0x05, 0x06};
  nibbles_t a = {.data = data_a, .len = 4};
  nibbles_t b = {.data = data_b, .len = 4};

  size_t common = nibbles_common_prefix(&a, &b);
  TEST_ASSERT_EQUAL_size_t(2, common);
}

void test_nibbles_common_prefix_full(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};

  size_t common = nibbles_common_prefix(&a, &b);
  TEST_ASSERT_EQUAL_size_t(3, common);
}

void test_nibbles_common_prefix_different_lengths(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  nibbles_t a = {.data = data_a, .len = 5};
  nibbles_t b = {.data = data_b, .len = 3};

  size_t common = nibbles_common_prefix(&a, &b);
  TEST_ASSERT_EQUAL_size_t(3, common);
}

// ===========================================================================
// nibbles_slice tests
// ===========================================================================

void test_nibbles_slice_full(void) {
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  nibbles_t src = {.data = data, .len = 4};

  nibbles_t result = nibbles_slice(&src, 0, SIZE_MAX, &test_arena);
  TEST_ASSERT_EQUAL_size_t(4, result.len);
  TEST_ASSERT_EQUAL_UINT8(0x01, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x04, result.data[3]);
}

void test_nibbles_slice_partial(void) {
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  nibbles_t src = {.data = data, .len = 5};

  nibbles_t result = nibbles_slice(&src, 1, 3, &test_arena);
  TEST_ASSERT_EQUAL_size_t(3, result.len);
  TEST_ASSERT_EQUAL_UINT8(0x02, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x03, result.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x04, result.data[2]);
}

void test_nibbles_slice_view(void) {
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  nibbles_t src = {.data = data, .len = 4};

  // When arena is NULL, return a view
  nibbles_t result = nibbles_slice(&src, 1, 2, NULL);
  TEST_ASSERT_EQUAL_size_t(2, result.len);
  TEST_ASSERT_EQUAL_PTR(data + 1, result.data); // Should point to original data
  TEST_ASSERT_EQUAL_UINT8(0x02, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x03, result.data[1]);
}

void test_nibbles_slice_empty(void) {
  uint8_t data[] = {0x01, 0x02, 0x03};
  nibbles_t src = {.data = data, .len = 3};

  nibbles_t result = nibbles_slice(&src, 0, 0, &test_arena);
  TEST_ASSERT_EQUAL_size_t(0, result.len);
}

void test_nibbles_slice_out_of_bounds(void) {
  uint8_t data[] = {0x01, 0x02, 0x03};
  nibbles_t src = {.data = data, .len = 3};

  // Start beyond end
  nibbles_t result = nibbles_slice(&src, 10, 5, &test_arena);
  TEST_ASSERT_EQUAL_size_t(0, result.len);
}

// ===========================================================================
// nibbles_copy tests
// ===========================================================================

void test_nibbles_copy_works(void) {
  uint8_t data[] = {0x01, 0x02, 0x03};
  nibbles_t src = {.data = data, .len = 3};

  nibbles_t copy = nibbles_copy(&src, &test_arena);
  TEST_ASSERT_EQUAL_size_t(3, copy.len);
  TEST_ASSERT_NOT_NULL(copy.data);
  TEST_ASSERT_NOT_EQUAL(src.data, copy.data); // Should be different memory
  TEST_ASSERT_EQUAL_UINT8(0x01, copy.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x02, copy.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x03, copy.data[2]);
}

void test_nibbles_copy_empty(void) {
  nibbles_t src = NIBBLES_EMPTY;
  nibbles_t copy = nibbles_copy(&src, &test_arena);
  TEST_ASSERT_EQUAL_size_t(0, copy.len);
}

// ===========================================================================
// nibbles_cmp tests
// ===========================================================================

void test_nibbles_cmp_equal(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};

  TEST_ASSERT_EQUAL_INT(0, nibbles_cmp(&a, &b));
}

void test_nibbles_cmp_less(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03};
  uint8_t data_b[] = {0x01, 0x02, 0x04};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};

  TEST_ASSERT_LESS_THAN_INT(0, nibbles_cmp(&a, &b));
}

void test_nibbles_cmp_greater(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x04};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};

  TEST_ASSERT_GREATER_THAN_INT(0, nibbles_cmp(&a, &b));
}

void test_nibbles_cmp_prefix(void) {
  uint8_t data_a[] = {0x01, 0x02};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  nibbles_t a = {.data = data_a, .len = 2};
  nibbles_t b = {.data = data_b, .len = 3};

  // Shorter prefix comes first
  TEST_ASSERT_LESS_THAN_INT(0, nibbles_cmp(&a, &b));
  TEST_ASSERT_GREATER_THAN_INT(0, nibbles_cmp(&b, &a));
}

// ===========================================================================
// nibbles_equal tests
// ===========================================================================

void test_nibbles_equal_works(void) {
  uint8_t data_a[] = {0x01, 0x02, 0x03};
  uint8_t data_b[] = {0x01, 0x02, 0x03};
  uint8_t data_c[] = {0x01, 0x02, 0x04};
  nibbles_t a = {.data = data_a, .len = 3};
  nibbles_t b = {.data = data_b, .len = 3};
  nibbles_t c = {.data = data_c, .len = 3};

  TEST_ASSERT_TRUE(nibbles_equal(&a, &b));
  TEST_ASSERT_FALSE(nibbles_equal(&a, &c));
}

// ===========================================================================
// nibbles utility tests
// ===========================================================================

void test_nibbles_is_empty(void) {
  nibbles_t empty = NIBBLES_EMPTY;
  uint8_t data[] = {0x01};
  nibbles_t non_empty = {.data = data, .len = 1};

  TEST_ASSERT_TRUE(nibbles_is_empty(&empty));
  TEST_ASSERT_FALSE(nibbles_is_empty(&non_empty));
}

void test_nibbles_get(void) {
  uint8_t data[] = {0x0A, 0x0B, 0x0C};
  nibbles_t n = {.data = data, .len = 3};

  TEST_ASSERT_EQUAL_UINT8(0x0A, nibbles_get(&n, 0));
  TEST_ASSERT_EQUAL_UINT8(0x0B, nibbles_get(&n, 1));
  TEST_ASSERT_EQUAL_UINT8(0x0C, nibbles_get(&n, 2));
}

// ===========================================================================
// roundtrip tests
// ===========================================================================

void test_nibbles_roundtrip(void) {
  uint8_t original[] = {0xDE, 0xAD, 0xBE, 0xEF};

  // Bytes -> Nibbles
  nibbles_t nibbles = nibbles_from_bytes(original, 4, &test_arena);
  TEST_ASSERT_EQUAL_size_t(8, nibbles.len);

  // Nibbles -> Bytes
  uint8_t result[4];
  nibbles_to_bytes(&nibbles, result);

  TEST_ASSERT_EQUAL_MEMORY(original, result, 4);
}
