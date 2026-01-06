#ifndef TEST_UINT256_H
#define TEST_UINT256_H

void test_uint256_zero_is_zero(void);
void test_uint256_from_u64_works(void);
void test_uint256_eq_works(void);
void test_uint256_add_no_overflow(void);
void test_uint256_add_with_carry(void);
void test_uint256_add_overflow_wraps(void);
void test_uint256_bytes_be_roundtrip(void);
void test_uint256_from_bytes_be_short(void);
void test_uint256_sub_basic(void);
void test_uint256_sub_with_borrow(void);
void test_uint256_sub_underflow_wraps(void);
void test_uint256_lt_basic(void);
void test_uint256_lt_equal(void);
void test_uint256_lt_multi_limb(void);
void test_uint256_mul_basic(void);
void test_uint256_mul_limb_boundary(void);
void test_uint256_mul_multi_limb(void);
void test_uint256_mul_overflow_wraps(void);
void test_uint256_div_basic(void);
void test_uint256_div_by_zero(void);
void test_uint256_div_smaller_than_divisor(void);
void test_uint256_div_with_remainder(void);
void test_uint256_div_multi_limb(void);
void test_uint256_mod_basic(void);
void test_uint256_mod_by_zero(void);
void test_uint256_mod_by_one(void);
void test_uint256_mod_no_remainder(void);
void test_uint256_div_mod_consistency(void);

// Signed arithmetic tests
void test_uint256_is_negative_zero(void);
void test_uint256_is_negative_positive(void);
void test_uint256_is_negative_negative(void);
void test_uint256_sdiv_by_zero(void);
void test_uint256_sdiv_positive_by_positive(void);
void test_uint256_sdiv_negative_by_positive(void);
void test_uint256_sdiv_positive_by_negative(void);
void test_uint256_sdiv_negative_by_negative(void);
void test_uint256_sdiv_min_value_by_minus_one(void);
void test_uint256_smod_by_zero(void);
void test_uint256_smod_positive_by_positive(void);
void test_uint256_smod_negative_by_positive(void);
void test_uint256_smod_positive_by_negative(void);
void test_uint256_smod_negative_by_negative(void);
void test_uint256_sdiv_smod_identity(void);

// Sign extend tests
void test_uint256_signextend_byte_pos_zero_positive(void);
void test_uint256_signextend_byte_pos_zero_negative(void);
void test_uint256_signextend_byte_pos_one_positive(void);
void test_uint256_signextend_byte_pos_one_negative(void);
void test_uint256_signextend_byte_pos_31_or_larger(void);
void test_uint256_signextend_clears_high_bits(void);

// Modular arithmetic tests
void test_uint256_addmod_by_zero(void);
void test_uint256_addmod_no_overflow(void);
void test_uint256_addmod_with_overflow(void);
void test_uint256_addmod_result_equals_modulus(void);
void test_uint256_addmod_modulus_one(void);
void test_uint256_mulmod_by_zero(void);
void test_uint256_mulmod_no_overflow(void);
void test_uint256_mulmod_with_overflow(void);
void test_uint256_mulmod_modulus_one(void);

// Exponentiation tests
void test_uint256_exp_exponent_zero(void);
void test_uint256_exp_base_zero(void);
void test_uint256_exp_base_one(void);
void test_uint256_exp_exponent_one(void);
void test_uint256_exp_small_powers(void);
void test_uint256_exp_powers_of_two(void);
void test_uint256_exp_overflow(void);

// Byte length tests
void test_uint256_byte_length_zero(void);
void test_uint256_byte_length_small_values(void);
void test_uint256_byte_length_limb_boundaries(void);

#endif // TEST_UINT256_H