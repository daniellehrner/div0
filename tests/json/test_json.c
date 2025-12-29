#include "test_json.h"

#include "div0/json/json.h"
#include "div0/json/parse.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"
#include "div0/util/hex.h"

#include <stdlib.h>
#include <string.h>
#include <unity.h>

// ============================================================================
// JSON Parsing Tests
// ============================================================================

void test_json_parse_empty_object(void) {
  json_doc_t doc;
  json_result_t result = json_parse("{}", 2, &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_NOT_NULL(doc.doc);

  yyjson_val_t *root = json_doc_root(&doc);
  TEST_ASSERT_TRUE(json_is_obj(root));
  TEST_ASSERT_EQUAL(0, json_obj_size(root));

  json_doc_free(&doc);
}

void test_json_parse_nested_object(void) {
  const char *json = "{\"outer\": {\"inner\": \"value\"}}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  TEST_ASSERT_TRUE(json_is_obj(root));

  yyjson_val_t *outer = json_obj_get(root, "outer");
  TEST_ASSERT_NOT_NULL(outer);
  TEST_ASSERT_TRUE(json_is_obj(outer));

  yyjson_val_t *inner = json_obj_get(outer, "inner");
  TEST_ASSERT_NOT_NULL(inner);
  TEST_ASSERT_TRUE(json_is_str(inner));
  TEST_ASSERT_EQUAL_STRING("value", json_get_str(inner));

  json_doc_free(&doc);
}

void test_json_parse_array(void) {
  const char *json = "[1, 2, 3]";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  TEST_ASSERT_TRUE(json_is_arr(root));
  TEST_ASSERT_EQUAL(3, json_arr_len(root));

  json_doc_free(&doc);
}

void test_json_get_hex_u64(void) {
  const char *json = "{\"value\": \"0xff\", \"large\": \"0xffffffffffffffff\"}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  uint64_t value;

  TEST_ASSERT_TRUE(json_get_hex_u64(root, "value", &value));
  TEST_ASSERT_EQUAL_UINT64(0xff, value);

  TEST_ASSERT_TRUE(json_get_hex_u64(root, "large", &value));
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, value);

  json_doc_free(&doc);
}

void test_json_get_hex_uint256(void) {
  const char *json = "{\"value\": \"0x1234\"}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  uint256_t value;

  TEST_ASSERT_TRUE(json_get_hex_uint256(root, "value", &value));
  TEST_ASSERT_EQUAL_UINT64(0x1234, value.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, value.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, value.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, value.limbs[3]);

  json_doc_free(&doc);
}

void test_json_get_hex_address(void) {
  const char *json = "{\"address\": \"0x1234567890123456789012345678901234567890\"}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  address_t addr;

  TEST_ASSERT_TRUE(json_get_hex_address(root, "address", &addr));
  TEST_ASSERT_EQUAL_UINT8(0x12, addr.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0x90, addr.bytes[19]);

  json_doc_free(&doc);
}

void test_json_get_hex_hash(void) {
  const char *json = "{\"hash\": "
                     "\"0x0000000000000000000000000000000000000000000000000000"
                     "000000001234\"}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  hash_t hash;

  TEST_ASSERT_TRUE(json_get_hex_hash(root, "hash", &hash));
  TEST_ASSERT_EQUAL_UINT8(0x12, hash.bytes[30]);
  TEST_ASSERT_EQUAL_UINT8(0x34, hash.bytes[31]);

  json_doc_free(&doc);
}

void test_json_get_hex_bytes(void) {
  const char *json = "{\"code\": \"0xdeadbeef\"}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  yyjson_val_t *root = json_doc_root(&doc);
  bytes_t code;

  TEST_ASSERT_TRUE(json_get_hex_bytes(root, "code", &arena, &code));
  TEST_ASSERT_EQUAL(4, code.size);
  TEST_ASSERT_EQUAL_UINT8(0xde, code.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0xad, code.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0xbe, code.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0xef, code.data[3]);

  div0_arena_destroy(&arena);
  json_doc_free(&doc);
}

void test_json_obj_iteration(void) {
  const char *json = "{\"a\": 1, \"b\": 2, \"c\": 3}";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  json_obj_iter_t iter = json_obj_iter(root);

  int count = 0;
  const char *key;
  yyjson_val_t *val;
  while (json_obj_iter_next(&iter, &key, &val)) {
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_NOT_NULL(val);
    count++;
  }
  TEST_ASSERT_EQUAL(3, count);

  json_doc_free(&doc);
}

void test_json_arr_iteration(void) {
  const char *json = "[\"a\", \"b\", \"c\"]";
  json_doc_t doc;
  json_result_t result = json_parse(json, strlen(json), &doc);

  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_val_t *root = json_doc_root(&doc);
  json_arr_iter_t iter = json_arr_iter(root);

  int count = 0;
  yyjson_val_t *val;
  while (json_arr_iter_next(&iter, &val)) {
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(json_is_str(val));
    count++;
  }
  TEST_ASSERT_EQUAL(3, count);

  json_doc_free(&doc);
}

// ============================================================================
// JSON Writing Tests
// ============================================================================

void test_json_write_object(void) {
  json_writer_t w;
  json_result_t result = json_writer_init(&w);
  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_mut_val_t *obj = json_write_obj(&w);
  json_obj_add_str(&w, obj, "key", "value");
  json_obj_add_u64(&w, obj, "num", 42);

  size_t len;
  char *str = json_write_string(&w, obj, JSON_WRITE_COMPACT, &len);
  TEST_ASSERT_NOT_NULL(str);

  // Check that output contains expected fields
  TEST_ASSERT_NOT_NULL(strstr(str, "\"key\":\"value\""));
  TEST_ASSERT_NOT_NULL(strstr(str, "\"num\":42"));

  free(str);
  json_writer_free(&w);
}

void test_json_write_array(void) {
  json_writer_t w;
  json_result_t result = json_writer_init(&w);
  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_mut_val_t *arr = json_write_arr(&w);
  json_arr_append(&w, arr, json_write_u64(&w, 1));
  json_arr_append(&w, arr, json_write_u64(&w, 2));
  json_arr_append(&w, arr, json_write_u64(&w, 3));

  size_t len;
  char *str = json_write_string(&w, arr, JSON_WRITE_COMPACT, &len);
  TEST_ASSERT_NOT_NULL(str);
  TEST_ASSERT_EQUAL_STRING("[1,2,3]", str);

  free(str);
  json_writer_free(&w);
}

void test_json_write_hex_values(void) {
  json_writer_t w;
  json_result_t result = json_writer_init(&w);
  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_mut_val_t *obj = json_write_obj(&w);
  json_obj_add_hex_u64(&w, obj, "u64", 0xff);

  uint256_t val = uint256_zero();
  val.limbs[0] = 0x1234;
  json_obj_add_hex_uint256(&w, obj, "u256", &val);

  address_t addr;
  memset(addr.bytes, 0, ADDRESS_SIZE);
  addr.bytes[0] = 0xab;
  addr.bytes[19] = 0xcd;
  json_obj_add_hex_address(&w, obj, "addr", &addr);

  size_t len;
  char *str = json_write_string(&w, obj, JSON_WRITE_COMPACT, &len);
  TEST_ASSERT_NOT_NULL(str);

  TEST_ASSERT_NOT_NULL(strstr(str, "\"u64\":\"0xff\""));
  TEST_ASSERT_NOT_NULL(strstr(str, "\"u256\":\"0x1234\""));
  TEST_ASSERT_NOT_NULL(strstr(str, "\"addr\":\"0xab"));

  free(str);
  json_writer_free(&w);
}

// ============================================================================
// Hex Encoding Tests
// ============================================================================

void test_hex_encode_u64(void) {
  char buf[19];

  hex_encode_u64(0, buf);
  TEST_ASSERT_EQUAL_STRING("0x0", buf);

  hex_encode_u64(0xff, buf);
  TEST_ASSERT_EQUAL_STRING("0xff", buf);

  hex_encode_u64(0x1234, buf);
  TEST_ASSERT_EQUAL_STRING("0x1234", buf);

  hex_encode_u64(UINT64_MAX, buf);
  TEST_ASSERT_EQUAL_STRING("0xffffffffffffffff", buf);
}

void test_hex_encode_uint256(void) {
  char buf[67];

  uint256_t zero = uint256_zero();
  hex_encode_uint256(&zero, buf);
  TEST_ASSERT_EQUAL_STRING("0x0", buf);

  uint256_t val = uint256_zero();
  val.limbs[0] = 0xff;
  hex_encode_uint256(&val, buf);
  TEST_ASSERT_EQUAL_STRING("0xff", buf);
}

void test_hex_encode_uint256_padded(void) {
  char buf[67];

  uint256_t zero = uint256_zero();
  hex_encode_uint256_padded(&zero, buf);
  TEST_ASSERT_EQUAL(66, strlen(buf)); // "0x" + 64 chars
  TEST_ASSERT_EQUAL_STRING("0x0000000000000000000000000000000000000000000000000000000000000000",
                           buf);

  uint256_t val = uint256_zero();
  val.limbs[0] = 0xff;
  hex_encode_uint256_padded(&val, buf);
  TEST_ASSERT_EQUAL_STRING("0x00000000000000000000000000000000000000000000000000000000000000ff",
                           buf);
}

void test_hex_decode_u64(void) {
  uint64_t val;

  TEST_ASSERT_TRUE(hex_decode_u64("0x0", &val));
  TEST_ASSERT_EQUAL_UINT64(0, val);

  TEST_ASSERT_TRUE(hex_decode_u64("0xff", &val));
  TEST_ASSERT_EQUAL_UINT64(0xff, val);

  TEST_ASSERT_TRUE(hex_decode_u64("0xFF", &val));
  TEST_ASSERT_EQUAL_UINT64(0xff, val);

  TEST_ASSERT_TRUE(hex_decode_u64("1234", &val));
  TEST_ASSERT_EQUAL_UINT64(0x1234, val);

  TEST_ASSERT_TRUE(hex_decode_u64("0xffffffffffffffff", &val));
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, val);

  // Invalid inputs
  TEST_ASSERT_FALSE(hex_decode_u64("", &val));
  TEST_ASSERT_FALSE(hex_decode_u64("0x", &val));
  TEST_ASSERT_FALSE(hex_decode_u64("0x10000000000000000", &val)); // > 64 bits
}

void test_hex_decode_uint256(void) {
  uint256_t val;

  TEST_ASSERT_TRUE(hex_decode_uint256("0x0", &val));
  TEST_ASSERT_TRUE(uint256_is_zero(val));

  TEST_ASSERT_TRUE(hex_decode_uint256("0x1234", &val));
  TEST_ASSERT_EQUAL_UINT64(0x1234, val.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, val.limbs[1]);

  // Invalid inputs
  TEST_ASSERT_FALSE(hex_decode_uint256("", &val));
  TEST_ASSERT_FALSE(hex_decode_uint256("0x", &val));
}
