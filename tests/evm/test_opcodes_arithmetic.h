#ifndef TEST_OPCODES_ARITHMETIC_H
#define TEST_OPCODES_ARITHMETIC_H

// Basic arithmetic opcode tests
void test_opcode_sub_basic(void);
void test_opcode_mul_basic(void);
void test_opcode_div_basic(void);
void test_opcode_div_by_zero(void);
void test_opcode_mod_basic(void);
void test_opcode_mod_by_zero(void);

// Signed arithmetic opcode tests
void test_opcode_sdiv_positive_by_positive(void);
void test_opcode_sdiv_negative_by_positive(void);
void test_opcode_sdiv_by_zero(void);
void test_opcode_smod_positive_by_positive(void);
void test_opcode_smod_negative_by_positive(void);
void test_opcode_smod_by_zero(void);

// Modular arithmetic opcode tests
void test_opcode_addmod_basic(void);
void test_opcode_addmod_by_zero(void);
void test_opcode_mulmod_basic(void);
void test_opcode_mulmod_by_zero(void);

// Exponentiation opcode tests
void test_opcode_exp_basic(void);
void test_opcode_exp_zero_exponent(void);
void test_opcode_exp_gas_cost(void);

// Sign extension opcode tests
void test_opcode_signextend_byte_zero(void);
void test_opcode_signextend_byte_one(void);
void test_opcode_signextend_byte_31(void);

// Gas and error tests
void test_opcode_sub_stack_underflow(void);
void test_opcode_addmod_stack_underflow(void);
void test_opcode_arithmetic_out_of_gas(void);

#endif // TEST_OPCODES_ARITHMETIC_H
