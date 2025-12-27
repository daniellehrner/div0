#ifndef TEST_EVM_H
#define TEST_EVM_H

void test_evm_stop(void);
void test_evm_empty_code(void);
void test_evm_push1(void);
void test_evm_push32(void);
void test_evm_add(void);
void test_evm_add_multiple(void);
void test_evm_invalid_opcode(void);
void test_evm_stack_underflow(void);

#endif // TEST_EVM_H