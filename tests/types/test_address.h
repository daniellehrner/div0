#ifndef TEST_ADDRESS_H
#define TEST_ADDRESS_H

// address test declarations
void test_address_zero_is_zero(void);
void test_address_from_bytes_works(void);
void test_address_equal_works(void);
void test_address_to_uint256_roundtrip(void);
void test_address_from_uint256_truncates(void);

#endif // TEST_ADDRESS_H