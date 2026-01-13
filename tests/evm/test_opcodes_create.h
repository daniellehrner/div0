#ifndef TEST_OPCODES_CREATE_H
#define TEST_OPCODES_CREATE_H

// =============================================================================
// CREATE/CREATE2 Basic Functionality Tests
// =============================================================================
void test_opcode_create_basic(void);
void test_opcode_create2_basic(void);
void test_opcode_create_with_value(void);
void test_opcode_create2_with_salt(void);

// =============================================================================
// Stack Underflow Tests
// =============================================================================
void test_opcode_create_stack_underflow(void);
void test_opcode_create2_stack_underflow(void);

// =============================================================================
// Static Context (Write Protection) Tests
// =============================================================================
void test_opcode_create_static_context(void);
void test_opcode_create2_static_context(void);

// =============================================================================
// Out of Gas Tests
// =============================================================================
void test_opcode_create_out_of_gas_base(void);
void test_opcode_create2_out_of_gas_initcode_hash(void);

// =============================================================================
// Depth Exceeded Tests (Soft Failure - Push 0)
// =============================================================================
// Note: depth exceeded cannot be easily tested from execution environment level

// =============================================================================
// Insufficient Balance Tests (Soft Failure - Push 0)
// =============================================================================
void test_opcode_create_insufficient_balance(void);

// =============================================================================
// Init Code Size Limit Tests (EIP-3860)
// =============================================================================
void test_opcode_create_initcode_size_exceeded(void);

#endif // TEST_OPCODES_CREATE_H
