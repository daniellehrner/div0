#ifndef TEST_HEX_H
#define TEST_HEX_H

// hex utility test declarations
void test_hex_char_to_nibble_digits(void);
void test_hex_char_to_nibble_lowercase(void);
void test_hex_char_to_nibble_uppercase(void);
void test_hex_char_to_nibble_invalid(void);
void test_hex_decode_basic(void);
void test_hex_decode_with_prefix(void);
void test_hex_decode_uppercase(void);
void test_hex_decode_mixed_case(void);
void test_hex_decode_null_input(void);
void test_hex_decode_null_output(void);
void test_hex_decode_wrong_length(void);
void test_hex_decode_invalid_char(void);

#endif // TEST_HEX_H