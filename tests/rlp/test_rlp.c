#include "test_rlp.h"

#include "div0/rlp/rlp.h"

#include "unity.h"

#include <string.h>

// External arena from main test file
extern div0_arena_t test_arena;

// ===========================================================================
// Encoding Tests
// ===========================================================================

void test_rlp_encode_empty_string(void) {
  bytes_t result = rlp_encode_bytes(&test_arena, NULL, 0);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x80, result.data[0]);
}

void test_rlp_encode_single_byte_00(void) {
  uint8_t data[] = {0x00};
  bytes_t result = rlp_encode_bytes(&test_arena, data, 1);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x00, result.data[0]);
}

void test_rlp_encode_single_byte_7f(void) {
  uint8_t data[] = {0x7f};
  bytes_t result = rlp_encode_bytes(&test_arena, data, 1);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x7f, result.data[0]);
}

void test_rlp_encode_short_string_dog(void) {
  // "dog" = 0x83 0x64 0x6f 0x67
  uint8_t data[] = {'d', 'o', 'g'};
  bytes_t result = rlp_encode_bytes(&test_arena, data, 3);
  TEST_ASSERT_EQUAL_size_t(4, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x83, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8('d', result.data[1]);
  TEST_ASSERT_EQUAL_UINT8('o', result.data[2]);
  TEST_ASSERT_EQUAL_UINT8('g', result.data[3]);
}

void test_rlp_encode_short_string_55_bytes(void) {
  // 55-byte string should use short encoding (0xb7 prefix)
  uint8_t data[55];
  memset(data, 'x', 55);
  bytes_t result = rlp_encode_bytes(&test_arena, data, 55);
  TEST_ASSERT_EQUAL_size_t(56, result.size);
  TEST_ASSERT_EQUAL_UINT8(0xb7, result.data[0]); // 0x80 + 55
}

void test_rlp_encode_long_string_56_bytes(void) {
  // 56-byte string should use long encoding (0xb8 0x38 prefix)
  uint8_t data[56];
  memset(data, 'y', 56);
  bytes_t result = rlp_encode_bytes(&test_arena, data, 56);
  TEST_ASSERT_EQUAL_size_t(58, result.size);
  TEST_ASSERT_EQUAL_UINT8(0xb8, result.data[0]); // 0xb7 + 1 (1 length byte)
  TEST_ASSERT_EQUAL_UINT8(56, result.data[1]);   // length = 56
}

void test_rlp_encode_u64_zero(void) {
  bytes_t result = rlp_encode_u64(&test_arena, 0);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x80, result.data[0]);
}

void test_rlp_encode_u64_small(void) {
  // Values 1-127 encode as themselves
  bytes_t result1 = rlp_encode_u64(&test_arena, 1);
  TEST_ASSERT_EQUAL_size_t(1, result1.size);
  TEST_ASSERT_EQUAL_UINT8(0x01, result1.data[0]);

  bytes_t result127 = rlp_encode_u64(&test_arena, 127);
  TEST_ASSERT_EQUAL_size_t(1, result127.size);
  TEST_ASSERT_EQUAL_UINT8(0x7f, result127.data[0]);
}

void test_rlp_encode_u64_medium(void) {
  // 128 = 0x81 0x80 (prefix + 1 byte)
  bytes_t result128 = rlp_encode_u64(&test_arena, 128);
  TEST_ASSERT_EQUAL_size_t(2, result128.size);
  TEST_ASSERT_EQUAL_UINT8(0x81, result128.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x80, result128.data[1]);

  // 1000 = 0x82 0x03 0xe8 (prefix + 2 bytes)
  bytes_t result1000 = rlp_encode_u64(&test_arena, 1000);
  TEST_ASSERT_EQUAL_size_t(3, result1000.size);
  TEST_ASSERT_EQUAL_UINT8(0x82, result1000.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x03, result1000.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0xe8, result1000.data[2]);

  // 100000 = 0x83 0x01 0x86 0xa0 (prefix + 3 bytes)
  bytes_t result100000 = rlp_encode_u64(&test_arena, 100000);
  TEST_ASSERT_EQUAL_size_t(4, result100000.size);
  TEST_ASSERT_EQUAL_UINT8(0x83, result100000.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x01, result100000.data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x86, result100000.data[2]);
  TEST_ASSERT_EQUAL_UINT8(0xa0, result100000.data[3]);
}

void test_rlp_encode_uint256_zero(void) {
  uint256_t value = uint256_zero();
  bytes_t result = rlp_encode_uint256(&test_arena, &value);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x80, result.data[0]);
}

void test_rlp_encode_uint256_single_byte(void) {
  uint256_t value = uint256_from_u64(1);
  bytes_t result = rlp_encode_uint256(&test_arena, &value);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x01, result.data[0]);

  value = uint256_from_u64(127);
  result = rlp_encode_uint256(&test_arena, &value);
  TEST_ASSERT_EQUAL_size_t(1, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x7f, result.data[0]);
}

void test_rlp_encode_uint256_multi_byte(void) {
  // 128 = 0x81 0x80
  uint256_t value = uint256_from_u64(128);
  bytes_t result = rlp_encode_uint256(&test_arena, &value);
  TEST_ASSERT_EQUAL_size_t(2, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x81, result.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x80, result.data[1]);
}

void test_rlp_encode_address(void) {
  // Address is always 21 bytes: 0x94 + 20 bytes
  address_t addr;
  for (int i = 0; i < 20; ++i) {
    addr.bytes[i] = (uint8_t)(i + 1);
  }
  bytes_t result = rlp_encode_address(&test_arena, &addr);
  TEST_ASSERT_EQUAL_size_t(21, result.size);
  TEST_ASSERT_EQUAL_UINT8(0x94, result.data[0]);
  for (int i = 0; i < 20; ++i) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, result.data[i + 1]);
  }
}

void test_rlp_encode_empty_list(void) {
  bytes_t output;
  bytes_init_arena(&output, &test_arena);
  bytes_reserve(&output, 64);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);
  rlp_list_end(&builder);

  TEST_ASSERT_EQUAL_size_t(1, output.size);
  TEST_ASSERT_EQUAL_UINT8(0xc0, output.data[0]);
}

void test_rlp_encode_nested_list(void) {
  // Encode ["dog", "cat"] = 0xc8 0x83 0x64 0x6f 0x67 0x83 0x63 0x61 0x74
  bytes_t output;
  bytes_init_arena(&output, &test_arena);
  bytes_reserve(&output, 64);

  uint8_t dog[] = {'d', 'o', 'g'};
  uint8_t cat[] = {'c', 'a', 't'};
  bytes_t dog_enc = rlp_encode_bytes(&test_arena, dog, 3);
  bytes_t cat_enc = rlp_encode_bytes(&test_arena, cat, 3);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);
  rlp_list_append(&output, &dog_enc);
  rlp_list_append(&output, &cat_enc);
  rlp_list_end(&builder);

  TEST_ASSERT_EQUAL_size_t(9, output.size);
  TEST_ASSERT_EQUAL_UINT8(0xc8, output.data[0]); // 0xc0 + 8 (payload size)
  // dog encoding: 0x83 d o g
  TEST_ASSERT_EQUAL_UINT8(0x83, output.data[1]);
  TEST_ASSERT_EQUAL_UINT8('d', output.data[2]);
  TEST_ASSERT_EQUAL_UINT8('o', output.data[3]);
  TEST_ASSERT_EQUAL_UINT8('g', output.data[4]);
  // cat encoding: 0x83 c a t
  TEST_ASSERT_EQUAL_UINT8(0x83, output.data[5]);
  TEST_ASSERT_EQUAL_UINT8('c', output.data[6]);
  TEST_ASSERT_EQUAL_UINT8('a', output.data[7]);
  TEST_ASSERT_EQUAL_UINT8('t', output.data[8]);
}

// ===========================================================================
// Decoding Tests
// ===========================================================================

void test_rlp_decode_empty_string(void) {
  uint8_t input[] = {0x80};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(0, result.len);
}

void test_rlp_decode_single_byte_00(void) {
  uint8_t input[] = {0x00};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(1, result.len);
  TEST_ASSERT_EQUAL_UINT8(0x00, result.data[0]);
}

void test_rlp_decode_single_byte_7f(void) {
  uint8_t input[] = {0x7f};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(1, result.len);
  TEST_ASSERT_EQUAL_UINT8(0x7f, result.data[0]);
}

void test_rlp_decode_short_string_dog(void) {
  uint8_t input[] = {0x83, 'd', 'o', 'g'};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 4);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(3, result.len);
  TEST_ASSERT_EQUAL_UINT8('d', result.data[0]);
  TEST_ASSERT_EQUAL_UINT8('o', result.data[1]);
  TEST_ASSERT_EQUAL_UINT8('g', result.data[2]);
}

void test_rlp_decode_u64_zero(void) {
  uint8_t input[] = {0x80};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_u64_result_t result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(0, result.value);
}

void test_rlp_decode_u64_small(void) {
  uint8_t input1[] = {0x01};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input1, 1);
  rlp_u64_result_t result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(1, result.value);

  uint8_t input127[] = {0x7f};
  rlp_decoder_init(&decoder, input127, 1);
  result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(127, result.value);
}

void test_rlp_decode_u64_medium(void) {
  // 128 = 0x81 0x80
  uint8_t input128[] = {0x81, 0x80};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input128, 2);
  rlp_u64_result_t result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(128, result.value);

  // 1000 = 0x82 0x03 0xe8
  uint8_t input1000[] = {0x82, 0x03, 0xe8};
  rlp_decoder_init(&decoder, input1000, 3);
  result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(1000, result.value);

  // 100000 = 0x83 0x01 0x86 0xa0
  uint8_t input100000[] = {0x83, 0x01, 0x86, 0xa0};
  rlp_decoder_init(&decoder, input100000, 4);
  result = rlp_decode_u64(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_UINT64(100000, result.value);
}

void test_rlp_decode_uint256_zero(void) {
  uint8_t input[] = {0x80};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_uint256_result_t result = rlp_decode_uint256(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_TRUE(uint256_is_zero(result.value));
}

void test_rlp_decode_uint256_big(void) {
  // 0x8f 10 20 30 40 50 60 70 80 90 a0 b0 c0 d0 e0 f2 (15 bytes)
  uint8_t input[] = {0x8f, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
                     0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf2};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 16);

  rlp_uint256_result_t result = rlp_decode_uint256(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);

  // Expected: 0x102030405060708090a0b0c0d0e0f2
  // limb0 = 0x8090a0b0c0d0e0f2, limb1 = 0x0010203040506070
  uint256_t expected = uint256_from_limbs(0x8090a0b0c0d0e0f2ULL, 0x0010203040506070ULL, 0, 0);
  TEST_ASSERT_TRUE(uint256_eq(result.value, expected));
}

void test_rlp_decode_address_valid(void) {
  uint8_t input[21];
  input[0] = 0x94; // 0x80 + 20
  for (int i = 0; i < 20; ++i) {
    input[i + 1] = (uint8_t)(i + 1);
  }

  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 21);

  rlp_address_result_t result = rlp_decode_address(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  for (int i = 0; i < 20; ++i) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, result.value.bytes[i]);
  }
}

void test_rlp_decode_address_wrong_size(void) {
  // 19 bytes - too short
  uint8_t input19[20];
  input19[0] = 0x93; // 0x80 + 19
  memset(input19 + 1, 0x11, 19);

  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input19, 20);

  rlp_address_result_t result = rlp_decode_address(&decoder);
  TEST_ASSERT_EQUAL(RLP_ERR_WRONG_SIZE, result.error);
}

void test_rlp_decode_empty_list(void) {
  uint8_t input[] = {0xc0};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 1);

  rlp_list_result_t result = rlp_decode_list_header(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(0, result.payload_length);
}

void test_rlp_decode_list_items(void) {
  // ["dog", "god", "cat"] = 0xcc 0x83 d o g 0x83 g o d 0x83 c a t
  uint8_t input[] = {0xcc, 0x83, 'd', 'o', 'g', 0x83, 'g', 'o', 'd', 0x83, 'c', 'a', 't'};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 13);

  rlp_list_result_t list_result = rlp_decode_list_header(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, list_result.error);
  TEST_ASSERT_EQUAL_size_t(12, list_result.payload_length);

  rlp_bytes_result_t dog = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, dog.error);
  TEST_ASSERT_EQUAL_size_t(3, dog.len);
  TEST_ASSERT_EQUAL_UINT8('d', dog.data[0]);

  rlp_bytes_result_t god = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, god.error);
  TEST_ASSERT_EQUAL_size_t(3, god.len);
  TEST_ASSERT_EQUAL_UINT8('g', god.data[0]);

  rlp_bytes_result_t cat = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, cat.error);
  TEST_ASSERT_EQUAL_size_t(3, cat.len);
  TEST_ASSERT_EQUAL_UINT8('c', cat.data[0]);

  TEST_ASSERT_FALSE(rlp_decoder_has_more(&decoder));
}

// ===========================================================================
// Error Tests
// ===========================================================================

void test_rlp_decode_error_input_too_short(void) {
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, NULL, 0);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_ERR_INPUT_TOO_SHORT, result.error);
}

void test_rlp_decode_error_leading_zeros(void) {
  // Long string with leading zero in length: 0xb8 0x00
  uint8_t input[] = {0xb8, 0x00};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 2);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_ERR_LEADING_ZEROS, result.error);
}

void test_rlp_decode_error_non_canonical(void) {
  // 0x81 0x00 - should have been encoded as 0x00 (single byte)
  uint8_t input[] = {0x81, 0x00};
  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, input, 2);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_ERR_NON_CANONICAL, result.error);
}

// ===========================================================================
// Roundtrip Tests
// ===========================================================================

void test_rlp_roundtrip_bytes(void) {
  uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  bytes_t encoded = rlp_encode_bytes(&test_arena, test_data, 5);

  rlp_decoder_t decoder;
  rlp_decoder_init(&decoder, encoded.data, encoded.size);

  rlp_bytes_result_t result = rlp_decode_bytes(&decoder);
  TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
  TEST_ASSERT_EQUAL_size_t(5, result.len);
  TEST_ASSERT_EQUAL_MEMORY(test_data, result.data, 5);
}

void test_rlp_roundtrip_u64(void) {
  uint64_t values[] = {0, 1, 127, 128, 255, 256, 1000, 100000, UINT32_MAX, UINT64_MAX};
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    bytes_t encoded = rlp_encode_u64(&test_arena, values[i]);

    rlp_decoder_t decoder;
    rlp_decoder_init(&decoder, encoded.data, encoded.size);

    rlp_u64_result_t result = rlp_decode_u64(&decoder);
    TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
    TEST_ASSERT_EQUAL_UINT64(values[i], result.value);
  }
}

void test_rlp_roundtrip_uint256(void) {
  uint256_t values[] = {
      uint256_zero(),        uint256_from_u64(1),          uint256_from_u64(127),
      uint256_from_u64(128), uint256_from_u64(UINT64_MAX),
  };
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    bytes_t encoded = rlp_encode_uint256(&test_arena, &values[i]);

    rlp_decoder_t decoder;
    rlp_decoder_init(&decoder, encoded.data, encoded.size);

    rlp_uint256_result_t result = rlp_decode_uint256(&decoder);
    TEST_ASSERT_EQUAL(RLP_SUCCESS, result.error);
    TEST_ASSERT_TRUE(uint256_eq(values[i], result.value));
  }
}

// ===========================================================================
// Helper Tests
// ===========================================================================

void test_rlp_prefix_length(void) {
  TEST_ASSERT_EQUAL_UINT8(0, rlp_prefix_length(0x00)); // single byte
  TEST_ASSERT_EQUAL_UINT8(0, rlp_prefix_length(0x7f)); // single byte
  TEST_ASSERT_EQUAL_UINT8(1, rlp_prefix_length(0x80)); // short string
  TEST_ASSERT_EQUAL_UINT8(1, rlp_prefix_length(0xb7)); // short string max
  TEST_ASSERT_EQUAL_UINT8(2, rlp_prefix_length(0xb8)); // long string, 1 byte len
  TEST_ASSERT_EQUAL_UINT8(9, rlp_prefix_length(0xbf)); // long string, 8 byte len
  TEST_ASSERT_EQUAL_UINT8(1, rlp_prefix_length(0xc0)); // short list
  TEST_ASSERT_EQUAL_UINT8(1, rlp_prefix_length(0xf7)); // short list max
  TEST_ASSERT_EQUAL_UINT8(2, rlp_prefix_length(0xf8)); // long list, 1 byte len
  TEST_ASSERT_EQUAL_UINT8(9, rlp_prefix_length(0xff)); // long list, 8 byte len
}

void test_rlp_length_of_length(void) {
  TEST_ASSERT_EQUAL_INT(0, rlp_length_of_length(0));
  TEST_ASSERT_EQUAL_INT(0, rlp_length_of_length(55));
  TEST_ASSERT_EQUAL_INT(1, rlp_length_of_length(56));
  TEST_ASSERT_EQUAL_INT(1, rlp_length_of_length(255));
  TEST_ASSERT_EQUAL_INT(2, rlp_length_of_length(256));
  TEST_ASSERT_EQUAL_INT(2, rlp_length_of_length(65535));
  TEST_ASSERT_EQUAL_INT(3, rlp_length_of_length(65536));
}

void test_rlp_byte_length_u64(void) {
  TEST_ASSERT_EQUAL_INT(0, rlp_byte_length_u64(0));
  TEST_ASSERT_EQUAL_INT(1, rlp_byte_length_u64(1));
  TEST_ASSERT_EQUAL_INT(1, rlp_byte_length_u64(255));
  TEST_ASSERT_EQUAL_INT(2, rlp_byte_length_u64(256));
  TEST_ASSERT_EQUAL_INT(2, rlp_byte_length_u64(65535));
  TEST_ASSERT_EQUAL_INT(3, rlp_byte_length_u64(65536));
  TEST_ASSERT_EQUAL_INT(8, rlp_byte_length_u64(UINT64_MAX));
}

void test_rlp_is_string_prefix(void) {
  TEST_ASSERT_TRUE(rlp_is_string_prefix(0x00));
  TEST_ASSERT_TRUE(rlp_is_string_prefix(0x7f));
  TEST_ASSERT_TRUE(rlp_is_string_prefix(0x80));
  TEST_ASSERT_TRUE(rlp_is_string_prefix(0xbf));
  TEST_ASSERT_FALSE(rlp_is_string_prefix(0xc0));
  TEST_ASSERT_FALSE(rlp_is_string_prefix(0xff));
}

void test_rlp_is_list_prefix(void) {
  TEST_ASSERT_FALSE(rlp_is_list_prefix(0x00));
  TEST_ASSERT_FALSE(rlp_is_list_prefix(0xbf));
  TEST_ASSERT_TRUE(rlp_is_list_prefix(0xc0));
  TEST_ASSERT_TRUE(rlp_is_list_prefix(0xff));
}
