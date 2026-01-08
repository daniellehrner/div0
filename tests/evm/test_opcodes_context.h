#ifndef TEST_OPCODES_CONTEXT_H
#define TEST_OPCODES_CONTEXT_H

// ADDRESS opcode tests
void test_opcode_address_basic(void);
void test_opcode_address_gas_consumption(void);
void test_opcode_address_out_of_gas(void);
void test_opcode_address_stack_overflow(void);

// CALLER opcode tests
void test_opcode_caller_basic(void);
void test_opcode_caller_gas_consumption(void);
void test_opcode_caller_out_of_gas(void);

// CALLVALUE opcode tests
void test_opcode_callvalue_basic(void);
void test_opcode_callvalue_zero(void);
void test_opcode_callvalue_large(void);
void test_opcode_callvalue_gas_consumption(void);

// CALLDATASIZE opcode tests
void test_opcode_calldatasize_basic(void);
void test_opcode_calldatasize_empty(void);
void test_opcode_calldatasize_large(void);

// CODESIZE opcode tests
void test_opcode_codesize_basic(void);
void test_opcode_codesize_minimal(void);

// ORIGIN opcode tests
void test_opcode_origin_basic(void);
void test_opcode_origin_gas_consumption(void);

// GASPRICE opcode tests
void test_opcode_gasprice_basic(void);
void test_opcode_gasprice_large(void);

// CALLDATALOAD opcode tests
void test_opcode_calldataload_basic(void);
void test_opcode_calldataload_offset(void);
void test_opcode_calldataload_partial(void);
void test_opcode_calldataload_out_of_bounds(void);
void test_opcode_calldataload_empty(void);
void test_opcode_calldataload_stack_underflow(void);

// CALLDATACOPY opcode tests
void test_opcode_calldatacopy_basic(void);
void test_opcode_calldatacopy_zero_size(void);
void test_opcode_calldatacopy_zero_pad(void);
void test_opcode_calldatacopy_out_of_bounds(void);
void test_opcode_calldatacopy_stack_underflow(void);

// CODECOPY opcode tests
void test_opcode_codecopy_basic(void);
void test_opcode_codecopy_zero_size(void);
void test_opcode_codecopy_zero_pad(void);
void test_opcode_codecopy_stack_underflow(void);

// RETURNDATASIZE opcode tests
void test_opcode_returndatasize_zero(void);

// RETURNDATACOPY opcode tests
void test_opcode_returndatacopy_empty(void);

#endif // TEST_OPCODES_CONTEXT_H
