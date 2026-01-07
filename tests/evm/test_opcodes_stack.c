#include "test_opcodes_stack.h"

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
// POP Opcode Tests
// =============================================================================

void test_opcode_pop_basic(void) {
  // PUSH1 5, POP => empty stack
  uint8_t code[] = {OP_PUSH1, 5, OP_POP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(0, evm_stack_size(evm.current_frame->stack));
}

void test_opcode_pop_multiple(void) {
  // PUSH1 1, PUSH1 2, PUSH1 3, POP, POP => [1]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_POP, OP_POP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_pop_stack_underflow(void) {
  // POP on empty stack
  uint8_t code[] = {OP_POP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_pop_gas_consumption(void) {
  // PUSH1 5, POP - check gas consumed (PUSH1=3, POP=2)
  uint8_t code[] = {OP_PUSH1, 5, OP_POP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  uint64_t initial_gas = 100;
  execution_env_t env = make_test_env(code, sizeof(code), initial_gas);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // PUSH1 costs 3, POP costs 2 = 5 total
  TEST_ASSERT_EQUAL_UINT64(5, result.gas_used);
}

// =============================================================================
// DUP Opcode Tests
// =============================================================================

void test_opcode_dup1_basic(void) {
  // PUSH1 5, DUP1 => [5, 5]
  uint8_t code[] = {OP_PUSH1, 5, OP_DUP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(2, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 1).limbs[0]);
}

void test_opcode_dup1_stack_underflow(void) {
  // DUP1 on empty stack
  uint8_t code[] = {OP_DUP1};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_dup1_stack_overflow(void) {
  // Fill stack to 1024 elements, then DUP1 should fail with overflow
  // We'll use a loop with PUSH1 + DUP1 pattern to fill quickly
  // Build code: PUSH1 1, then 1023 DUP1s to fill, then one more DUP1 to overflow
  uint8_t code[2 + 1023 + 1 + 1]; // PUSH1 x, 1023 DUP1s, 1 DUP1 (overflow), STOP
  code[0] = OP_PUSH1;
  code[1] = 1;
  for (int i = 0; i < 1023; i++) {
    code[2 + i] = OP_DUP1;
  }
  code[2 + 1023] = OP_DUP1; // This should overflow
  code[2 + 1024] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 10000000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_OVERFLOW, result.error);
}

void test_opcode_dup2_basic(void) {
  // PUSH1 1, PUSH1 2, DUP2 => [1, 2, 1]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_DUP2, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(3, evm_stack_size(evm.current_frame->stack));
  // Stack (top to bottom): 1, 2, 1
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 1).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 2).limbs[0]);
}

void test_opcode_dup8_basic(void) {
  // Push 8 values, DUP8 duplicates the 8th item (bottom)
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4,      OP_PUSH1, 5,
                    OP_PUSH1, 6, OP_PUSH1, 7, OP_PUSH1, 8, OP_DUP8,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(9, evm_stack_size(evm.current_frame->stack));
  // Top should be 1 (the 8th item from top, which was the first pushed)
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup16_basic(void) {
  // Push 16 values, DUP16 duplicates the 16th item (bottom)
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,      OP_PUSH1, 3,  OP_PUSH1, 4,  OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,      OP_PUSH1, 8,  OP_PUSH1, 9,  OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12,     OP_PUSH1, 13, OP_PUSH1, 14, OP_PUSH1, 15,
                    OP_PUSH1, 16, OP_DUP16, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(17, evm_stack_size(evm.current_frame->stack));
  // Top should be 1 (the 16th item from top, which was the first pushed)
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup_gas_consumption(void) {
  // PUSH1 5, DUP1 - check gas consumed (PUSH1=3, DUP1=3)
  uint8_t code[] = {OP_PUSH1, 5, OP_DUP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  uint64_t initial_gas = 100;
  execution_env_t env = make_test_env(code, sizeof(code), initial_gas);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // PUSH1 costs 3, DUP1 costs 3 = 6 total
  TEST_ASSERT_EQUAL_UINT64(6, result.gas_used);
}

// =============================================================================
// SWAP Opcode Tests
// =============================================================================

void test_opcode_swap1_basic(void) {
  // PUSH1 1, PUSH1 2, SWAP1 => [1, 2] (swapped to [2, 1] but top is now 1)
  // Wait, let me re-check: stack before SWAP1: [2, 1] (2 on top, 1 below)
  // After SWAP1: [1, 2] (1 on top, 2 below)
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_SWAP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(2, evm_stack_size(evm.current_frame->stack));
  // After SWAP1: top=1, second=2
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 1).limbs[0]);
}

void test_opcode_swap1_stack_underflow(void) {
  // SWAP1 with only 1 item on stack (needs 2)
  uint8_t code[] = {OP_PUSH1, 5, OP_SWAP1};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_swap2_basic(void) {
  // PUSH1 1, PUSH1 2, PUSH1 3, SWAP2
  // Before: [3, 2, 1] (3 on top)
  // After:  [1, 2, 3] (1 on top - swapped top with 3rd)
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_SWAP2, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(3, evm_stack_size(evm.current_frame->stack));
  // After SWAP2: top=1, second=2, third=3
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 1).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(3, evm_stack_peek_unsafe(evm.current_frame->stack, 2).limbs[0]);
}

void test_opcode_swap8_basic(void) {
  // Push 9 values, SWAP8 swaps top with 9th
  // Before: [9, 8, 7, 6, 5, 4, 3, 2, 1]
  // After:  [1, 8, 7, 6, 5, 4, 3, 2, 9]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4, OP_PUSH1, 5,
                    OP_PUSH1, 6, OP_PUSH1, 7, OP_PUSH1, 8, OP_PUSH1, 9, OP_SWAP8, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(9, evm_stack_size(evm.current_frame->stack));
  // Top should be 1, bottom (8th from top) should be 9
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(9, evm_stack_peek_unsafe(evm.current_frame->stack, 8).limbs[0]);
}

void test_opcode_swap16_basic(void) {
  // Push 17 values, SWAP16 swaps top with 17th
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1,  3,      OP_PUSH1, 4,  OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1,  8,      OP_PUSH1, 9,  OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_PUSH1,  13,     OP_PUSH1, 14, OP_PUSH1, 15,
                    OP_PUSH1, 16, OP_PUSH1, 17, OP_SWAP16, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(17, evm_stack_size(evm.current_frame->stack));
  // Top should be 1, bottom (16th from top) should be 17
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(17, evm_stack_peek_unsafe(evm.current_frame->stack, 16).limbs[0]);
}

void test_opcode_swap_gas_consumption(void) {
  // PUSH1 1, PUSH1 2, SWAP1 - check gas consumed (PUSH1=3, PUSH1=3, SWAP1=3)
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_SWAP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  uint64_t initial_gas = 100;
  execution_env_t env = make_test_env(code, sizeof(code), initial_gas);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // PUSH1 + PUSH1 + SWAP1 = 3 + 3 + 3 = 9 total
  TEST_ASSERT_EQUAL_UINT64(9, result.gas_used);
}

// =============================================================================
// Out-of-Gas Tests
// =============================================================================

void test_opcode_pop_out_of_gas(void) {
  // PUSH1 costs 3, POP costs 2 - give only 4 gas (enough for PUSH1, not POP)
  uint8_t code[] = {OP_PUSH1, 5, OP_POP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 4); // 3 for PUSH1, 1 left (need 2)
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_dup_out_of_gas(void) {
  // PUSH1 costs 3, DUP1 costs 3 - give only 5 gas
  uint8_t code[] = {OP_PUSH1, 5, OP_DUP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 5); // 3 for PUSH1, 2 left (need 3)
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_swap_out_of_gas(void) {
  // PUSH1 + PUSH1 costs 6, SWAP1 costs 3 - give only 8 gas
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_SWAP1, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 8); // 6 for PUSHes, 2 left (need 3)
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

// =============================================================================
// Middle DUP Variant Tests (DUP3-DUP7, DUP9-DUP15)
// =============================================================================

void test_opcode_dup3_basic(void) {
  // Push 3 values, DUP3 duplicates the 3rd item
  // Before: [3, 2, 1] => After: [1, 3, 2, 1]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_DUP3, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(4, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup4_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4, OP_DUP4, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(5, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup5_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3,
                    OP_PUSH1, 4, OP_PUSH1, 5, OP_DUP5,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(6, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup6_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,        OP_PUSH1, 2,        OP_PUSH1, 3,       OP_PUSH1,
                    4,        OP_PUSH1, 5,        OP_PUSH1, 6,        OP_DUP6, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(7, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup7_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4,
                    OP_PUSH1, 5, OP_PUSH1, 6, OP_PUSH1, 7, OP_DUP7,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(8, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup9_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4, OP_PUSH1, 5,
                    OP_PUSH1, 6, OP_PUSH1, 7, OP_PUSH1, 8, OP_PUSH1, 9, OP_DUP9,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(10, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup10_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2,  OP_PUSH1, 3,      OP_PUSH1, 4,
                    OP_PUSH1, 5, OP_PUSH1, 6,  OP_PUSH1, 7,      OP_PUSH1, 8,
                    OP_PUSH1, 9, OP_PUSH1, 10, OP_DUP10, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(11, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup11_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1, 4,
                    OP_PUSH1, 5, OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,
                    OP_PUSH1, 9, OP_PUSH1, 10, OP_PUSH1, 11, OP_DUP11, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(12, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup12_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,      OP_PUSH1, 4, OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,      OP_PUSH1, 9, OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_DUP12, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(13, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup13_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1, 4,      OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,  OP_PUSH1, 9,      OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_PUSH1, 13, OP_DUP13, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(14, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup14_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1, 4,  OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,  OP_PUSH1, 9,  OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_PUSH1, 13, OP_PUSH1, 14, OP_DUP14, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(15, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_dup15_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1, 4,
                    OP_PUSH1, 5,  OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,
                    OP_PUSH1, 9,  OP_PUSH1, 10, OP_PUSH1, 11, OP_PUSH1, 12,
                    OP_PUSH1, 13, OP_PUSH1, 14, OP_PUSH1, 15, OP_DUP15, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(16, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

// =============================================================================
// Middle SWAP Variant Tests (SWAP3-SWAP7, SWAP9-SWAP15)
// =============================================================================

void test_opcode_swap3_basic(void) {
  // Push 4 values, SWAP3 swaps top with 4th
  // Before: [4, 3, 2, 1] => After: [1, 3, 2, 4]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4, OP_SWAP3, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(4, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(4, evm_stack_peek_unsafe(evm.current_frame->stack, 3).limbs[0]);
}

void test_opcode_swap4_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3,
                    OP_PUSH1, 4, OP_PUSH1, 5, OP_SWAP4, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(5, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 4).limbs[0]);
}

void test_opcode_swap5_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,        OP_PUSH1, 2,        OP_PUSH1, 3,        OP_PUSH1,
                    4,        OP_PUSH1, 5,        OP_PUSH1, 6,        OP_SWAP5, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(6, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(6, evm_stack_peek_unsafe(evm.current_frame->stack, 5).limbs[0]);
}

void test_opcode_swap6_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4,
                    OP_PUSH1, 5, OP_PUSH1, 6, OP_PUSH1, 7, OP_SWAP6, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(7, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(7, evm_stack_peek_unsafe(evm.current_frame->stack, 6).limbs[0]);
}

void test_opcode_swap7_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_PUSH1, 4,      OP_PUSH1, 5,
                    OP_PUSH1, 6, OP_PUSH1, 7, OP_PUSH1, 8, OP_SWAP7, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(8, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(8, evm_stack_peek_unsafe(evm.current_frame->stack, 7).limbs[0]);
}

void test_opcode_swap9_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2,  OP_PUSH1, 3,      OP_PUSH1, 4,
                    OP_PUSH1, 5, OP_PUSH1, 6,  OP_PUSH1, 7,      OP_PUSH1, 8,
                    OP_PUSH1, 9, OP_PUSH1, 10, OP_SWAP9, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(10, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(10, evm_stack_peek_unsafe(evm.current_frame->stack, 9).limbs[0]);
}

void test_opcode_swap10_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1,  4,
                    OP_PUSH1, 5, OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1,  8,
                    OP_PUSH1, 9, OP_PUSH1, 10, OP_PUSH1, 11, OP_SWAP10, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(11, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(11, evm_stack_peek_unsafe(evm.current_frame->stack, 10).limbs[0]);
}

void test_opcode_swap11_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1,  3,      OP_PUSH1, 4, OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1,  8,      OP_PUSH1, 9, OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_SWAP11, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(12, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(12, evm_stack_peek_unsafe(evm.current_frame->stack, 11).limbs[0]);
}

void test_opcode_swap12_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1,  4,      OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,  OP_PUSH1,  9,      OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_PUSH1, 13, OP_SWAP12, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(13, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(13, evm_stack_peek_unsafe(evm.current_frame->stack, 12).limbs[0]);
}

void test_opcode_swap13_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1, 4,  OP_PUSH1,  5,
                    OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1, 8,  OP_PUSH1, 9,  OP_PUSH1,  10,
                    OP_PUSH1, 11, OP_PUSH1, 12, OP_PUSH1, 13, OP_PUSH1, 14, OP_SWAP13, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(14, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(14, evm_stack_peek_unsafe(evm.current_frame->stack, 13).limbs[0]);
}

void test_opcode_swap14_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1, 2,  OP_PUSH1, 3,  OP_PUSH1,  4,
                    OP_PUSH1, 5,  OP_PUSH1, 6,  OP_PUSH1, 7,  OP_PUSH1,  8,
                    OP_PUSH1, 9,  OP_PUSH1, 10, OP_PUSH1, 11, OP_PUSH1,  12,
                    OP_PUSH1, 13, OP_PUSH1, 14, OP_PUSH1, 15, OP_SWAP14, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(15, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(15, evm_stack_peek_unsafe(evm.current_frame->stack, 14).limbs[0]);
}

void test_opcode_swap15_basic(void) {
  uint8_t code[] = {OP_PUSH1, 1,  OP_PUSH1,  2,      OP_PUSH1, 3,  OP_PUSH1, 4,  OP_PUSH1, 5,
                    OP_PUSH1, 6,  OP_PUSH1,  7,      OP_PUSH1, 8,  OP_PUSH1, 9,  OP_PUSH1, 10,
                    OP_PUSH1, 11, OP_PUSH1,  12,     OP_PUSH1, 13, OP_PUSH1, 14, OP_PUSH1, 15,
                    OP_PUSH1, 16, OP_SWAP15, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(16, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(16, evm_stack_peek_unsafe(evm.current_frame->stack, 15).limbs[0]);
}
