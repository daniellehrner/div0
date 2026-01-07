#ifndef TEST_OPCODES_BITWISE_H
#define TEST_OPCODES_BITWISE_H

// AND opcode tests
void test_opcode_and_basic(void);
void test_opcode_and_with_zero(void);
void test_opcode_and_with_max(void);
void test_opcode_and_stack_underflow(void);

// OR opcode tests
void test_opcode_or_basic(void);
void test_opcode_or_with_zero(void);
void test_opcode_or_stack_underflow(void);

// XOR opcode tests
void test_opcode_xor_basic(void);
void test_opcode_xor_with_self(void);
void test_opcode_xor_stack_underflow(void);

// NOT opcode tests
void test_opcode_not_zero(void);
void test_opcode_not_max(void);
void test_opcode_not_double(void);
void test_opcode_not_stack_underflow(void);

// BYTE opcode tests
void test_opcode_byte_extract_byte0(void);
void test_opcode_byte_extract_byte31(void);
void test_opcode_byte_index_out_of_range(void);
void test_opcode_byte_stack_underflow(void);

// SHL opcode tests
void test_opcode_shl_by_one(void);
void test_opcode_shl_by_zero(void);
void test_opcode_shl_by_256(void);
void test_opcode_shl_stack_underflow(void);

// SHR opcode tests
void test_opcode_shr_by_one(void);
void test_opcode_shr_by_zero(void);
void test_opcode_shr_by_256(void);
void test_opcode_shr_stack_underflow(void);

// SAR opcode tests
void test_opcode_sar_positive_value(void);
void test_opcode_sar_negative_value(void);
void test_opcode_sar_negative_by_large_shift(void);
void test_opcode_sar_stack_underflow(void);

// Gas tests
void test_opcode_bitwise_out_of_gas(void);

#endif // TEST_OPCODES_BITWISE_H
