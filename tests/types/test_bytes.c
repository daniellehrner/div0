#include "test_bytes.h"

#include "div0/types/bytes.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

void test_bytes_init_empty(void) {
  bytes_t b;
  bytes_init(&b);

  TEST_ASSERT_NULL(b.data);
  TEST_ASSERT_EQUAL_size_t(0, b.size);
  TEST_ASSERT_EQUAL_size_t(0, b.capacity);
  TEST_ASSERT_NULL(b.arena);
  TEST_ASSERT_TRUE(bytes_is_empty(&b));

  bytes_free(&b);
}

void test_bytes_from_data_works(void) {
  bytes_t b;
  bytes_init(&b);

  uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
  TEST_ASSERT_TRUE(bytes_from_data(&b, data, 4));

  TEST_ASSERT_NOT_NULL(b.data);
  TEST_ASSERT_EQUAL_size_t(4, b.size);
  TEST_ASSERT_TRUE(b.capacity >= 4);
  TEST_ASSERT_EQUAL_MEMORY(data, b.data, 4);

  bytes_free(&b);
}

void test_bytes_append_works(void) {
  bytes_t b;
  bytes_init(&b);

  uint8_t data1[] = {0x01, 0x02};
  uint8_t data2[] = {0x03, 0x04, 0x05};

  TEST_ASSERT_TRUE(bytes_from_data(&b, data1, 2));
  TEST_ASSERT_TRUE(bytes_append(&b, data2, 3));

  TEST_ASSERT_EQUAL_size_t(5, b.size);
  TEST_ASSERT_EQUAL_UINT8(0x01, b.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x02, b.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x03, b.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0x04, b.data[3]);
  TEST_ASSERT_EQUAL_UINT8(0x05, b.data[4]);

  bytes_free(&b);
}

void test_bytes_append_byte_works(void) {
  bytes_t b;
  bytes_init(&b);

  TEST_ASSERT_TRUE(bytes_append_byte(&b, 0xAA));
  TEST_ASSERT_TRUE(bytes_append_byte(&b, 0xBB));
  TEST_ASSERT_TRUE(bytes_append_byte(&b, 0xCC));

  TEST_ASSERT_EQUAL_size_t(3, b.size);
  TEST_ASSERT_EQUAL_UINT8(0xAA, b.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0xBB, b.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0xCC, b.data[2]);

  bytes_free(&b);
}

void test_bytes_clear_works(void) {
  bytes_t b;
  bytes_init(&b);

  uint8_t data[] = {0x01, 0x02, 0x03};
  TEST_ASSERT_TRUE(bytes_from_data(&b, data, 3));

  size_t old_capacity = b.capacity;
  bytes_clear(&b);

  TEST_ASSERT_EQUAL_size_t(0, b.size);
  TEST_ASSERT_EQUAL_size_t(old_capacity, b.capacity); // Capacity preserved
  TEST_ASSERT_TRUE(bytes_is_empty(&b));

  bytes_free(&b);
}

void test_bytes_free_works(void) {
  bytes_t b;
  bytes_init(&b);

  uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  TEST_ASSERT_TRUE(bytes_from_data(&b, data, 4));
  TEST_ASSERT_NOT_NULL(b.data);

  bytes_free(&b);

  TEST_ASSERT_NULL(b.data);
  TEST_ASSERT_EQUAL_size_t(0, b.size);
  TEST_ASSERT_EQUAL_size_t(0, b.capacity);
}

void test_bytes_arena_backed(void) {
  bytes_t b;
  bytes_init_arena(&b, &test_arena);

  TEST_ASSERT_EQUAL_PTR(&test_arena, b.arena);

  // Reserve capacity from arena
  TEST_ASSERT_TRUE(bytes_reserve(&b, 64));
  TEST_ASSERT_NOT_NULL(b.data);
  TEST_ASSERT_EQUAL_size_t(64, b.capacity);

  // Add some data
  uint8_t data[] = {0xAA, 0xBB, 0xCC};
  TEST_ASSERT_TRUE(bytes_from_data(&b, data, 3));
  TEST_ASSERT_EQUAL_size_t(3, b.size);
  TEST_ASSERT_EQUAL_MEMORY(data, b.data, 3);

  // Free is a no-op for arena-backed
  bytes_free(&b);
  TEST_ASSERT_NULL(b.data);
  TEST_ASSERT_EQUAL_size_t(0, b.size);
}

void test_bytes_arena_no_realloc(void) {
  bytes_t b;
  bytes_init_arena(&b, &test_arena);

  // Reserve initial capacity
  TEST_ASSERT_TRUE(bytes_reserve(&b, 8));
  TEST_ASSERT_EQUAL_size_t(8, b.capacity);

  // Try to reserve more - should fail (arena can't realloc)
  TEST_ASSERT_FALSE(bytes_reserve(&b, 16));
  TEST_ASSERT_EQUAL_size_t(8, b.capacity); // Unchanged

  // Can still use the original capacity
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  TEST_ASSERT_TRUE(bytes_from_data(&b, data, 8));
  TEST_ASSERT_EQUAL_size_t(8, b.size);

  // But can't exceed capacity
  TEST_ASSERT_FALSE(bytes_append_byte(&b, 0x09));
  TEST_ASSERT_EQUAL_size_t(8, b.size); // Unchanged

  bytes_free(&b);
}