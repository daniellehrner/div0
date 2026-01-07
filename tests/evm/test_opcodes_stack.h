#ifndef TEST_OPCODES_STACK_H
#define TEST_OPCODES_STACK_H

// POP opcode tests
void test_opcode_pop_basic(void);
void test_opcode_pop_multiple(void);
void test_opcode_pop_stack_underflow(void);
void test_opcode_pop_gas_consumption(void);
void test_opcode_pop_out_of_gas(void);

// DUP opcode tests
void test_opcode_dup1_basic(void);
void test_opcode_dup1_stack_underflow(void);
void test_opcode_dup1_stack_overflow(void);
void test_opcode_dup2_basic(void);
void test_opcode_dup3_basic(void);
void test_opcode_dup4_basic(void);
void test_opcode_dup5_basic(void);
void test_opcode_dup6_basic(void);
void test_opcode_dup7_basic(void);
void test_opcode_dup8_basic(void);
void test_opcode_dup9_basic(void);
void test_opcode_dup10_basic(void);
void test_opcode_dup11_basic(void);
void test_opcode_dup12_basic(void);
void test_opcode_dup13_basic(void);
void test_opcode_dup14_basic(void);
void test_opcode_dup15_basic(void);
void test_opcode_dup16_basic(void);
void test_opcode_dup_gas_consumption(void);
void test_opcode_dup_out_of_gas(void);

// SWAP opcode tests
void test_opcode_swap1_basic(void);
void test_opcode_swap1_stack_underflow(void);
void test_opcode_swap2_basic(void);
void test_opcode_swap3_basic(void);
void test_opcode_swap4_basic(void);
void test_opcode_swap5_basic(void);
void test_opcode_swap6_basic(void);
void test_opcode_swap7_basic(void);
void test_opcode_swap8_basic(void);
void test_opcode_swap9_basic(void);
void test_opcode_swap10_basic(void);
void test_opcode_swap11_basic(void);
void test_opcode_swap12_basic(void);
void test_opcode_swap13_basic(void);
void test_opcode_swap14_basic(void);
void test_opcode_swap15_basic(void);
void test_opcode_swap16_basic(void);
void test_opcode_swap_gas_consumption(void);
void test_opcode_swap_out_of_gas(void);

#endif // TEST_OPCODES_STACK_H
