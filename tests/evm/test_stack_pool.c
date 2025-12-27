#include "div0/evm/stack_pool.h"
#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include "unity.h"

// External test arena from test_div0.c
extern div0_arena_t test_arena;

void test_stack_pool_init(void) {
  evm_stack_pool_t pool;
  evm_stack_pool_init(&pool, &test_arena);
  TEST_ASSERT_EQUAL_PTR(&test_arena, pool.arena);
}

void test_stack_pool_borrow(void) {
  evm_stack_pool_t pool;
  evm_stack_pool_init(&pool, &test_arena);

  evm_stack_t *stack = evm_stack_pool_borrow(&pool);
  TEST_ASSERT_NOT_NULL(stack);
  TEST_ASSERT_TRUE(evm_stack_is_empty(stack));

  // Use the borrowed stack
  TEST_ASSERT_TRUE(evm_stack_push(stack, uint256_from_u64(42)));
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(stack));

  evm_stack_pool_return(&pool, stack);
}

void test_stack_pool_multiple_borrows(void) {
  evm_stack_pool_t pool;
  evm_stack_pool_init(&pool, &test_arena);

  // Borrow multiple stacks
  evm_stack_t *s1 = evm_stack_pool_borrow(&pool);
  evm_stack_t *s2 = evm_stack_pool_borrow(&pool);
  evm_stack_t *s3 = evm_stack_pool_borrow(&pool);

  TEST_ASSERT_NOT_NULL(s1);
  TEST_ASSERT_NOT_NULL(s2);
  TEST_ASSERT_NOT_NULL(s3);

  // Each stack should be independent
  TEST_ASSERT_NOT_EQUAL(s1, s2);
  TEST_ASSERT_NOT_EQUAL(s2, s3);

  // Use them independently
  TEST_ASSERT_TRUE(evm_stack_push(s1, uint256_from_u64(1)));
  TEST_ASSERT_TRUE(evm_stack_push(s2, uint256_from_u64(2)));
  TEST_ASSERT_TRUE(evm_stack_push(s3, uint256_from_u64(3)));

  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(s1, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(s2, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(3, evm_stack_peek_unsafe(s3, 0).limbs[0]);

  evm_stack_pool_return(&pool, s1);
  evm_stack_pool_return(&pool, s2);
  evm_stack_pool_return(&pool, s3);
}