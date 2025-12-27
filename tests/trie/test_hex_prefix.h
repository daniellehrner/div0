#ifndef TEST_HEX_PREFIX_H
#define TEST_HEX_PREFIX_H

// hex_prefix_encode tests
void test_hex_prefix_encode_odd_extension(void);
void test_hex_prefix_encode_even_extension(void);
void test_hex_prefix_encode_odd_leaf(void);
void test_hex_prefix_encode_even_leaf(void);
void test_hex_prefix_encode_empty(void);
void test_hex_prefix_encode_single_nibble(void);

// hex_prefix_decode tests
void test_hex_prefix_decode_odd_extension(void);
void test_hex_prefix_decode_even_extension(void);
void test_hex_prefix_decode_odd_leaf(void);
void test_hex_prefix_decode_even_leaf(void);
void test_hex_prefix_decode_empty(void);
void test_hex_prefix_decode_null_input(void);

// roundtrip tests
void test_hex_prefix_roundtrip_various(void);

#endif // TEST_HEX_PREFIX_H
