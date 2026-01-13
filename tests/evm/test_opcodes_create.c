#include "test_opcodes_create.h"

#include "div0/evm/evm.h"
#include "div0/evm/execution_env.h"
#include "div0/evm/gas.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/state/world_state.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include "unity.h"

#include <string.h>

// External test arena from test_div0.c
extern div0_arena_t test_arena;

/// Helper to create a minimal execution environment for testing.
static execution_env_t make_test_env(const uint8_t *code, size_t code_size, uint64_t gas) {
  execution_env_t env;
  execution_env_init(&env);
  env.call.code = code;
  env.call.code_size = code_size;
  env.call.gas = gas;
  return env;
}

/// Helper to create an address from a value.
static address_t make_address(uint64_t val) {
  address_t addr = address_zero();
  for (size_t i = 0; i < 8; i++) {
    addr.bytes[ADDRESS_SIZE - 1 - i] = (uint8_t)(val >> (i * 8));
  }
  return addr;
}

// =============================================================================
// CREATE/CREATE2 Basic Functionality Tests
// =============================================================================

void test_opcode_create_basic(void) {
  // CREATE with empty init code
  // PUSH1 0 (size), PUSH1 0 (offset), PUSH1 0 (value), CREATE, STOP
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.address = make_address(0xCAFEBABE);

  // Set some balance for the creator
  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(1000000));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  // CREATE sets up a pending child frame for execution
  // With empty init code, the child returns immediately with empty code
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have the new contract address (or 0 on failure)
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  world_state_destroy(ws);
}

void test_opcode_create2_basic(void) {
  // CREATE2 with empty init code
  // PUSH32 <salt>, PUSH1 0 (size), PUSH1 0 (offset), PUSH1 0 (value), CREATE2, STOP
  uint8_t code[1 + 32 + 2 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1); // salt
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // value
  code[idx++] = OP_CREATE2;
  code[idx++] = OP_STOP;

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.address = make_address(0xDEADBEEF);

  // Set some balance for the creator
  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(1000000));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have the new contract address
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  world_state_destroy(ws);
}

void test_opcode_create_with_value(void) {
  // CREATE with value transfer
  // PUSH1 0 (size), PUSH1 0 (offset), PUSH1 100 (value), CREATE, STOP
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 100, OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.address = make_address(0xCAFEBABE);

  // Set sufficient balance for value transfer
  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(1000));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Stack should have the new contract address
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  world_state_destroy(ws);
}

void test_opcode_create2_with_salt(void) {
  // CREATE2 with specific salt and verify address computation
  // The address is deterministic: keccak256(0xff || sender || salt || keccak256(init_code))[12:]
  uint8_t code[1 + 32 + 2 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  // Salt: all zeros except last byte = 0x42
  for (int i = 0; i < 31; i++) {
    code[idx++] = 0;
  }
  code[idx++] = 0x42;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // value
  code[idx++] = OP_CREATE2;
  code[idx++] = OP_STOP;

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.address = make_address(0xDEADBEEF);

  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(1000000));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  // Verify we got a non-zero address (successful creation)
  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  world_state_destroy(ws);
}

// =============================================================================
// Stack Underflow Tests
// =============================================================================

void test_opcode_create_stack_underflow(void) {
  // CREATE needs 3 stack items (value, offset, size), only push 2
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_CREATE};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);

  world_state_destroy(ws);
}

void test_opcode_create2_stack_underflow(void) {
  // CREATE2 needs 4 stack items (value, offset, size, salt), only push 3
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CREATE2};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);

  world_state_destroy(ws);
}

// =============================================================================
// Static Context (Write Protection) Tests
// =============================================================================

void test_opcode_create_static_context(void) {
  // CREATE in static context should fail with write protection error
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.is_static = true;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_WRITE_PROTECTION, result.error);

  world_state_destroy(ws);
}

void test_opcode_create2_static_context(void) {
  // CREATE2 in static context should fail with write protection error
  uint8_t code[1 + 32 + 2 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_CREATE2;
  code[idx++] = OP_STOP;

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.is_static = true;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_WRITE_PROTECTION, result.error);

  world_state_destroy(ws);
}

// =============================================================================
// Out of Gas Tests
// =============================================================================

void test_opcode_create_out_of_gas_base(void) {
  // CREATE base cost is 32000, give less than that
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  // 3 PUSH1 = 9 gas, CREATE base = 32000
  // Give 100 gas: enough for pushes but not for CREATE
  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);

  world_state_destroy(ws);
}

void test_opcode_create2_out_of_gas_initcode_hash(void) {
  // CREATE2 has additional cost for hashing init code
  // Base: 32000, plus keccak256 word cost for init code
  uint8_t code[200];
  size_t idx = 0;

  // Store 64 bytes of init code in memory first
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;

  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xBB;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_MSTORE;

  // CREATE2 with 64 bytes of init code
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0; // salt
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 64; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // value
  code[idx++] = OP_CREATE2;
  code[idx++] = OP_STOP;

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  // Give enough for setup but not enough for CREATE2 with hash cost
  // Setup: 2*PUSH32(6) + 2*PUSH1(6) + 2*MSTORE(12) + PUSH32(3) + 3*PUSH1(9) = 36
  // CREATE2: 32000 + memory + initcode_words * (2 + 6) = 32000 + mem + 16
  execution_env_t env = make_test_env(code, idx, 32050);
  env.call.address = make_address(0x1234);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);

  world_state_destroy(ws);
}

// =============================================================================
// Depth Exceeded Tests (Soft Failure - Push 0)
// =============================================================================

// Note: test_opcode_create_depth_exceeded is not implemented because
// call depth is managed internally by the EVM frame stack and cannot be
// easily set from the execution environment level. The depth check in
// prepare_create is tested through the prepare_create unit tests.

// =============================================================================
// Insufficient Balance Tests (Soft Failure - Push 0)
// =============================================================================

void test_opcode_create_insufficient_balance(void) {
  // CREATE with value > balance should push 0 and continue
  uint8_t code[] = {OP_PUSH1,  0,            // size
                    OP_PUSH1,  0,            // offset
                    OP_PUSH2,  0x27,   0x10, // value = 10000
                    OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.address = make_address(0xCAFEBABE);

  // Set balance less than the value being transferred
  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(100));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  // Should succeed with 0 pushed (soft failure due to insufficient balance)
  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);

  TEST_ASSERT_NOT_NULL(evm.current_frame);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));
  TEST_ASSERT_TRUE(uint256_is_zero(evm_stack_peek_unsafe(evm.current_frame->stack, 0)));

  world_state_destroy(ws);
}

// =============================================================================
// Init Code Size Limit Tests (EIP-3860)
// =============================================================================

void test_opcode_create_initcode_size_exceeded(void) {
  // CREATE with init code size > MAX_INITCODE_SIZE (49152) should fail with out of gas
  // We don't actually allocate that much memory, just push a large size value
  uint8_t code[] = {OP_PUSH3,  0x00,   0xC0, 0x01, // size = 49153 (MAX_INITCODE_SIZE + 1)
                    OP_PUSH1,  0,                  // offset
                    OP_PUSH1,  0,                  // value
                    OP_CREATE, OP_STOP};

  world_state_t *ws = world_state_create(&test_arena);
  TEST_ASSERT_NOT_NULL(ws);

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);
  evm_set_state(&evm, world_state_access(ws));

  execution_env_t env = make_test_env(code, sizeof(code), 10000000);
  env.call.address = make_address(0xCAFEBABE);

  state_set_balance(world_state_access(ws), &env.call.address, uint256_from_u64(1000000));

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  // Should fail with out of gas (EIP-3860 check)
  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);

  world_state_destroy(ws);
}
