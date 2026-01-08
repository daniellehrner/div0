#include "test_opcodes_context.h"

#include "div0/evm/evm.h"
#include "div0/evm/execution_env.h"
#include "div0/evm/memory.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
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

/// Helper to create an address from a value
static address_t make_address(uint64_t val) {
  address_t addr = address_zero();
  // Store as big-endian in the last 8 bytes
  for (size_t i = 0; i < 8; i++) {
    addr.bytes[ADDRESS_SIZE - 1 - i] = (uint8_t)(val >> (i * 8));
  }
  return addr;
}

// =============================================================================
// ADDRESS Opcode Tests (0x30)
// =============================================================================

void test_opcode_address_basic(void) {
  // ADDRESS => pushes contract address
  uint8_t code[] = {OP_ADDRESS, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.address = make_address(0xDEADBEEF);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  // Check the address value
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(0xDEADBEEF, top.limbs[0]);
}

void test_opcode_address_gas_consumption(void) {
  uint8_t code[] = {OP_ADDRESS, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.gas_used); // ADDRESS costs 2 gas
}

void test_opcode_address_out_of_gas(void) {
  uint8_t code[] = {OP_ADDRESS};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 1); // Not enough gas
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_address_stack_overflow(void) {
  // Build code to fill stack then ADDRESS
  uint8_t code[2 + 1023 + 1 + 1]; // PUSH1 x, 1023 DUP1s, ADDRESS (overflow), STOP
  code[0] = OP_PUSH1;
  code[1] = 1;
  for (int i = 0; i < 1023; i++) {
    code[2 + i] = OP_DUP1;
  }
  code[2 + 1023] = OP_ADDRESS; // This should overflow
  code[2 + 1024] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 10000000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_OVERFLOW, result.error);
}

// =============================================================================
// CALLER Opcode Tests (0x33)
// =============================================================================

void test_opcode_caller_basic(void) {
  uint8_t code[] = {OP_CALLER, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.caller = make_address(0x12345678);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(0x12345678, top.limbs[0]);
}

void test_opcode_caller_gas_consumption(void) {
  uint8_t code[] = {OP_CALLER, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.gas_used);
}

void test_opcode_caller_out_of_gas(void) {
  uint8_t code[] = {OP_CALLER};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 1);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

// =============================================================================
// CALLVALUE Opcode Tests (0x34)
// =============================================================================

void test_opcode_callvalue_basic(void) {
  uint8_t code[] = {OP_CALLVALUE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.value = uint256_from_u64(1000000);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(1000000, top.limbs[0]);
}

void test_opcode_callvalue_zero(void) {
  uint8_t code[] = {OP_CALLVALUE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.value = uint256_zero();

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(top));
}

void test_opcode_callvalue_large(void) {
  uint8_t code[] = {OP_CALLVALUE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  // Large value: 1 ETH = 10^18 wei
  env.call.value = uint256_from_u64(1000000000000000000ULL);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(1000000000000000000ULL, top.limbs[0]);
}

void test_opcode_callvalue_gas_consumption(void) {
  uint8_t code[] = {OP_CALLVALUE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.gas_used);
}

// =============================================================================
// CALLDATASIZE Opcode Tests (0x36)
// =============================================================================

void test_opcode_calldatasize_basic(void) {
  uint8_t code[] = {OP_CALLDATASIZE, OP_STOP};
  uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(5, top.limbs[0]);
}

void test_opcode_calldatasize_empty(void) {
  uint8_t code[] = {OP_CALLDATASIZE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = nullptr;
  env.call.input_size = 0;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(top));
}

void test_opcode_calldatasize_large(void) {
  uint8_t code[] = {OP_CALLDATASIZE, OP_STOP};
  static uint8_t input[1000]; // Large input

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(1000, top.limbs[0]);
}

// =============================================================================
// CODESIZE Opcode Tests (0x38)
// =============================================================================

void test_opcode_codesize_basic(void) {
  uint8_t code[] = {OP_CODESIZE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(sizeof(code), top.limbs[0]);
}

void test_opcode_codesize_minimal(void) {
  uint8_t code[] = {OP_CODESIZE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.gas_used);
}

// =============================================================================
// ORIGIN Opcode Tests (0x32)
// =============================================================================

void test_opcode_origin_basic(void) {
  uint8_t code[] = {OP_ORIGIN, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.tx.origin = make_address(0xCAFEBABE);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(0xCAFEBABE, top.limbs[0]);
}

void test_opcode_origin_gas_consumption(void) {
  uint8_t code[] = {OP_ORIGIN, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.gas_used);
}

// =============================================================================
// GASPRICE Opcode Tests (0x3A)
// =============================================================================

void test_opcode_gasprice_basic(void) {
  uint8_t code[] = {OP_GASPRICE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.tx.gas_price = uint256_from_u64(20000000000ULL); // 20 gwei

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(20000000000ULL, top.limbs[0]);
}

void test_opcode_gasprice_large(void) {
  uint8_t code[] = {OP_GASPRICE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  // Large gas price: 500 gwei
  env.tx.gas_price = uint256_from_u64(500000000000ULL);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT64(500000000000ULL, top.limbs[0]);
}

// =============================================================================
// CALLDATALOAD Opcode Tests (0x35)
// =============================================================================

void test_opcode_calldataload_basic(void) {
  // PUSH1 0, CALLDATALOAD => load 32 bytes at offset 0
  uint8_t code[] = {OP_PUSH1, 0, OP_CALLDATALOAD, OP_STOP};
  uint8_t input[32];
  for (int i = 0; i < 32; i++) {
    input[i] = (uint8_t)(i + 1);
  }

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  // First byte (0x01) should be in MSB position
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT8(0x01, (top.limbs[3] >> 56) & 0xFF);
}

void test_opcode_calldataload_offset(void) {
  // PUSH1 16, CALLDATALOAD => load 32 bytes at offset 16
  uint8_t code[] = {OP_PUSH1, 16, OP_CALLDATALOAD, OP_STOP};
  uint8_t input[64];
  for (int i = 0; i < 64; i++) {
    input[i] = (uint8_t)i;
  }

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Byte at offset 16 (0x10) should be in MSB
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_EQUAL_UINT8(0x10, (top.limbs[3] >> 56) & 0xFF);
}

void test_opcode_calldataload_partial(void) {
  // PUSH1 0, CALLDATALOAD => load partial data (zero-padded)
  uint8_t code[] = {OP_PUSH1, 0, OP_CALLDATALOAD, OP_STOP};
  uint8_t input[] = {0xFF, 0xEE, 0xDD}; // Only 3 bytes

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  // First 3 bytes are data, rest is zero-padded
  TEST_ASSERT_EQUAL_UINT8(0xFF, (top.limbs[3] >> 56) & 0xFF);
  TEST_ASSERT_EQUAL_UINT64(0, top.limbs[0]); // Lower limbs should be zero
}

void test_opcode_calldataload_out_of_bounds(void) {
  // PUSH1 100, CALLDATALOAD => beyond calldata, returns zero
  uint8_t code[] = {OP_PUSH1, 100, OP_CALLDATALOAD, OP_STOP};
  uint8_t input[] = {0xFF, 0xEE, 0xDD};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(top));
}

void test_opcode_calldataload_empty(void) {
  // PUSH1 0, CALLDATALOAD with empty calldata
  uint8_t code[] = {OP_PUSH1, 0, OP_CALLDATALOAD, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = nullptr;
  env.call.input_size = 0;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(top));
}

void test_opcode_calldataload_stack_underflow(void) {
  uint8_t code[] = {OP_CALLDATALOAD}; // No offset on stack

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// CALLDATACOPY Opcode Tests (0x37)
// =============================================================================

void test_opcode_calldatacopy_basic(void) {
  // PUSH1 32 (size), PUSH1 0 (srcOffset), PUSH1 0 (destOffset), CALLDATACOPY
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 0, OP_PUSH1, 0, OP_CALLDATACOPY, OP_STOP};
  uint8_t input[32];
  for (int i = 0; i < 32; i++) {
    input[i] = (uint8_t)(i + 1);
  }

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT16(0, evm_stack_size(evm.current_frame->stack));
}

void test_opcode_calldatacopy_zero_size(void) {
  // Zero-size copy should be a no-op
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CALLDATACOPY, OP_STOP};
  uint8_t input[] = {0xFF, 0xEE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Only base cost: PUSH1*3 + CALLDATACOPY = 3+3+3+3 = 12
  TEST_ASSERT_EQUAL_UINT64(12, result.gas_used);
}

void test_opcode_calldatacopy_zero_pad(void) {
  // Copy 32 bytes from offset 5 with only 10 bytes of input
  // Should copy 5 bytes and zero-pad the rest
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 5, OP_PUSH1, 0, OP_CALLDATACOPY, OP_STOP};
  uint8_t input[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_opcode_calldatacopy_out_of_bounds(void) {
  // Copy from offset beyond calldata - should be all zeros
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 100, OP_PUSH1, 0, OP_CALLDATACOPY, OP_STOP};
  uint8_t input[] = {0xFF, 0xEE};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.input = input;
  env.call.input_size = sizeof(input);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_opcode_calldatacopy_stack_underflow(void) {
  // Only 2 args on stack instead of 3
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_CALLDATACOPY};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// CODECOPY Opcode Tests (0x39)
// =============================================================================

void test_opcode_codecopy_basic(void) {
  // Copy first 4 bytes of code to memory
  uint8_t code[] = {OP_PUSH1, 4, OP_PUSH1, 0, OP_PUSH1, 0, OP_CODECOPY, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_opcode_codecopy_zero_size(void) {
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_CODECOPY, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Only base cost
  TEST_ASSERT_EQUAL_UINT64(12, result.gas_used);
}

void test_opcode_codecopy_zero_pad(void) {
  // Copy from offset beyond code size - should zero-pad
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 100, OP_PUSH1, 0, OP_CODECOPY, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
}

void test_opcode_codecopy_stack_underflow(void) {
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_CODECOPY};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// RETURNDATASIZE Opcode Tests (0x3D)
// =============================================================================

void test_opcode_returndatasize_zero(void) {
  // RETURNDATASIZE with no prior CALL - should be 0
  uint8_t code[] = {OP_RETURNDATASIZE, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT16(1, evm_stack_size(evm.current_frame->stack));

  uint256_t top = evm_stack_peek_unsafe(evm.current_frame->stack, 0);
  TEST_ASSERT_TRUE(uint256_is_zero(top));
}

// =============================================================================
// RETURNDATACOPY Opcode Tests (0x3E)
// =============================================================================

void test_opcode_returndatacopy_empty(void) {
  // Zero-size copy with no return data should succeed
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_RETURNDATACOPY, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
}
