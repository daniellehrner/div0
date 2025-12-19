#include "div0/div0.h"

#include "unity.h"

#include <string.h>

void setUp(void) {
  // Called before each test
}

void tearDown(void) {
  // Called after each test
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

// =============================================================================
// stack tests
// =============================================================================

void test_stack_init_is_empty(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
  TEST_ASSERT_EQUAL_UINT16(0, evm_stack_size(&stack));
}

void test_stack_push_pop(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);

  uint256_t value = uint256_from_u64(42);
  evm_stack_push_unsafe(&stack, value);

  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(&stack));

  uint256_t popped = evm_stack_pop_unsafe(&stack);
  TEST_ASSERT_TRUE(uint256_eq(value, popped));
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
}

void test_stack_lifo_order(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);

  evm_stack_push_unsafe(&stack, uint256_from_u64(1));
  evm_stack_push_unsafe(&stack, uint256_from_u64(2));
  evm_stack_push_unsafe(&stack, uint256_from_u64(3));

  TEST_ASSERT_EQUAL_UINT64(3, evm_stack_pop_unsafe(&stack).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_pop_unsafe(&stack).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_pop_unsafe(&stack).limbs[0]);
}

void test_stack_peek(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);

  evm_stack_push_unsafe(&stack, uint256_from_u64(10));
  evm_stack_push_unsafe(&stack, uint256_from_u64(20));
  evm_stack_push_unsafe(&stack, uint256_from_u64(30));

  // Peek at top (depth 0)
  TEST_ASSERT_EQUAL_UINT64(30, evm_stack_peek_unsafe(&stack, 0).limbs[0]);
  // Peek at second (depth 1)
  TEST_ASSERT_EQUAL_UINT64(20, evm_stack_peek_unsafe(&stack, 1).limbs[0]);
  // Peek at third (depth 2)
  TEST_ASSERT_EQUAL_UINT64(10, evm_stack_peek_unsafe(&stack, 2).limbs[0]);

  // Peeking shouldn't change size
  TEST_ASSERT_EQUAL_UINT16(3, evm_stack_size(&stack));
}

void test_stack_can_push_overflow(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);

  // Fill to max
  for (int i = 0; i < EVM_STACK_MAX_DEPTH; i++) {
    TEST_ASSERT_TRUE(evm_stack_can_push(&stack));
    evm_stack_push_unsafe(&stack, uint256_from_u64((uint64_t)i));
  }

  // Now should be full
  TEST_ASSERT_FALSE(evm_stack_can_push(&stack));
  TEST_ASSERT_EQUAL_UINT16(EVM_STACK_MAX_DEPTH, evm_stack_size(&stack));
}

void test_stack_clear(void) {
  evm_stack_t stack;
  evm_stack_init(&stack);

  evm_stack_push_unsafe(&stack, uint256_from_u64(1));
  evm_stack_push_unsafe(&stack, uint256_from_u64(2));
  TEST_ASSERT_EQUAL_UINT16(2, evm_stack_size(&stack));

  evm_stack_clear(&stack);
  TEST_ASSERT_TRUE(evm_stack_is_empty(&stack));
}

// =============================================================================
// evm tests
// =============================================================================

void test_evm_stop(void) {
  // Just STOP
  uint8_t code[] = {OP_STOP};
  evm_stack_t stack;
  evm_stack_init(&stack);

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
}

void test_evm_empty_code(void) {
  // Empty code should also return STOP
  evm_stack_t stack;
  evm_stack_init(&stack);

  evm_context_t ctx;
  evm_context_init(&ctx, NULL, 0, &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result);
}

void test_evm_push1(void) {
  // PUSH1 0x42, STOP
  uint8_t code[] = {OP_PUSH1, 0x42, OP_STOP};
  evm_stack_t stack;
  evm_stack_init(&stack);

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
  evm_stack_init(&stack);

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
  evm_stack_init(&stack);

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
  evm_stack_init(&stack);

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
  evm_stack_init(&stack);

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
  evm_stack_init(&stack);

  evm_context_t ctx;
  evm_context_init(&ctx, code, sizeof(code), &stack);

  evm_result_t result = evm_execute(&ctx);
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, ctx.status);
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

  // stack tests
  RUN_TEST(test_stack_init_is_empty);
  RUN_TEST(test_stack_push_pop);
  RUN_TEST(test_stack_lifo_order);
  RUN_TEST(test_stack_peek);
  RUN_TEST(test_stack_can_push_overflow);
  RUN_TEST(test_stack_clear);

  // evm tests
  RUN_TEST(test_evm_stop);
  RUN_TEST(test_evm_empty_code);
  RUN_TEST(test_evm_push1);
  RUN_TEST(test_evm_push32);
  RUN_TEST(test_evm_add);
  RUN_TEST(test_evm_add_multiple);
  RUN_TEST(test_evm_invalid_opcode);
  RUN_TEST(test_evm_stack_underflow);

  return UNITY_END();
}