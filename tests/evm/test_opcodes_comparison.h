#ifndef TEST_OPCODES_COMPARISON_H
#define TEST_OPCODES_COMPARISON_H

// LT (Less Than) opcode tests
void test_opcode_lt_true_when_less(void);
void test_opcode_lt_false_when_greater(void);
void test_opcode_lt_false_when_equal(void);
void test_opcode_lt_stack_underflow(void);

// GT (Greater Than) opcode tests
void test_opcode_gt_true_when_greater(void);
void test_opcode_gt_false_when_less(void);
void test_opcode_gt_false_when_equal(void);
void test_opcode_gt_stack_underflow(void);

// EQ (Equality) opcode tests
void test_opcode_eq_true_when_equal(void);
void test_opcode_eq_false_when_not_equal(void);
void test_opcode_eq_zero_equals_zero(void);
void test_opcode_eq_stack_underflow(void);

// ISZERO opcode tests
void test_opcode_iszero_true_when_zero(void);
void test_opcode_iszero_false_when_nonzero(void);
void test_opcode_iszero_false_when_one(void);
void test_opcode_iszero_stack_underflow(void);

// SLT (Signed Less Than) opcode tests
void test_opcode_slt_positive_less_than_positive(void);
void test_opcode_slt_negative_less_than_positive(void);
void test_opcode_slt_positive_not_less_than_negative(void);
void test_opcode_slt_stack_underflow(void);

// SGT (Signed Greater Than) opcode tests
void test_opcode_sgt_positive_greater_than_negative(void);
void test_opcode_sgt_negative_not_greater_than_positive(void);
void test_opcode_sgt_larger_positive_greater(void);
void test_opcode_sgt_stack_underflow(void);

// Gas tests
void test_opcode_comparison_out_of_gas(void);

#endif // TEST_OPCODES_COMPARISON_H
