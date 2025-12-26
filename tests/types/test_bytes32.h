#ifndef TEST_BYTES32_H
#define TEST_BYTES32_H

// bytes32 test declarations
void test_bytes32_zero_is_zero(void);
void test_bytes32_from_bytes_works(void);
void test_bytes32_from_bytes_padded_short(void);
void test_bytes32_from_bytes_padded_long(void);
void test_bytes32_equal_works(void);
void test_bytes32_to_uint256_roundtrip(void);

#endif // TEST_BYTES32_H