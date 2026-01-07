#include "test_opcodes_comparison.h"

#include "div0/evm/evm.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include "unity.h"

#include <string.h>

// External test arena from test_div0.c
extern div0_arena_t test_arena;

/// Helper to create a minimal execution environment for testing.
static execution_env_t make_test_env(const uint8_t *code, size_t code_size, uint64_t gas) {
  execution_env_t env;
  memset(&env, 0, sizeof(env));
  env.call.code = code;
  env.call.code_size = code_size;
  env.call.gas = gas;
  return env;
}

// =============================================================================
// LT (Less Than) Opcode Tests
// =============================================================================

void test_opcode_lt_true_when_less(void) {
  // PUSH1 10, PUSH1 5, LT => 5 < 10 = 1
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 5, OP_LT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_lt_false_when_greater(void) {
  // PUSH1 5, PUSH1 10, LT => 10 < 5 = 0
  uint8_t code[] = {OP_PUSH1, 5, OP_PUSH1, 10, OP_LT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_lt_false_when_equal(void) {
  // PUSH1 42, PUSH1 42, LT => 42 < 42 = 0
  uint8_t code[] = {OP_PUSH1, 42, OP_PUSH1, 42, OP_LT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_lt_stack_underflow(void) {
  // LT with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_LT};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// GT (Greater Than) Opcode Tests
// =============================================================================

void test_opcode_gt_true_when_greater(void) {
  // PUSH1 5, PUSH1 10, GT => 10 > 5 = 1
  uint8_t code[] = {OP_PUSH1, 5, OP_PUSH1, 10, OP_GT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_gt_false_when_less(void) {
  // PUSH1 10, PUSH1 5, GT => 5 > 10 = 0
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 5, OP_GT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_gt_false_when_equal(void) {
  // PUSH1 42, PUSH1 42, GT => 42 > 42 = 0
  uint8_t code[] = {OP_PUSH1, 42, OP_PUSH1, 42, OP_GT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_gt_stack_underflow(void) {
  // GT with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_GT};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// EQ (Equality) Opcode Tests
// =============================================================================

void test_opcode_eq_true_when_equal(void) {
  // PUSH1 42, PUSH1 42, EQ => 42 == 42 = 1
  uint8_t code[] = {OP_PUSH1, 42, OP_PUSH1, 42, OP_EQ, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_eq_false_when_not_equal(void) {
  // PUSH1 100, PUSH1 42, EQ => 42 == 100 = 0
  uint8_t code[] = {OP_PUSH1, 100, OP_PUSH1, 42, OP_EQ, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_eq_zero_equals_zero(void) {
  // PUSH1 0, PUSH1 0, EQ => 0 == 0 = 1
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_EQ, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_eq_stack_underflow(void) {
  // EQ with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_EQ};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// ISZERO Opcode Tests
// =============================================================================

void test_opcode_iszero_true_when_zero(void) {
  // PUSH1 0, ISZERO => ISZERO(0) = 1
  uint8_t code[] = {OP_PUSH1, 0, OP_ISZERO, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_iszero_false_when_nonzero(void) {
  // PUSH1 42, ISZERO => ISZERO(42) = 0
  uint8_t code[] = {OP_PUSH1, 42, OP_ISZERO, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_iszero_false_when_one(void) {
  // PUSH1 1, ISZERO => ISZERO(1) = 0
  uint8_t code[] = {OP_PUSH1, 1, OP_ISZERO, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_iszero_stack_underflow(void) {
  // ISZERO with empty stack
  uint8_t code[] = {OP_ISZERO};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// SLT (Signed Less Than) Opcode Tests
// =============================================================================

void test_opcode_slt_positive_less_than_positive(void) {
  // PUSH1 10, PUSH1 5, SLT => 5 <_s 10 = 1
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 5, OP_SLT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_slt_negative_less_than_positive(void) {
  // PUSH1 1, PUSH32 -1 (MAX), SLT => -1 <_s 1 = 1
  // -1 = 0xFFFF...FF (all bits set)
  uint8_t code[] = {OP_PUSH1,  1, // push 1
                    OP_PUSH32, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // push -1
                    OP_SLT,    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_slt_positive_not_less_than_negative(void) {
  // PUSH32 -1 (MAX), PUSH1 1, SLT => 1 <_s -1 = 0
  uint8_t code[] = {OP_PUSH32, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // push -1
                    OP_PUSH1,  1,                                                // push 1
                    OP_SLT,    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_slt_stack_underflow(void) {
  // SLT with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_SLT};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// SGT (Signed Greater Than) Opcode Tests
// =============================================================================

void test_opcode_sgt_positive_greater_than_negative(void) {
  // PUSH32 -1 (MAX), PUSH1 1, SGT => 1 >_s -1 = 1
  uint8_t code[] = {OP_PUSH32, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // push -1
                    OP_PUSH1,  1,                                                // push 1
                    OP_SGT,    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_sgt_negative_not_greater_than_positive(void) {
  // PUSH1 1, PUSH32 -1 (MAX), SGT => -1 >_s 1 = 0
  uint8_t code[] = {OP_PUSH1,  1, // push 1
                    OP_PUSH32, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // push -1
                    OP_SGT,    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_sgt_larger_positive_greater(void) {
  // PUSH1 5, PUSH1 10, SGT => 10 >_s 5 = 1
  uint8_t code[] = {OP_PUSH1, 5, OP_PUSH1, 10, OP_SGT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_sgt_stack_underflow(void) {
  // SGT with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_SGT};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// Gas Tests
// =============================================================================

void test_opcode_comparison_out_of_gas(void) {
  // LT with insufficient gas (gas cost is 3)
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 5, OP_LT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // 3 for each PUSH1, only 2 remaining for LT (needs 3)
  execution_env_t env = make_test_env(code, sizeof(code), 8);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}
