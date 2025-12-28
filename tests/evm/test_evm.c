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

void test_evm_stop(void) {
  // Just STOP
  uint8_t code[] = {OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_evm_empty_code(void) {
  // Empty code should also return STOP
  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(nullptr, 0, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_evm_push1(void) {
  // PUSH1 0x42, STOP
  uint8_t code[] = {OP_PUSH1, 0x42, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify stack has the value
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_evm_push32(void) {
  // PUSH32 <32 bytes>, STOP
  uint8_t code[34];
  code[0] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[1 + i] = (uint8_t)i;
  }
  code[33] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify stack has the value
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  // Verify the value (big-endian input)
  uint8_t output[32];
  uint256_to_bytes_be(evm_stack_peek_unsafe(evm.current_frame->stack, 0), output);
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(i, output[i]);
  }
}

void test_evm_add(void) {
  // PUSH1 10, PUSH1 20, ADD, STOP
  // Stack after: [30]
  uint8_t code[] = {OP_PUSH1, 10, OP_PUSH1, 20, OP_ADD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(30, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_evm_add_multiple(void) {
  // PUSH1 1, PUSH1 2, PUSH1 3, ADD, ADD, STOP
  // Stack: [1] -> [1,2] -> [1,2,3] -> [1,5] -> [6]
  uint8_t code[] = {OP_PUSH1, 1, OP_PUSH1, 2, OP_PUSH1, 3, OP_ADD, OP_ADD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(6, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
}

void test_evm_invalid_opcode(void) {
  // Use an opcode that is not implemented in the interpreter.
  // 0x02 (MUL) is a valid EVM opcode but not yet implemented.
  uint8_t code[] = {OP_MUL};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, result.error);
}

void test_evm_stack_underflow(void) {
  // ADD with empty stack
  uint8_t code[] = {OP_ADD};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_evm_mstore(void) {
  // PUSH1 0x42, PUSH1 0, MSTORE, STOP
  // Store 0x42 at memory offset 0
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_MSTORE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify memory was expanded and value stored
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL(32, evm_memory_size(evm.current_frame->memory));

  // MSTORE stores big-endian, so 0x42 should be at byte 31
  const uint8_t *mem = evm_memory_ptr_unsafe(evm.current_frame->memory, 0);
  TEST_ASSERT_EQUAL_UINT8(0x42, mem[31]);
  TEST_ASSERT_EQUAL_UINT8(0x00, mem[0]); // Leading zeros
}

void test_evm_mstore8(void) {
  // PUSH1 0xAB, PUSH1 5, MSTORE8, STOP
  // Store single byte 0xAB at memory offset 5
  uint8_t code[] = {OP_PUSH1, 0xAB, OP_PUSH1, 5, OP_MSTORE8, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify memory was expanded (to 32 bytes, next word boundary)
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL(32, evm_memory_size(evm.current_frame->memory));

  // MSTORE8 stores the low byte at the exact offset
  const uint8_t *mem = evm_memory_ptr_unsafe(evm.current_frame->memory, 0);
  TEST_ASSERT_EQUAL_UINT8(0xAB, mem[5]);
  TEST_ASSERT_EQUAL_UINT8(0x00, mem[0]); // Other bytes are zero
  TEST_ASSERT_EQUAL_UINT8(0x00, mem[4]);
  TEST_ASSERT_EQUAL_UINT8(0x00, mem[6]);
}

void test_evm_return_empty(void) {
  // PUSH1 0 (size), PUSH1 0 (offset), RETURN
  // RETURN pops offset first, then size from the stack.
  // Here both are 0, resulting in empty return data.
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_RETURN};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL(0, result.output_size);
}

void test_evm_return_with_data(void) {
  // PUSH1 0xAB, PUSH1 0, MSTORE, PUSH1 32, PUSH1 0, RETURN
  // Store 0xAB at offset 0, then return 32 bytes from offset 0
  uint8_t code[] = {
      OP_PUSH1,  0xAB, // value to store
      OP_PUSH1,  0,    // offset
      OP_MSTORE,       // store value
      OP_PUSH1,  32,   // size (32 bytes)
      OP_PUSH1,  0,    // offset
      OP_RETURN        // return
  };

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL(32, result.output_size);
  TEST_ASSERT_NOT_NULL(result.output);

  // Last byte should be 0xAB (big-endian)
  TEST_ASSERT_EQUAL_UINT8(0xAB, result.output[31]);
}

void test_evm_revert_empty(void) {
  // PUSH1 0, PUSH1 0, REVERT
  // Revert with offset=0, size=0 (empty revert data)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_REVERT};

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error); // Revert is not a fatal error
  TEST_ASSERT_EQUAL(0, result.output_size);
}

void test_evm_revert_with_data(void) {
  // PUSH1 0xCD, PUSH1 0, MSTORE, PUSH1 32, PUSH1 0, REVERT
  // Store 0xCD at offset 0, then revert with 32 bytes from offset 0
  uint8_t code[] = {
      OP_PUSH1,  0xCD, // value to store
      OP_PUSH1,  0,    // offset
      OP_MSTORE,       // store value
      OP_PUSH1,  32,   // size (32 bytes)
      OP_PUSH1,  0,    // offset
      OP_REVERT        // revert
  };

  evm_t evm;
  evm_init(&evm, &test_arena);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error); // Revert is not a fatal error
  TEST_ASSERT_EQUAL(32, result.output_size);
  TEST_ASSERT_NOT_NULL(result.output);

  // Last byte should be 0xCD (big-endian)
  TEST_ASSERT_EQUAL_UINT8(0xCD, result.output[31]);
}

void test_evm_call_without_state(void) {
  // CALL without state configured should fail with INVALID_OPCODE
  // Stack needs 7 items: gas, addr, value, argsOffset, argsSize, retOffset, retSize
  uint8_t code[] = {OP_PUSH1, 0,          // retSize
                    OP_PUSH1, 0,          // retOffset
                    OP_PUSH1, 0,          // argsSize
                    OP_PUSH1, 0,          // argsOffset
                    OP_PUSH1, 0,          // value
                    OP_PUSH1, 0,          // addr
                    OP_PUSH2, 0xFF, 0xFF, // gas (65535)
                    OP_CALL};

  evm_t evm;
  evm_init(&evm, &test_arena);
  // Note: evm.state is nullptr

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  // CALL without state returns INVALID_OPCODE
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, result.error);
}
