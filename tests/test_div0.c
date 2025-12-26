#include "div0/div0.h"
#include "div0/evm/stack_pool.h"

#include "unity.h"

#include <string.h>

// Global arena for tests
static div0_arena_t test_arena;
static bool arena_initialized = false;

void setUp(void) {
  // Initialize arena once, reset between tests
  if (!arena_initialized) {
    TEST_ASSERT_TRUE(div0_arena_init(&test_arena));
    arena_initialized = true;
  }
}

void tearDown(void) {
  // Reset arena after each test (keeps blocks for reuse)
  div0_arena_reset(&test_arena);
}

// =============================================================================
// uint256 tests
// =============================================================================

void test_uint256_zero_is_zero(void) {
  uint256_t z = uint256_zero();
  TEST_ASSERT_TRUE(uint256_is_zero(z));
}

void test_uint256_from_u64_works(void) {
  uint256_t a = uint256_from_u64(42);
  TEST_ASSERT_FALSE(uint256_is_zero(a));
  TEST_ASSERT_EQUAL_UINT64(42, a.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[3]);
}

void test_uint256_eq_works(void) {
  uint256_t a = uint256_from_u64(123);
  uint256_t b = uint256_from_u64(123);
  uint256_t c = uint256_from_u64(456);
  TEST_ASSERT_TRUE(uint256_eq(a, b));
  TEST_ASSERT_FALSE(uint256_eq(a, c));
}

void test_uint256_add_no_overflow(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(200);
  uint256_t result = uint256_add(a, b);
  TEST_ASSERT_EQUAL_UINT64(300, result.limbs[0]);
}

void test_uint256_add_with_carry(void) {
  // Test carry propagation: (2^64 - 1) + 1 = 2^64
  uint256_t a = uint256_zero();
  a.limbs[0] = UINT64_MAX;
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_add(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
}

void test_uint256_add_overflow_wraps(void) {
  // Max uint256 + 1 should wrap to 0
  uint256_t max;
  max.limbs[0] = UINT64_MAX;
  max.limbs[1] = UINT64_MAX;
  max.limbs[2] = UINT64_MAX;
  max.limbs[3] = UINT64_MAX;
  uint256_t one = uint256_from_u64(1);
  uint256_t result = uint256_add(max, one);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_bytes_be_roundtrip(void) {
  // Create a value with bytes in all limbs
  uint256_t original = uint256_zero();
  original.limbs[0] = 0x0807060504030201ULL;
  original.limbs[1] = 0x100F0E0D0C0B0A09ULL;
  original.limbs[2] = 0x1817161514131211ULL;
  original.limbs[3] = 0x201F1E1D1C1B1A19ULL;

  uint8_t bytes[32];
  uint256_to_bytes_be(original, bytes);

  uint256_t restored = uint256_from_bytes_be(bytes, 32);
  TEST_ASSERT_TRUE(uint256_eq(original, restored));
}

void test_uint256_from_bytes_be_short(void) {
  // 2 bytes: 0x0102 = 258
  uint8_t bytes[] = {0x01, 0x02};
  uint256_t value = uint256_from_bytes_be(bytes, 2);
  TEST_ASSERT_EQUAL_UINT64(258, value.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, value.limbs[1]);
}

void test_uint256_sub_basic(void) {
  uint256_t a = uint256_from_u64(200);
  uint256_t b = uint256_from_u64(100);
  uint256_t result = uint256_sub(a, b);
  TEST_ASSERT_EQUAL_UINT64(100, result.limbs[0]);
}

void test_uint256_sub_with_borrow(void) {
  // 2^64 - 1 = result with borrow from second limb
  uint256_t a = uint256_from_limbs(0, 1, 0, 0); // 2^64
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_sub(a, b);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_sub_underflow_wraps(void) {
  // 0 - 1 should wrap to MAX_UINT256
  uint256_t zero = uint256_zero();
  uint256_t one = uint256_from_u64(1);
  uint256_t result = uint256_sub(zero, one);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_lt_basic(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(200);
  TEST_ASSERT_TRUE(uint256_lt(a, b));
  TEST_ASSERT_FALSE(uint256_lt(b, a));
}

void test_uint256_lt_equal(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(100);
  TEST_ASSERT_FALSE(uint256_lt(a, b));
}

void test_uint256_lt_multi_limb(void) {
  // a = 2^64 (only second limb set)
  uint256_t a = uint256_from_limbs(0, 1, 0, 0);
  // b = MAX_UINT64 (only first limb set)
  uint256_t b = uint256_from_limbs(UINT64_MAX, 0, 0, 0);
  TEST_ASSERT_FALSE(uint256_lt(a, b)); // a > b
  TEST_ASSERT_TRUE(uint256_lt(b, a));  // b < a
}

void test_uint256_mul_basic(void) {
  uint256_t a = uint256_from_u64(6);
  uint256_t b = uint256_from_u64(7);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(42, result.limbs[0]);
}

void test_uint256_mul_limb_boundary(void) {
  // 2^32 * 2^32 = 2^64
  uint256_t a = uint256_from_u64(1ULL << 32);
  uint256_t b = uint256_from_u64(1ULL << 32);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
}

void test_uint256_mul_multi_limb(void) {
  // (2^64) * (2^64) = 2^128
  uint256_t a = uint256_from_limbs(0, 1, 0, 0);
  uint256_t b = uint256_from_limbs(0, 1, 0, 0);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[3]);
}

void test_uint256_mul_overflow_wraps(void) {
  // MAX * 2 should wrap
  uint256_t max = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t two = uint256_from_u64(2);
  uint256_t result = uint256_mul(max, two);
  // MAX * 2 = 2 * (2^256 - 1) = 2^257 - 2 ≡ -2 (mod 2^256) = MAX - 1
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX - 1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_div_basic(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(10, result.limbs[0]);
}

void test_uint256_div_by_zero(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_zero();
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_smaller_than_divisor(void) {
  uint256_t a = uint256_from_u64(5);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_with_remainder(void) {
  uint256_t a = uint256_from_u64(17);
  uint256_t b = uint256_from_u64(5);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(3, result.limbs[0]);
}

void test_uint256_div_multi_limb(void) {
  // 2^128 / 2^64 = 2^64
  uint256_t a = uint256_from_limbs(0, 0, 1, 0); // 2^128
  uint256_t b = uint256_from_limbs(0, 1, 0, 0); // 2^64
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[3]);
}

void test_uint256_mod_basic(void) {
  uint256_t a = uint256_from_u64(17);
  uint256_t b = uint256_from_u64(5);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_EQUAL_UINT64(2, result.limbs[0]);
}

void test_uint256_mod_by_zero(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_zero();
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_mod_by_one(void) {
  uint256_t a = uint256_from_u64(12345);
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_mod_no_remainder(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_mod_consistency(void) {
  // Verify: a == (a / b) * b + (a % b)
  uint256_t a = uint256_from_u64(12345);
  uint256_t b = uint256_from_u64(67);
  uint256_t q = uint256_div(a, b);
  uint256_t r = uint256_mod(a, b);
  uint256_t reconstructed = uint256_add(uint256_mul(q, b), r);
  TEST_ASSERT_TRUE(uint256_eq(a, reconstructed));
}

// =============================================================================
// arena tests
// =============================================================================

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

// =============================================================================
// stack tests
// =============================================================================

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

// =============================================================================
// evm tests
// =============================================================================

void test_evm_stop(void) {
  // Just STOP
  uint8_t code[] = {OP_STOP};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
}

void test_evm_empty_code(void) {
  // Empty code should also return STOP
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, nullptr, 0, &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
}

void test_evm_push1(void) {
  // PUSH1 0x42, STOP
  uint8_t code[] = {OP_PUSH1, 0x42, OP_STOP};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
}

void test_evm_push32(void) {
  // PUSH32 <32 bytes>, STOP
  uint8_t code[34];
  code[0] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[1 + i] = (uint8_t)i;
  }
  code[33] = OP_STOP;

  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));

  // Verify the value (big-endian input)
  uint8_t output[32];
  uint256_to_bytes_be(evm_stack_peek_unsafe(&stack, 0), output);
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(i, output[i]);
  }
}

void test_evm_add(void) {
  // PUSH1 10, PUSH1 20, ADD, STOP
  // Stack after: [30]
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 20, OP_ADD, OP_STOP};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));
  TEST_ASSERT_EQUAL_UINT64(30, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
}

void test_evm_add_multiple(void) {
  // PUSH1 1, PUSH1 2, PUSH1 3, ADD, ADD, STOP
  // Stack: [1] -> [1,2] -> [1,2,3] -> [1,5] -> [6]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_ADD, OP_ADD, OP_STOP};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));
  TEST_ASSERT_EQUAL_UINT64(6, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
}

void test_evm_invalid_opcode(void) {
  // Use an unimplemented opcode (e.g., MUL = 0x02)
  uint8_t code[] = {OP_MUL};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, ctx.status);
}

void test_evm_stack_underflow(void) {
  // ADD with empty stack
  uint8_t code[] = {OP_ADD};
  evm_stack_t stack;
  TEST_ASSERT_TRUE(evm_stack_init(&stack, &test_arena));

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, ctx.status);
}

// =============================================================================
// stack pool tests
// =============================================================================

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

// =============================================================================
// Main
// =============================================================================

int main(void) {
  UNITY_BEGIN();

  // uint256 tests
  RUN_TEST(test_uint256_zero_is_zero);
  RUN_TEST(test_uint256_from_u64_works);
  RUN_TEST(test_uint256_eq_works);
  RUN_TEST(test_uint256_add_no_overflow);
  RUN_TEST(test_uint256_add_with_carry);
  RUN_TEST(test_uint256_add_overflow_wraps);
  RUN_TEST(test_uint256_bytes_be_roundtrip);
  RUN_TEST(test_uint256_from_bytes_be_short);
  RUN_TEST(test_uint256_sub_basic);
  RUN_TEST(test_uint256_sub_with_borrow);
  RUN_TEST(test_uint256_sub_underflow_wraps);
  RUN_TEST(test_uint256_lt_basic);
  RUN_TEST(test_uint256_lt_equal);
  RUN_TEST(test_uint256_lt_multi_limb);
  RUN_TEST(test_uint256_mul_basic);
  RUN_TEST(test_uint256_mul_limb_boundary);
  RUN_TEST(test_uint256_mul_multi_limb);
  RUN_TEST(test_uint256_mul_overflow_wraps);
  RUN_TEST(test_uint256_div_basic);
  RUN_TEST(test_uint256_div_by_zero);
  RUN_TEST(test_uint256_div_smaller_than_divisor);
  RUN_TEST(test_uint256_div_with_remainder);
  RUN_TEST(test_uint256_div_multi_limb);
  RUN_TEST(test_uint256_mod_basic);
  RUN_TEST(test_uint256_mod_by_zero);
  RUN_TEST(test_uint256_mod_by_one);
  RUN_TEST(test_uint256_mod_no_remainder);
  RUN_TEST(test_uint256_div_mod_consistency);

  // arena tests
  RUN_TEST(test_arena_alloc_basic);
  RUN_TEST(test_arena_alloc_aligned);
  RUN_TEST(test_arena_realloc);
  RUN_TEST(test_arena_reset);

  // stack tests
  RUN_TEST(test_stack_init_is_empty);
  RUN_TEST(test_stack_push_pop);
  RUN_TEST(test_stack_lifo_order);
  RUN_TEST(test_stack_peek);
  RUN_TEST(test_stack_has_space_overflow);
  RUN_TEST(test_stack_has_items);
  RUN_TEST(test_stack_clear);
  RUN_TEST(test_stack_growth);
  RUN_TEST(test_stack_dup);
  RUN_TEST(test_stack_swap);

  // evm tests
  RUN_TEST(test_evm_stop);
  RUN_TEST(test_evm_empty_code);
  RUN_TEST(test_evm_push1);
  RUN_TEST(test_evm_push32);
  RUN_TEST(test_evm_add);
  RUN_TEST(test_evm_add_multiple);
  RUN_TEST(test_evm_invalid_opcode);
  RUN_TEST(test_evm_stack_underflow);

  // stack pool tests
  RUN_TEST(test_stack_pool_init);
  RUN_TEST(test_stack_pool_borrow);
  RUN_TEST(test_stack_pool_multiple_borrows);

  // Cleanup
  div0_arena_destroy(&test_arena);

  return UNITY_END();
}