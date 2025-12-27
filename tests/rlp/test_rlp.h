#ifndef TEST_RLP_H
#define TEST_RLP_H

// RLP encoding tests
void test_rlp_encode_empty_string(void);
void test_rlp_encode_single_byte_00(void);
void test_rlp_encode_single_byte_7f(void);
void test_rlp_encode_short_string_dog(void);
void test_rlp_encode_short_string_55_bytes(void);
void test_rlp_encode_long_string_56_bytes(void);
void test_rlp_encode_u64_zero(void);
void test_rlp_encode_u64_small(void);
void test_rlp_encode_u64_medium(void);
void test_rlp_encode_uint256_zero(void);
void test_rlp_encode_uint256_single_byte(void);
void test_rlp_encode_uint256_multi_byte(void);
void test_rlp_encode_address(void);
void test_rlp_encode_empty_list(void);
void test_rlp_encode_nested_list(void);

// RLP decoding tests
void test_rlp_decode_empty_string(void);
void test_rlp_decode_single_byte_00(void);
void test_rlp_decode_single_byte_7f(void);
void test_rlp_decode_short_string_dog(void);
void test_rlp_decode_u64_zero(void);
void test_rlp_decode_u64_small(void);
void test_rlp_decode_u64_medium(void);
void test_rlp_decode_uint256_zero(void);
void test_rlp_decode_uint256_big(void);
void test_rlp_decode_address_valid(void);
void test_rlp_decode_address_wrong_size(void);
void test_rlp_decode_empty_list(void);
void test_rlp_decode_list_items(void);

// RLP decoding error tests
void test_rlp_decode_error_input_too_short(void);
void test_rlp_decode_error_leading_zeros(void);
void test_rlp_decode_error_non_canonical(void);

// Roundtrip tests
void test_rlp_roundtrip_bytes(void);
void test_rlp_roundtrip_u64(void);
void test_rlp_roundtrip_uint256(void);

// Helper tests
void test_rlp_prefix_length(void);
void test_rlp_length_of_length(void);
void test_rlp_byte_length_u64(void);
void test_rlp_is_string_prefix(void);
void test_rlp_is_list_prefix(void);

#endif // TEST_RLP_H
