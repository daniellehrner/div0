#include "div0/evm/evm.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/state/world_state.h"
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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_evm_empty_code(void) {
  // Empty code should also return STOP
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(nullptr, 0, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_evm_push1(void) {
  // PUSH1 0x42, STOP
  uint8_t code[] = {OP_PUSH1, 0x42, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  // 0x0C is an undefined opcode in the EVM.
  uint8_t code[] = {0x0C};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, result.error);
}

void test_evm_stack_underflow(void) {
  // ADD with empty stack
  uint8_t code[] = {OP_ADD};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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

void test_evm_mload(void) {
  // PUSH1 0, MLOAD, STOP
  // Load from memory offset 0 (should be all zeros initially)
  uint8_t code[] = {OP_PUSH1, 0, OP_MLOAD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify stack has the loaded value (should be zero)
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t loaded = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(loaded));

  // Memory should be expanded to 32 bytes
  TEST_ASSERT_EQUAL(32, evm_memory_size(evm.current_frame->memory));
}

void test_evm_mload_mstore_roundtrip(void) {
  // PUSH1 0x42, PUSH1 0, MSTORE, PUSH1 0, MLOAD, STOP
  // Store 0x42 at offset 0, then load it back
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_MSTORE, OP_PUSH1, 0, OP_MLOAD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify stack has the value we stored
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t loaded = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(0x42, loaded.limbs[0]);
}

void test_evm_keccak256_empty(void) {
  // PUSH1 0 (size), PUSH1 0 (offset), KECCAK256, STOP
  // Hash of empty input
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_KECCAK256, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify stack has the keccak256 of empty string
  // keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t hash_result = evm_stack_peek_unsafe(evm.current_frame->stack, 0);

  uint8_t expected[32] = {0xc5, 0xd2, 0x46, 0x01, 0x86, 0xf7, 0x23, 0x3c, 0x92, 0x7e, 0x7d,
                          0xb2, 0xdc, 0xc7, 0x03, 0xc0, 0xe5, 0x00, 0xb6, 0x53, 0xca, 0x82,
                          0x27, 0x3b, 0x7b, 0xfa, 0xd8, 0x04, 0x5d, 0x85, 0xa4, 0x70};
  uint8_t actual[32];
  uint256_to_bytes_be(hash_result, actual);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, 32);
}

void test_evm_keccak256_single_byte(void) {
  // Store a byte at memory offset 0, then hash 1 byte
  // PUSH1 0xAB, PUSH1 0, MSTORE8, PUSH1 1 (size), PUSH1 0 (offset), KECCAK256, STOP
  uint8_t code[] = {OP_PUSH1, 0xAB, OP_PUSH1,     0,      OP_MSTORE8, OP_PUSH1, 1,
                    OP_PUSH1, 0,    OP_KECCAK256, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // keccak256(0xAB) = 468fc9c005382579139846222b7b0aebc9182ba073b2455938a86d9753bfb078
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t hash_result = evm_stack_peek_unsafe(evm.current_frame->stack, 0);

  uint8_t expected[32] = {0x46, 0x8f, 0xc9, 0xc0, 0x05, 0x38, 0x25, 0x79, 0x13, 0x98, 0x46,
                          0x22, 0x2b, 0x7b, 0x0a, 0xeb, 0xc9, 0x18, 0x2b, 0xa0, 0x73, 0xb2,
                          0x45, 0x59, 0x38, 0xa8, 0x6d, 0x97, 0x53, 0xbf, 0xb0, 0x78};
  uint8_t actual[32];
  uint256_to_bytes_be(hash_result, actual);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, 32);
}

void test_evm_keccak256_32_bytes(void) {
  // Hash 32 zero bytes from memory
  // PUSH1 32 (size), PUSH1 0 (offset), KECCAK256, STOP
  // Memory is initially zero, so hashing from offset 0 with size 32 gives us keccak256(0x00...00)
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 0, OP_KECCAK256, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // keccak256(32 zero bytes) = 290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  uint256_t hash_result = evm_stack_peek_unsafe(evm.current_frame->stack, 0);

  uint8_t expected[32] = {0x29, 0x0d, 0xec, 0xd9, 0x54, 0x8b, 0x62, 0xa8, 0xd6, 0x03, 0x45,
                          0xa9, 0x88, 0x38, 0x6f, 0xc8, 0x4b, 0xa6, 0xbc, 0x95, 0x48, 0x40,
                          0x08, 0xf6, 0x36, 0x2f, 0x93, 0x16, 0x0e, 0xf3, 0xe5, 0x63};
  uint8_t actual[32];
  uint256_to_bytes_be(hash_result, actual);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, 32);
}

void test_evm_return_empty(void) {
  // PUSH1 0 (size), PUSH1 0 (offset), RETURN
  // RETURN pops offset first, then size from the stack.
  // Here both are 0, resulting in empty return data.
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_RETURN};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

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
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  // Note: evm.state is nullptr

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  // CALL without state returns INVALID_OPCODE
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, result.error);
}

// =============================================================================
// SLOAD/SSTORE tests
// =============================================================================

void test_evm_sload_empty_slot(void) {
  // PUSH1 0 (slot), SLOAD, STOP
  // Load from slot 0 - should return 0
  uint8_t code[] = {OP_PUSH1, 0, OP_SLOAD, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have zero
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_TRUE(uint256_is_zero(evm_stack_peek_unsafe(evm.current_frame->stack, 0)));

  world_state_destroy(ws);
}

void test_evm_sstore_and_sload(void) {
  // PUSH1 0x42 (value), PUSH1 0 (slot), SSTORE, PUSH1 0 (slot), SLOAD, STOP
  // Store 0x42 at slot 0, then load it back
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_SSTORE, OP_PUSH1, 0, OP_SLOAD, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have 0x42
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0x42, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);

  world_state_destroy(ws);
}

void test_evm_sstore_multiple_slots(void) {
  // Store different values in different slots, then load them back
  // PUSH1 0xAA, PUSH1 0, SSTORE  (store 0xAA at slot 0)
  // PUSH1 0xBB, PUSH1 1, SSTORE  (store 0xBB at slot 1)
  // PUSH1 0, SLOAD               (load slot 0)
  // PUSH1 1, SLOAD               (load slot 1)
  // STOP
  uint8_t code[] = {OP_PUSH1, 0xAA,     OP_PUSH1, 0,         OP_SSTORE, OP_PUSH1,
                    0xBB,     OP_PUSH1, 1,        OP_SSTORE, OP_PUSH1,  0,
                    OP_SLOAD, OP_PUSH1, 1,        OP_SLOAD,  OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have [0xAA, 0xBB] (0xBB on top)
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(2, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_EQUAL_UINT64(0xBB, evm_stack_peek_unsafe(evm.current_frame->stack, 0).limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0xAA, evm_stack_peek_unsafe(evm.current_frame->stack, 1).limbs[0]);

  world_state_destroy(ws);
}

void test_evm_sload_gas_cold(void) {
  // PUSH1(3) + cold SLOAD(2100) + STOP(0) = 2103
  uint8_t code[] = {OP_PUSH1, 0, OP_SLOAD, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(2103, result.gas_used);

  world_state_destroy(ws);
}

void test_evm_sload_gas_warm(void) {
  // PUSH1(3) + cold SLOAD(2100) + PUSH1(3) + warm SLOAD(100) + STOP(0) = 2206
  uint8_t code[] = {OP_PUSH1, 0, OP_SLOAD, OP_PUSH1, 0, OP_SLOAD, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(2206, result.gas_used);

  world_state_destroy(ws);
}

void test_evm_sstore_without_state(void) {
  // SSTORE without state should fail
  uint8_t code[] = {OP_PUSH1, 0x42, OP_PUSH1, 0, OP_SSTORE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  // Note: evm.state is nullptr

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_INVALID_OPCODE, result.error);
}

// =============================================================================
// Multi-fork tests
// =============================================================================

void test_evm_init_shanghai(void) {
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  TEST_ASSERT_EQUAL(FORK_SHANGHAI, evm.fork);
  // PUSH1 should cost 3 gas
  TEST_ASSERT_EQUAL(3, evm.gas_table[OP_PUSH1]);
}

void test_evm_init_cancun(void) {
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_CANCUN);

  TEST_ASSERT_EQUAL(FORK_CANCUN, evm.fork);
  // PUSH1 should cost 3 gas
  TEST_ASSERT_EQUAL(3, evm.gas_table[OP_PUSH1]);
}

void test_evm_init_prague(void) {
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_PRAGUE);

  TEST_ASSERT_EQUAL(FORK_PRAGUE, evm.fork);
  // PUSH1 should cost 3 gas
  TEST_ASSERT_EQUAL(3, evm.gas_table[OP_PUSH1]);
}

void test_evm_gas_refund_initialized(void) {
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // gas_refund should be initialized to 0
  TEST_ASSERT_EQUAL(0, evm.gas_refund);
}

void test_evm_gas_refund_reset(void) {
  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // Manually set gas_refund
  evm.gas_refund = 1000;

  // Reset should clear it
  evm_reset(&evm);
  TEST_ASSERT_EQUAL(0, evm.gas_refund);
}

// =============================================================================
// Edge case tests for MLOAD and KECCAK256
// =============================================================================

void test_evm_mload_underflow(void) {
  // MLOAD with empty stack should fail
  uint8_t code[] = {OP_MLOAD};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_evm_keccak256_underflow(void) {
  // KECCAK256 needs 2 stack items (offset, size), test with only 1
  uint8_t code[] = {OP_PUSH1, 0, OP_KECCAK256};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_evm_mload_out_of_gas(void) {
  // MLOAD costs 3 (base) + memory expansion cost
  // Give only 2 gas to trigger out-of-gas
  uint8_t code[] = {OP_PUSH1, 0, OP_MLOAD};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // PUSH1 costs 3, MLOAD costs 3 + memory expansion
  // Start with 4 gas: enough for PUSH1 (3), but not enough for MLOAD (3+mem)
  execution_env_t env = make_test_env(code, sizeof(code), 4);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_evm_keccak256_out_of_gas(void) {
  // KECCAK256 costs 30 (base) + 6 * words + memory expansion
  // For empty input (size=0), only base cost of 30 is charged
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_KECCAK256};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // 2 PUSH1 = 6 gas, need 30 for KECCAK256 base cost
  // Give 35 gas: enough for PUSH1s (6), not enough for KECCAK256 (30)
  execution_env_t env = make_test_env(code, sizeof(code), 35);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_evm_mstore_underflow(void) {
  // MSTORE needs 2 stack items (offset, value), test with only 1
  uint8_t code[] = {OP_PUSH1, 0, OP_MSTORE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_evm_mstore8_underflow(void) {
  // MSTORE8 needs 2 stack items (offset, value), test with only 1
  uint8_t code[] = {OP_PUSH1, 0, OP_MSTORE8};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}
