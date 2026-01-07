#include "test_opcodes_bitwise.h"

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
// AND Opcode Tests
// =============================================================================

void test_opcode_and_basic(void) {
  // PUSH1 0x0F, PUSH1 0xFF, AND => 0xFF & 0x0F = 0x0F
  uint8_t code[] = {OP_PUSH1, 0x0F, OP_PUSH1, 0xFF, OP_AND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x0F, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_and_with_zero(void) {
  // PUSH1 0, PUSH1 0xFF, AND => 0xFF & 0 = 0
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0xFF, OP_AND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_and_with_max(void) {
  // PUSH1 0x42, PUSH1 0xFF, AND => 0xFF & 0x42 = 0x42
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0xFF, OP_AND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_and_stack_underflow(void) {
  // AND with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_AND};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// OR Opcode Tests
// =============================================================================

void test_opcode_or_basic(void) {
  // PUSH1 0x0F, PUSH1 0xF0, OR => 0xF0 | 0x0F = 0xFF
  uint8_t code[] = {OP_PUSH1, 0x0F, OP_PUSH1, 0xF0, OP_OR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0xFF, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_or_with_zero(void) {
  // PUSH1 0, PUSH1 0x42, OR => 0x42 | 0 = 0x42
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0x42, OP_OR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_or_stack_underflow(void) {
  // OR with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_OR};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// XOR Opcode Tests
// =============================================================================

void test_opcode_xor_basic(void) {
  // PUSH1 0x0F, PUSH1 0xFF, XOR => 0xFF ^ 0x0F = 0xF0
  uint8_t code[] = {OP_PUSH1, 0x0F, OP_PUSH1, 0xFF, OP_XOR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0xF0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_xor_with_self(void) {
  // PUSH1 0x42, PUSH1 0x42, XOR => 0x42 ^ 0x42 = 0
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0x42, OP_XOR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_xor_stack_underflow(void) {
  // XOR with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_XOR};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// NOT Opcode Tests
// =============================================================================

void test_opcode_not_zero(void) {
  // PUSH1 0, NOT => ~0 = MAX (all 1s)
  uint8_t code[] = {OP_PUSH1, 0, OP_NOT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[3]);
}

void test_opcode_not_max(void) {
  // PUSH32 MAX, NOT => ~MAX = 0
  uint8_t code[] = {OP_PUSH32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF,   0xFF,
                    0xFF,      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF,   0xFF,
                    0xFF,      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, OP_NOT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(val));
}

void test_opcode_not_double(void) {
  // PUSH1 0x42, NOT, NOT => ~~0x42 = 0x42
  uint8_t code[] = {OP_PUSH1, 0x42, OP_NOT, OP_NOT, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_not_stack_underflow(void) {
  // NOT with empty stack
  uint8_t code[] = {OP_NOT};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// BYTE Opcode Tests
// =============================================================================

void test_opcode_byte_extract_byte0(void) {
  // BYTE extracts the i-th byte from most significant
  // For value 0x0102...1F20 (byte 0 = 0x01), index 0 should return 0x01
  // Push 32 bytes: 0x0102030405...1F20
  uint8_t code[] = {OP_PUSH32, 0x01,   0x02, 0x03, 0x04, 0x05, 0x06,     0x07, 0x08,
                    0x09,      0x0A,   0x0B, 0x0C, 0x0D, 0x0E, 0x0F,     0x10, 0x11,
                    0x12,      0x13,   0x14, 0x15, 0x16, 0x17, 0x18,     0x19, 0x1A,
                    0x1B,      0x1C,   0x1D, 0x1E, 0x1F, 0x20, OP_PUSH1, 0, // index 0
                    OP_BYTE,   OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x01, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_byte_extract_byte31(void) {
  // For value 0x0102...1F20, index 31 should return 0x20 (least significant byte)
  uint8_t code[] = {OP_PUSH32, 0x01,   0x02, 0x03, 0x04, 0x05, 0x06,     0x07, 0x08,
                    0x09,      0x0A,   0x0B, 0x0C, 0x0D, 0x0E, 0x0F,     0x10, 0x11,
                    0x12,      0x13,   0x14, 0x15, 0x16, 0x17, 0x18,     0x19, 0x1A,
                    0x1B,      0x1C,   0x1D, 0x1E, 0x1F, 0x20, OP_PUSH1, 31, // index 31
                    OP_BYTE,   OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x20, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_byte_index_out_of_range(void) {
  // Index >= 32 should return 0
  uint8_t code[] = {OP_PUSH1, 0xFF, OP_PUSH1, 32, OP_BYTE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_byte_stack_underflow(void) {
  // BYTE with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_BYTE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// SHL Opcode Tests
// =============================================================================

void test_opcode_shl_by_one(void) {
  // PUSH1 1, PUSH1 1, SHL => 1 << 1 = 2
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 1, OP_SHL, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_shl_by_zero(void) {
  // PUSH1 0x42, PUSH1 0, SHL => 0x42 << 0 = 0x42
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_SHL, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_shl_by_256(void) {
  // PUSH1 1, PUSH2 256, SHL => 1 << 256 = 0 (overflow)
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH2, 0x01, 0x00, OP_SHL, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(val));
}

void test_opcode_shl_stack_underflow(void) {
  // SHL with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_SHL};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// SHR Opcode Tests
// =============================================================================

void test_opcode_shr_by_one(void) {
  // PUSH1 4, PUSH1 1, SHR => 4 >> 1 = 2
  uint8_t code[] = {OP_PUSH1, 4, OP_PUSH1, 1, OP_SHR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_shr_by_zero(void) {
  // PUSH1 0x42, PUSH1 0, SHR => 0x42 >> 0 = 0x42
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_SHR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_shr_by_256(void) {
  // PUSH32 MAX, PUSH2 256, SHR => MAX >> 256 = 0
  uint8_t code[] = {OP_PUSH32, 0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF,   0xFF,   0xFF, 0xFF,
                    0xFF,      0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF,   0xFF,   0xFF, 0xFF,
                    0xFF,      0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF,   0xFF,   0xFF, 0xFF,
                    0xFF,      0xFF, 0xFF, OP_PUSH2, 0x01, 0x00, OP_SHR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(val));
}

void test_opcode_shr_stack_underflow(void) {
  // SHR with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_SHR};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// SAR Opcode Tests
// =============================================================================

void test_opcode_sar_positive_value(void) {
  // PUSH1 4, PUSH1 1, SAR => 4 >>_s 1 = 2 (positive stays positive)
  uint8_t code[] = {OP_PUSH1, 4, OP_PUSH1, 1, OP_SAR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(2, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_opcode_sar_negative_value(void) {
  // SAR on -2 (all 1s except last bit) by 1 should give -1 (all 1s)
  // -2 in two's complement: 0xFF...FE
  uint8_t code[] = {OP_PUSH32, 0xFF, 0xFF,   0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF, 0xFF,   0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF, 0xFF,   0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, // -2
                    OP_PUSH1,  1,    OP_SAR, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  // Result should be -1 (all 1s)
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[3]);
}

void test_opcode_sar_negative_by_large_shift(void) {
  // SAR on -1 by 256 should give -1 (all 1s, sign-extended)
  uint8_t code[] = {OP_PUSH32, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF,      0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // -1
                    OP_PUSH2,  0x01,   0x00,                                                 // 256
                    OP_SAR,    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  // Result should be -1 (all 1s, sign-extended)
  uint256_t val = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, val.limbs[3]);
}

void test_opcode_sar_stack_underflow(void) {
  // SAR with only one item on stack
  uint8_t code[] = {OP_PUSH1, 5, OP_SAR};

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

void test_opcode_bitwise_out_of_gas(void) {
  // AND with insufficient gas (gas cost is 3)
  uint8_t code[] = {OP_PUSH1, 0x0F, OP_PUSH1, 0xFF, OP_AND, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // 3 for each PUSH1, only 2 remaining for AND (needs 3)
  execution_env_t env = make_test_env(code, sizeof(code), 8);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}
