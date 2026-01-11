#include "test_opcodes_logging.h"

#include "div0/evm/evm.h"
#include "div0/evm/execution_env.h"
#include "div0/evm/gas.h"
#include "div0/evm/log.h"
#include "div0/evm/memory.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
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
// LOG0-LOG4 Basic Functionality Tests
// =============================================================================

void test_opcode_log0_basic(void) {
  // Store 32 bytes in memory then LOG0
  // PUSH32 <data>, PUSH1 0, MSTORE, PUSH1 32, PUSH1 0, LOG0, STOP
  uint8_t code[1 + 32 + 2 + 1 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1); // 0x01, 0x02, ... 0x20
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // memory offset
  code[idx++] = OP_MSTORE;
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG0;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.address = make_address(0xDEADBEEF);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(32, result.logs[0].data_size);
}

void test_opcode_log1_basic(void) {
  // PUSH32 <data>, PUSH1 0, MSTORE, PUSH32 <topic0>, PUSH1 32, PUSH1 0, LOG1, STOP
  uint8_t code[1 + 32 + 2 + 1 + 1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;
  // Topic
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(32, result.logs[0].data_size);
}

void test_opcode_log2_basic(void) {
  // PUSH32 <data>, PUSH1 0, MSTORE, PUSH32 <t1>, PUSH32 <t0>, PUSH1 32, PUSH1 0, LOG2, STOP
  uint8_t code[1 + 32 + 2 + 1 + 1 + 32 + 1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;
  // Topic1 (pushed first, will be second on stack)
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xBB;
  }
  // Topic0 (pushed second, will be first on stack)
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG2;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(2, result.logs[0].topic_count);
}

void test_opcode_log3_basic(void) {
  // Similar pattern with 3 topics
  uint8_t code[200];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;
  // Topics in reverse order
  for (int t = 2; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)(0xAA + t);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG3;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(3, result.logs[0].topic_count);
}

void test_opcode_log4_basic(void) {
  // Similar pattern with 4 topics
  uint8_t code[250];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;
  // Topics in reverse order
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)(0xAA + t);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG4;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(4, result.logs[0].topic_count);
}

// =============================================================================
// Zero-Size Data Tests
// =============================================================================

void test_opcode_log0_zero_data(void) {
  // PUSH1 0 (size), PUSH1 0 (offset), LOG0, STOP
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL(EVM_OK, result.error);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].data_size);
  TEST_ASSERT_NULL(result.logs[0].data);
}

void test_opcode_log1_zero_data(void) {
  // PUSH32 <topic>, PUSH1 0 (size), PUSH1 0 (offset), LOG1, STOP
  uint8_t code[1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].data_size);
  TEST_ASSERT_NULL(result.logs[0].data);
}

void test_opcode_log4_zero_data(void) {
  // 4 topics, zero data
  uint8_t code[200];
  size_t idx = 0;
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)(0xAA + t);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG4;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(4, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].data_size);
}

// =============================================================================
// Stack Underflow Tests
// =============================================================================

void test_opcode_log0_stack_underflow(void) {
  // LOG0 needs 2 items: offset, size - but we only push 1
  uint8_t code[] = {OP_PUSH1, 0, OP_LOG0};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_log1_stack_underflow(void) {
  // LOG1 needs 3 items: offset, size, topic0 - but we only push 2
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG1};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_log2_stack_underflow(void) {
  // LOG2 needs 4 items - but we only push 3
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG2};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_log3_stack_underflow(void) {
  // LOG3 needs 5 items - but we only push 4
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG3};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

void test_opcode_log4_stack_underflow(void) {
  // LOG4 needs 6 items - but we only push 5
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG4};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_STACK_UNDERFLOW, result.error);
}

// =============================================================================
// Static Context (Write Protection) Tests
// =============================================================================

void test_opcode_log0_static_context(void) {
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.is_static = true;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_WRITE_PROTECTION, result.error);
}

void test_opcode_log1_static_context(void) {
  uint8_t code[1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.is_static = true;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_WRITE_PROTECTION, result.error);
}

void test_opcode_log4_static_context(void) {
  uint8_t code[200];
  size_t idx = 0;
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)(0xAA + t);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG4;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  env.call.is_static = true;

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_WRITE_PROTECTION, result.error);
}

// =============================================================================
// Gas Consumption Tests
// =============================================================================

void test_opcode_log0_gas_exact(void) {
  // LOG0 with 32 bytes: 375 (base) + 0 (topics) + 256 (32*8 data) = 631
  // Plus: 2*PUSH1 = 6, memory expansion for 32 bytes = 3
  // Total setup: 6 (push) + 3 (mem) + 631 (log) = 640
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas: PUSH1(3) + PUSH1(3) + LOG0(375 + 32*8 + mem_expansion(3)) = 640
  // Memory expansion for 32 bytes = 3 gas
  TEST_ASSERT_EQUAL_UINT64(640, result.gas_used);
}

void test_opcode_log1_gas_exact(void) {
  // LOG1 with 32 bytes: 375 (base) + 375 (1 topic) + 256 (32*8 data) = 1006
  // Plus: PUSH32(3) + 2*PUSH1(6) + memory expansion(3) = 1018
  uint8_t code[1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas: PUSH32(3) + PUSH1(3) + PUSH1(3) + LOG1(375 + 375 + 256 + 3) = 1018
  TEST_ASSERT_EQUAL_UINT64(1018, result.gas_used);
}

void test_opcode_log2_gas_exact(void) {
  // LOG2 with 64 bytes: 375 (base) + 750 (2 topics) + 512 (64*8) = 1637
  uint8_t code[100];
  size_t idx = 0;
  // Two topics
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xBB;
  }
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 64; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG2;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas: 2*PUSH32(6) + 2*PUSH1(6) + LOG2(375 + 750 + 512 + mem_expansion(6)) = 1655
  TEST_ASSERT_EQUAL_UINT64(1655, result.gas_used);
}

void test_opcode_log4_gas_exact(void) {
  // LOG4 with 0 bytes: 375 (base) + 1500 (4 topics) + 0 (0*8) = 1875
  uint8_t code[200];
  size_t idx = 0;
  // 4 topics
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)(0xAA + t);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG4;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas: 4*PUSH32(12) + 2*PUSH1(6) + LOG4(375 + 1500 + 0 + 0) = 1893
  TEST_ASSERT_EQUAL_UINT64(1893, result.gas_used);
}

// =============================================================================
// Out of Gas Tests
// =============================================================================

void test_opcode_log0_out_of_gas_base(void) {
  // Not enough gas for LOG0 base cost (375)
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // PUSH1 costs 3 each, so after 2 pushes we have 100 - 6 = 94 gas left
  // LOG0 base is 375, so should fail
  execution_env_t env = make_test_env(code, sizeof(code), 100);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_log1_out_of_gas_topics(void) {
  // Enough for base (375) but not for topic cost (375)
  uint8_t code[1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // After pushes (3 + 3 + 3 = 9), need 375 + 375 = 750 for LOG1
  // Give just enough for base but not topic
  execution_env_t env = make_test_env(code, idx, 9 + 400);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_log0_out_of_gas_data(void) {
  // LOG0 with large data - not enough for data cost
  uint8_t code[] = {OP_PUSH2, 0x01,   0x00, // 256 bytes
                    OP_PUSH1, 0,            // offset
                    OP_LOG0,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // Data cost: 256 * 8 = 2048, base: 375, total: 2423 + memory expansion
  // Give enough for base but not data
  execution_env_t env = make_test_env(code, sizeof(code), 400);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

void test_opcode_log0_out_of_gas_memory(void) {
  // LOG0 with data from high memory offset - memory expansion cost
  uint8_t code[] = {OP_PUSH1, 32,           // size
                    OP_PUSH2, 0x10,   0x00, // offset = 4096 (triggers memory expansion)
                    OP_LOG0,  OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  // Memory expansion for 4128 bytes is significant
  // Give enough for base + data but not memory
  execution_env_t env = make_test_env(code, sizeof(code), 700);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_ERROR, result.result);
  TEST_ASSERT_EQUAL(EVM_OUT_OF_GAS, result.error);
}

// =============================================================================
// Topic Verification Tests
// =============================================================================

void test_opcode_log1_topic_value(void) {
  // Verify that the topic value is correctly stored
  uint8_t code[1 + 32 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  // Topic value: 0x0102030405...1F20
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG1;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);

  // Verify topic content
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, result.logs[0].topics[0].bytes[i]);
  }
}

void test_opcode_log4_all_topics(void) {
  // Verify all 4 topics are correctly stored
  uint8_t code[200];
  size_t idx = 0;
  // Push topics in reverse order (topic3, topic2, topic1, topic0)
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++) {
      code[idx++] = (uint8_t)((t + 1) * 0x11);
    }
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG4;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(4, result.logs[0].topic_count);

  // Verify each topic: topic0=0x11..., topic1=0x22..., etc.
  for (int t = 0; t < 4; t++) {
    TEST_ASSERT_EQUAL_UINT8((t + 1) * 0x11, result.logs[0].topics[t].bytes[0]);
  }
}

void test_opcode_log2_topic_order(void) {
  // Verify topics are in correct order (topic0 first)
  uint8_t code[100];
  size_t idx = 0;
  // Push topic1 first, then topic0 (reversed for stack)
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xBB; // This will be topic1
  }
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA; // This will be topic0
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG2;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(2, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT8(0xAA, result.logs[0].topics[0].bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0xBB, result.logs[0].topics[1].bytes[0]);
}

// =============================================================================
// Data Verification Tests
// =============================================================================

void test_opcode_log0_data_content(void) {
  // Store specific data in memory and verify it's in the log
  uint8_t code[1 + 32 + 2 + 1 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  // Data: 0x0102030405...1F20
  for (int i = 0; i < 32; i++) {
    code[idx++] = (uint8_t)(i + 1);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE;
  code[idx++] = OP_PUSH1;
  code[idx++] = 32;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG0;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(32, result.logs[0].data_size);
  TEST_ASSERT_NOT_NULL(result.logs[0].data);

  // Verify data content
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, result.logs[0].data[i]);
  }
}

void test_opcode_log0_data_offset(void) {
  // Store data at offset 32, read from offset 32
  uint8_t code[1 + 32 + 2 + 1 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xFF;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // Store at offset 32
  code[idx++] = OP_MSTORE;
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // offset
  code[idx++] = OP_LOG0;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(32, result.logs[0].data_size);

  // Verify all bytes are 0xFF
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL_UINT8(0xFF, result.logs[0].data[i]);
  }
}

void test_opcode_log0_large_data(void) {
  // Test with 256 bytes of data
  uint8_t code[20];
  size_t idx = 0;
  // First store some pattern in memory
  // Use multiple MSTORE operations
  code[idx++] = OP_PUSH2;
  code[idx++] = 0x01;
  code[idx++] = 0x00; // 256
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG0;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(256, result.logs[0].data_size);
  TEST_ASSERT_NOT_NULL(result.logs[0].data);
  // Memory starts as zero, so all bytes should be 0
  for (int i = 0; i < 256; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, result.logs[0].data[i]);
  }
}

// =============================================================================
// Multiple Logs Tests
// =============================================================================

void test_opcode_log_multiple_logs(void) {
  // Emit 3 LOG0s in sequence
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, // Log 1
                    OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, // Log 2
                    OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, // Log 3
                    OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(3, result.logs_count);
}

void test_opcode_log_mixed_types(void) {
  // Emit LOG0, then LOG2, then LOG4
  uint8_t code[300];
  size_t idx = 0;

  // LOG0
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG0;

  // LOG2 with 2 topics
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++)
    code[idx++] = 0xBB;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++)
    code[idx++] = 0xAA;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG2;

  // LOG4 with 4 topics
  for (int t = 3; t >= 0; t--) {
    code[idx++] = OP_PUSH32;
    for (int i = 0; i < 32; i++)
      code[idx++] = (uint8_t)(0x10 + t);
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_LOG4;

  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(3, result.logs_count);
  TEST_ASSERT_EQUAL_UINT64(0, result.logs[0].topic_count);
  TEST_ASSERT_EQUAL_UINT64(2, result.logs[1].topic_count);
  TEST_ASSERT_EQUAL_UINT64(4, result.logs[2].topic_count);
}

// =============================================================================
// Address Verification Tests
// =============================================================================

void test_opcode_log0_address(void) {
  uint8_t code[] = {OP_PUSH1, 0, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  env.call.address = make_address(0xCAFEBABE);

  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  TEST_ASSERT_EQUAL_UINT64(1, result.logs_count);

  // Verify log address matches contract address
  TEST_ASSERT_TRUE(address_equal(&result.logs[0].address, &env.call.address));
}

// =============================================================================
// Memory Expansion Tests
// =============================================================================

void test_opcode_log0_memory_expansion(void) {
  // LOG0 that triggers memory expansion
  uint8_t code[] = {OP_PUSH1, 32, OP_PUSH1, 0, OP_LOG0, OP_STOP};

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, sizeof(code), 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas includes memory expansion: ceil(32/32) * 3 = 3
  // Total: PUSH1(3) + PUSH1(3) + LOG0(375 + 256 + 3) = 640
  TEST_ASSERT_EQUAL_UINT64(640, result.gas_used);
}

void test_opcode_log0_memory_preexisting(void) {
  // First expand memory with MSTORE, then LOG0 should not re-charge expansion
  uint8_t code[1 + 32 + 2 + 1 + 2 + 2 + 1 + 1];
  size_t idx = 0;
  code[idx++] = OP_PUSH32;
  for (int i = 0; i < 32; i++) {
    code[idx++] = 0xAA;
  }
  code[idx++] = OP_PUSH1;
  code[idx++] = 0;
  code[idx++] = OP_MSTORE; // This expands memory to 32 bytes
  code[idx++] = OP_PUSH1;
  code[idx++] = 32; // size
  code[idx++] = OP_PUSH1;
  code[idx++] = 0; // offset
  code[idx++] = OP_LOG0;
  code[idx++] = OP_STOP;

  evm_t evm;
  evm_init(&evm, &test_arena, FORK_SHANGHAI);

  execution_env_t env = make_test_env(code, idx, 100000);
  evm_execution_result_t result = evm_execute_env(&evm, &env);

  TEST_ASSERT_EQUAL(EVM_RESULT_STOP, result.result);
  // Gas: PUSH32(3) + PUSH1(3) + MSTORE(3 + 3 mem expansion) + PUSH1(3) + PUSH1(3) +
  //      LOG0(375 + 256 + 0 no mem expansion) = 649
  TEST_ASSERT_EQUAL_UINT64(649, result.gas_used);
}
