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
void test_evm_mstore8(void);
void test_evm_mload(void);
void test_evm_mload_mstore_roundtrip(void);
void test_evm_keccak256_empty(void);
void test_evm_keccak256_single_byte(void);
void test_evm_keccak256_32_bytes(void);
void test_evm_return_empty(void);
void test_evm_return_with_data(void);
void test_evm_revert_empty(void);
void test_evm_revert_with_data(void);
void test_evm_call_without_state(void);

// SLOAD/SSTORE tests
void test_evm_sload_empty_slot(void);
void test_evm_sstore_and_sload(void);
void test_evm_sstore_multiple_slots(void);
void test_evm_sload_gas_cold(void);
void test_evm_sload_gas_warm(void);
void test_evm_sstore_without_state(void);

// Multi-fork tests
void test_evm_init_shanghai(void);
void test_evm_init_cancun(void);
void test_evm_init_prague(void);
void test_evm_gas_refund_initialized(void);
void test_evm_gas_refund_reset(void);

// Edge case tests (underflow, out-of-gas)
void test_evm_mload_underflow(void);
void test_evm_keccak256_underflow(void);
void test_evm_mload_out_of_gas(void);
void test_evm_keccak256_out_of_gas(void);
void test_evm_mstore_underflow(void);
void test_evm_mstore8_underflow(void);

#endif // TEST_EVM_H
