#ifndef TEST_JSON_H
#define TEST_JSON_H

// JSON Core Tests
void test_json_parse_empty_object(void);
void test_json_parse_nested_object(void);
void test_json_parse_array(void);
void test_json_get_hex_u64(void);
void test_json_get_hex_uint256(void);
void test_json_get_hex_address(void);
void test_json_get_hex_hash(void);
void test_json_get_hex_bytes(void);
void test_json_obj_iteration(void);
void test_json_arr_iteration(void);
void test_json_write_object(void);
void test_json_write_array(void);
void test_json_write_hex_values(void);

// Hex encoding tests
void test_hex_encode_u64(void);
void test_hex_encode_uint256(void);
void test_hex_encode_uint256_padded(void);
void test_hex_decode_u64(void);
void test_hex_decode_uint256(void);

#endif // TEST_JSON_H
