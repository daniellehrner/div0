#ifndef TEST_OPCODES_LOGGING_H
#define TEST_OPCODES_LOGGING_H

// =============================================================================
// LOG0-LOG4 Basic Functionality Tests
// =============================================================================
void test_opcode_log0_basic(void);
void test_opcode_log1_basic(void);
void test_opcode_log2_basic(void);
void test_opcode_log3_basic(void);
void test_opcode_log4_basic(void);

// =============================================================================
// Zero-Size Data Tests
// =============================================================================
void test_opcode_log0_zero_data(void);
void test_opcode_log1_zero_data(void);
void test_opcode_log4_zero_data(void);

// =============================================================================
// Stack Underflow Tests
// =============================================================================
void test_opcode_log0_stack_underflow(void);
void test_opcode_log1_stack_underflow(void);
void test_opcode_log2_stack_underflow(void);
void test_opcode_log3_stack_underflow(void);
void test_opcode_log4_stack_underflow(void);

// =============================================================================
// Static Context (Write Protection) Tests
// =============================================================================
void test_opcode_log0_static_context(void);
void test_opcode_log1_static_context(void);
void test_opcode_log4_static_context(void);

// =============================================================================
// Gas Consumption Tests
// =============================================================================
void test_opcode_log0_gas_exact(void);
void test_opcode_log1_gas_exact(void);
void test_opcode_log2_gas_exact(void);
void test_opcode_log4_gas_exact(void);

// =============================================================================
// Out of Gas Tests
// =============================================================================
void test_opcode_log0_out_of_gas_base(void);
void test_opcode_log1_out_of_gas_topics(void);
void test_opcode_log0_out_of_gas_data(void);
void test_opcode_log0_out_of_gas_memory(void);

// =============================================================================
// Topic Verification Tests
// =============================================================================
void test_opcode_log1_topic_value(void);
void test_opcode_log4_all_topics(void);
void test_opcode_log2_topic_order(void);

// =============================================================================
// Data Verification Tests
// =============================================================================
void test_opcode_log0_data_content(void);
void test_opcode_log0_data_offset(void);
void test_opcode_log0_large_data(void);

// =============================================================================
// Multiple Logs Tests
// =============================================================================
void test_opcode_log_multiple_logs(void);
void test_opcode_log_mixed_types(void);

// =============================================================================
// Address Verification Tests
// =============================================================================
void test_opcode_log0_address(void);

// =============================================================================
// Memory Expansion Tests
// =============================================================================
void test_opcode_log0_memory_expansion(void);
void test_opcode_log0_memory_preexisting(void);

#endif // TEST_OPCODES_LOGGING_H
