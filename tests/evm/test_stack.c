#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include "unity.h"

// External test arena from test_div0.c
extern div0_arena_t test_arena;

void test_stack_init_is_empty(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
  TEST_ASSERT_EQUAL_UINT16(0, evm_stack_size(&stack));
}

void test_stack_push_pop(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  uint256_t value = uint256_from_u64(42);
  TEST_ASSERT_TRUE(evm_stack_push(&stack, value));

  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));

  uint256_t popped = evm_stack_pop_unsafe(&stack);
  TEST_ASSERT_TRUE(uint256_eq(value, popped));
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
}

void test_stack_lifo_order(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(1)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(2)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(3)));

  TEST_ASSERT_EQUAL_UINT64(3, evm_stack_pop_unsafe(&stack).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_pop_unsafe(&stack).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_pop_unsafe(&stack).limbs[0]);
}

void test_stack_peek(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(10)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(20)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(30)));

  // Peek at top (depth 0)
  TEST_ASSERT_EQUAL_UINT64(30, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
  // Peek at second (depth 1)
  TEST_ASSERT_EQUAL_UINT64(20, evm_stack_peek_unsafe(&stack, 1).limbs[0]);
  // Peek at third (depth 2)
  TEST_ASSERT_EQUAL_UINT64(10, evm_stack_peek_unsafe(&stack, 2).limbs[0]);

  // Peeking shouldn't change size
  TEST_ASSERT_EQUAL_UINT16(3, evm_stack_size(&stack));
}

void test_stack_has_space_overflow(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  // Fill to max
  for (int i = 0; i < EVM_STACK_MAX_DEPTH; i++) {
    TEST_ASSERT_TRUE(evm_stack_has_space(&stack, 1));
    TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64((uint64_t)i)));
  }

  // Now should be full
  TEST_ASSERT_FALSE(evm_stack_has_space(&stack, 1));
  TEST_ASSERT_EQUAL_UINT16(EVM_STACK_MAX_DEPTH, evm_stack_size(&stack));
}

void test_stack_has_items(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_FALSE(evm_stack_has_items(&stack, 1));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(1)));
  TEST_ASSERT_TRUE(evm_stack_has_items(&stack, 1));
  TEST_ASSERT_FALSE(evm_stack_has_items(&stack, 2));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(2)));
  TEST_ASSERT_TRUE(evm_stack_has_items(&stack, 2));
}

void test_stack_clear(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(1)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(2)));
  TEST_ASSERT_EQUAL_UINT16(2, evm_stack_size(&stack));

  evm_stack_clear(&stack);
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
}

void test_stack_growth(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  // Initial capacity is EVM_STACK_INITIAL_CAPACITY (32)
  TEST_ASSERT_EQUAL_UINT16(EVM_STACK_INITIAL_CAPACITY, stack.capacity);

  // Push more than initial capacity to trigger growth
  for (int i = 0; i < EVM_STACK_INITIAL_CAPACITY + 10; i++) {
    TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64((uint64_t)i)));
  }

  // Capacity should have grown
  TEST_ASSERT_TRUE(stack.capacity > EVM_STACK_INITIAL_CAPACITY);

  // Verify values are correct
  for (int i = EVM_STACK_INITIAL_CAPACITY + 9; i >= 0; i--) {
    TEST_ASSERT_EQUAL_UINT64((uint64_t)i, evm_stack_pop_unsafe(&stack).limbs[0]);
  }
}

void test_stack_dup(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(100)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(200)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(300)));

  // DUP1 (depth=1) duplicates top
  evm_stack_dup_unsafe(&stack, 1);
  TEST_ASSERT_EQUAL_UINT16(4, evm_stack_size(&stack));
  TEST_ASSERT_EQUAL_UINT64(300, evm_stack_peek_unsafe(&stack, 0).limbs[0]);

  // DUP3 (depth=3) duplicates third from new top
  evm_stack_dup_unsafe(&stack, 3);
  TEST_ASSERT_EQUAL_UINT16(5, evm_stack_size(&stack));
  TEST_ASSERT_EQUAL_UINT64(200, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
}

void test_stack_swap(void) {
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(100)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(200)));
  TEST_ASSERT_TRUE(evm_stack_push(&stack, uint256_from_u64(300)));

  // SWAP1 (depth=1) swaps top two
  evm_stack_swap_unsafe(&stack, 1);
  TEST_ASSERT_EQUAL_UINT64(200, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(300, evm_stack_peek_unsafe(&stack, 1).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(100, evm_stack_peek_unsafe(&stack, 2).limbs[0]);

  // SWAP2 (depth=2) swaps top and third
  evm_stack_swap_unsafe(&stack, 2);
  TEST_ASSERT_EQUAL_UINT64(100, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(300, evm_stack_peek_unsafe(&stack, 1).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(200, evm_stack_peek_unsafe(&stack, 2).limbs[0]);
}