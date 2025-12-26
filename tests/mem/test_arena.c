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