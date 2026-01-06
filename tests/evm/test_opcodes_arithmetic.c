#include "test_opcodes_arithmetic.h"

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
// Basic Arithmetic Opcode Tests
// =============================================================================

void test_opcode_sub_basic(void) {
  // PUSH1 3, PUSH1 5, SUB => 5 - 3 = 2
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 5, OP_SUB, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_mul_basic(void) {
  // PUSH1 3, PUSH1 5, MUL => 5 * 3 = 15
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 5, OP_MUL, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(15, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_div_basic(void) {
  // PUSH1 3, PUSH1 15, DIV => 15 / 3 = 5
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 15, OP_DIV, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_div_by_zero(void) {
  // PUSH1 0, PUSH1 100, DIV => 100 / 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 100, OP_DIV, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_mod_basic(void) {
  // PUSH1 3, PUSH1 10, MOD => 10 % 3 = 1
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 10, OP_MOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_mod_by_zero(void) {
  // PUSH1 0, PUSH1 100, MOD => 100 % 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 100, OP_MOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

// =============================================================================
// Signed Arithmetic Opcode Tests
// =============================================================================

void test_opcode_sdiv_positive_by_positive(void) {
  // PUSH1 3, PUSH1 10, SDIV => 10 / 3 = 3
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 10, OP_SDIV, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(3, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_sdiv_negative_by_positive(void) {
  // We need to push -10 (which is MAX_UINT256 - 9) and divide by 3
  // -10 in two's complement is 0xFF...F6
  // -10 / 3 = -3 (0xFF...FD in two's complement)
  // Use PUSH32 to push the negative value
  uint8_t code[69];
  size_t pc = 0;

  // Push 3
  code[pc++] = OP_PUSH1;
  code[pc++] = 3;

  // Push -10 (0xFFFF...FFF6 = two's complement of -10)
  code[pc++] = OP_PUSH32;
  for (int i = 0; i < 31; i++) {
    code[pc++] = 0xFF;
  }
  code[pc++] = 0xF6; // -10 = ...11110110

  code[pc++] = OP_SDIV;
  code[pc++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, pc, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  // Result should be -3 (0xFF...FD)
  uint256_t expected = uint256_from_limbs(UINT64_MAX - 2, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  TEST_ASSERT_TRUE(uint256_eq(evm_stack_peek_unsafe(evm.current_frame->stack, 0), expected));
}

void test_opcode_sdiv_by_zero(void) {
  // PUSH1 0, PUSH1 10, SDIV => 10 / 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 10, OP_SDIV, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_smod_positive_by_positive(void) {
  // PUSH1 3, PUSH1 10, SMOD => 10 % 3 = 1
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 10, OP_SMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_smod_negative_by_positive(void) {
  // -10 % 3 = -1 (sign follows dividend)
  uint8_t code[69];
  size_t pc = 0;

  // Push 3
  code[pc++] = OP_PUSH1;
  code[pc++] = 3;

  // Push -10 (0xFFFF...FFF6)
  code[pc++] = OP_PUSH32;
  for (int i = 0; i < 31; i++) {
    code[pc++] = 0xFF;
  }
  code[pc++] = 0xF6;

  code[pc++] = OP_SMOD;
  code[pc++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, pc, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  // Result should be -1 (all 1s)
  uint256_t expected = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  TEST_ASSERT_TRUE(uint256_eq(evm_stack_peek_unsafe(evm.current_frame->stack, 0), expected));
}

void test_opcode_smod_by_zero(void) {
  // PUSH1 0, PUSH1 10, SMOD => 10 % 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 10, OP_SMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

// =============================================================================
// Modular Arithmetic Opcode Tests
// =============================================================================

void test_opcode_addmod_basic(void) {
  // PUSH1 10, PUSH1 8, PUSH1 7, ADDMOD => (7 + 8) % 10 = 5
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 8, OP_PUSH1, 7, OP_ADDMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_addmod_by_zero(void) {
  // PUSH1 0, PUSH1 5, PUSH1 3, ADDMOD => (3 + 5) % 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 5, OP_PUSH1, 3, OP_ADDMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_mulmod_basic(void) {
  // PUSH1 10, PUSH1 3, PUSH1 5, MULMOD => (5 * 3) % 10 = 5
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 3, OP_PUSH1, 5, OP_MULMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(5, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_mulmod_by_zero(void) {
  // PUSH1 0, PUSH1 5, PUSH1 3, MULMOD => (3 * 5) % 0 = 0 (EVM semantics)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 5, OP_PUSH1, 3, OP_MULMOD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

// =============================================================================
// Exponentiation Opcode Tests
// =============================================================================

void test_opcode_exp_basic(void) {
  // PUSH1 3, PUSH1 2, EXP => 2^3 = 8
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 2, OP_EXP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(8, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_exp_zero_exponent(void) {
  // PUSH1 0, PUSH1 5, EXP => 5^0 = 1
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 5, OP_EXP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(1, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_exp_gas_cost(void) {
  // Test that EXP gas cost is calculated correctly
  // Gas = 10 (base) + 50 * byte_length(exponent)
  // For exponent = 3 (1 byte): gas = 10 + 50*1 = 60
  // Plus PUSH1 costs (3 each) = 6
  // Total = 66
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 2, OP_EXP, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  uint64_t initial_gas = 100;
  execution_env_t env = make_test_env(code, sizeof(code), initial_gas);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Gas used = 3 (PUSH1) + 3 (PUSH1) + 60 (EXP with 1-byte exp) = 66
  uint64_t expected_gas_remaining = initial_gas - 66;
  TEST_ASSERT_EQUAL_UINT64(expected_gas_remaining, evm.current_frame->gas);
}

// =============================================================================
// Sign Extension Opcode Tests
// =============================================================================

void test_opcode_signextend_byte_zero(void) {
  // PUSH1 0x7F, PUSH1 0, SIGNEXTEND => 0x7F (positive, no extension)
  uint8_t code[] = {OP_PUSH1, 0x7F, OP_PUSH1, 0, OP_SIGNEXTEND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x7F, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_signextend_byte_one(void) {
  // PUSH2 0x7FFF, PUSH1 1, SIGNEXTEND => 0x7FFF (positive, no extension)
  uint8_t code[] = {OP_PUSH2, 0x7F, 0xFF, OP_PUSH1, 1, OP_SIGNEXTEND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x7FFF, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_signextend_byte_31(void) {
  // byte_pos >= 31 means no extension, value unchanged
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 31, OP_SIGNEXTEND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

// =============================================================================
// Gas and Error Tests
// =============================================================================

void test_opcode_sub_stack_underflow(void) {
  // SUB with empty stack should fail
  uint8_t code[] = {OP_SUB};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_addmod_stack_underflow(void) {
  // ADDMOD needs 3 elements, test with only 2
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_ADDMOD};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_arithmetic_out_of_gas(void) {
  // MUL with gas = 4 (less than GAS_MID = 5)
  uint8_t code[] = {OP_PUSH1, 3, OP_PUSH1, 5, OP_MUL};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // Give enough gas for the pushes (3 each = 6) but not enough for MUL (5)
  // Total needed: 6 + 5 = 11
  execution_env_t env = make_test_env(code, sizeof(code), 10);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}
