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
void test_evm_mstore(void);
void test_evm_return_empty(void);
void test_evm_return_with_data(void);
void test_evm_revert_empty(void);
void test_evm_revert_with_data(void);
void test_evm_call_without_state(void);

#endif // TEST_EVM_H