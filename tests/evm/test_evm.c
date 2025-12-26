#include "div0/evm/evm.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include "unity.h"

// External test arena from test_div0.c
extern div0_arena_t test_arena;

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