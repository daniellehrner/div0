#include "div0/mem/arena.h"

#include "unity.h"

#include <string.h>

// External test arena from test_div0.c
extern div0_arena_t test_arena;

void test_arena_alloc_basic(void) {
  void *ptr = div0_arena_alloc(&test_arena, 100);
  TEST_ASSERT_NOT_NULL(ptr);
}

void test_arena_alloc_aligned(void) {
  void *ptr1 = div0_arena_alloc(&test_arena, 1);
  void *ptr2 = div0_arena_alloc(&test_arena, 1);
  // Both should be 8-byte aligned
  TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)ptr1 % 8);
  TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)ptr2 % 8);
}

void test_arena_realloc(void) {
  void *ptr1 = div0_arena_alloc(&test_arena, 32);
  TEST_ASSERT_NOT_NULL(ptr1);
  memset(ptr1, 0xAB, 32);

  void *ptr2 = div0_arena_realloc(&test_arena, ptr1, 32, 64);
  TEST_ASSERT_NOT_NULL(ptr2);

  // Old data should be copied
  uint8_t *bytes = (uint8_t *)ptr2;
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(0xAB, bytes[i]);
  }
}

void test_arena_reset(void) {
  void *ptr1 = div0_arena_alloc(&test_arena, 1000);
  TEST_ASSERT_NOT_NULL(ptr1);

  div0_arena_reset(&test_arena);

  // After reset, should be able to allocate from the beginning again
  void *ptr2 = div0_arena_alloc(&test_arena, 1000);
  TEST_ASSERT_NOT_NULL(ptr2);
  // Pointers should be the same (or very close) since we reset
  TEST_ASSERT_EQUAL_PTR(ptr1, ptr2);
}

void test_arena_alloc_large(void) {
  // Allocate larger than DIV0_ARENA_BLOCK_SIZE (64KB)
  size_t large_size = 128 * 1024; // 128KB
  void *ptr = div0_arena_alloc_large(&test_arena, large_size, 8);
  TEST_ASSERT_NOT_NULL(ptr);

  // Should be 8-byte aligned
  TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)ptr % 8);

  // Should be writable
  memset(ptr, 0xAB, large_size);
  uint8_t *bytes = (uint8_t *)ptr;
  TEST_ASSERT_EQUAL_UINT8(0xAB, bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, bytes[large_size - 1]);
}

void test_arena_alloc_large_alignment(void) {
  // Allocate with 64-byte alignment (cache line)
  size_t large_size = 100 * 1024; // 100KB
  void *ptr = div0_arena_alloc_large(&test_arena, large_size, 64);
  TEST_ASSERT_NOT_NULL(ptr);

  // Should be 64-byte aligned
  TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)ptr % 64);
}

void test_arena_alloc_large_freed_on_reset(void) {
  // Create a separate arena for this test to avoid affecting others
  div0_arena_t local_arena;
  TEST_ASSERT_TRUE(div0_arena_init(&local_arena));

  // Allocate a large block
  size_t large_size = 128 * 1024;
  void *ptr1 = div0_arena_alloc_large(&local_arena, large_size, 8);
  TEST_ASSERT_NOT_NULL(ptr1);

  // Large blocks should be in the large_blocks chain
  TEST_ASSERT_NOT_NULL(local_arena.large_blocks);

  // Reset should free large blocks
  div0_arena_reset(&local_arena);
  TEST_ASSERT_NULL(local_arena.large_blocks);

  // Regular blocks should still be there
  TEST_ASSERT_NOT_NULL(local_arena.head);

  div0_arena_destroy(&local_arena);
}

void test_arena_alloc_large_multiple(void) {
  // Create a separate arena
  div0_arena_t local_arena;
  TEST_ASSERT_TRUE(div0_arena_init(&local_arena));

  // Allocate multiple large blocks
  void *ptr1 = div0_arena_alloc_large(&local_arena, 100 * 1024, 8);
  void *ptr2 = div0_arena_alloc_large(&local_arena, 200 * 1024, 8);
  void *ptr3 = div0_arena_alloc_large(&local_arena, 150 * 1024, 8);

  TEST_ASSERT_NOT_NULL(ptr1);
  TEST_ASSERT_NOT_NULL(ptr2);
  TEST_ASSERT_NOT_NULL(ptr3);

  // All should be different addresses
  TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
  TEST_ASSERT_NOT_EQUAL(ptr2, ptr3);
  TEST_ASSERT_NOT_EQUAL(ptr1, ptr3);

  div0_arena_destroy(&local_arena);
}