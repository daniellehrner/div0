#ifndef TEST_BLOCK_EXECUTOR_H
#define TEST_BLOCK_EXECUTOR_H

// Intrinsic gas calculation tests
void test_intrinsic_gas_simple_transfer(void);
void test_intrinsic_gas_with_calldata(void);
void test_intrinsic_gas_contract_creation(void);
void test_intrinsic_gas_with_access_list(void);

// Transaction validation tests
void test_validation_valid_tx(void);
void test_validation_nonce_too_low(void);
void test_validation_nonce_too_high(void);
void test_validation_insufficient_balance(void);
void test_validation_intrinsic_gas_too_low(void);
void test_validation_gas_limit_exceeded(void);

// Block executor tests
void test_block_executor_empty_block(void);
void test_block_executor_single_tx(void);
void test_block_executor_recipient_balance(void);
void test_block_executor_coinbase_fee(void);
void test_block_executor_multiple_txs(void);
void test_block_executor_mixed_valid_rejected(void);
void test_block_executor_nonce_increment_on_failed_execution(void);

#endif // TEST_BLOCK_EXECUTOR_H
