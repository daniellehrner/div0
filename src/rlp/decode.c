#include "div0/rlp/decode.h"

#include <string.h>

/// Helper: get remaining bytes in decoder.
static inline size_t remaining(const rlp_decoder_t *decoder) {
  return decoder->size - decoder->pos;
}

/// Helper: read a single byte and advance position.
static inline uint8_t read_byte(rlp_decoder_t *decoder) {
  return decoder->input[decoder->pos++];
}

/// Helper: decode length from multi-byte encoding.
static rlp_bytes_result_t decode_length(rlp_decoder_t *decoder, int num_bytes) {
  rlp_bytes_result_t result = {
      .data = nullptr, .len = 0, .bytes_consumed = 0, .error = RLP_SUCCESS};

  if (remaining(decoder) < (size_t)num_bytes) {
    result.error = RLP_ERR_INPUT_TOO_SHORT;
    return result;
  }

  // Check for leading zeros (non-canonical)
  if (decoder->input[decoder->pos] == 0) {
    result.error = RLP_ERR_LEADING_ZEROS;
    return result;
  }

  size_t length = 0;
  for (int i = 0; i < num_bytes; ++i) {
    length = (length << 8) | read_byte(decoder);
  }

  result.len = length;
  result.bytes_consumed = (size_t)num_bytes;
  return result;
}

rlp_bytes_result_t rlp_decode_bytes(rlp_decoder_t *decoder) {
  rlp_bytes_result_t result = {
      .data = nullptr, .len = 0, .bytes_consumed = 0, .error = RLP_SUCCESS};

  if (!rlp_decoder_has_more(decoder)) {
    result.error = RLP_ERR_INPUT_TOO_SHORT;
    return result;
  }

  size_t start_pos = decoder->pos;
  uint8_t prefix = read_byte(decoder);

  // Single byte [0x00, 0x7f]
  if (prefix <= RLP_SINGLE_BYTE_MAX) {
    result.data = decoder->input + start_pos;
    result.len = 1;
    result.bytes_consumed = 1;
    return result;
  }

  // Short string [0x80, 0xb7]: length = prefix - 0x80
  if (prefix <= RLP_SHORT_STRING_MAX) {
    size_t len = prefix - RLP_EMPTY_STRING_BYTE;

    if (remaining(decoder) < len) {
      result.error = RLP_ERR_INPUT_TOO_SHORT;
      return result;
    }

    // Check canonical: single byte in [0x00, 0x7f] should not use string encoding
    if (len == 1 && decoder->input[decoder->pos] <= RLP_SINGLE_BYTE_MAX) {
      result.error = RLP_ERR_NON_CANONICAL;
      return result;
    }

    result.data = decoder->input + decoder->pos;
    result.len = len;
    decoder->pos += len;
    result.bytes_consumed = 1 + len;
    return result;
  }

  // Long string [0xb8, 0xbf]: length of length = prefix - 0xb7
  if (prefix <= RLP_LONG_STRING_MAX) {
    int len_of_len = prefix - RLP_SHORT_STRING_MAX;

    rlp_bytes_result_t len_result = decode_length(decoder, len_of_len);
    if (len_result.error != RLP_SUCCESS) {
      result.error = len_result.error;
      return result;
    }

    size_t len = len_result.len;

    // Check canonical: could this have used short string encoding?
    if (len < RLP_SMALL_PREFIX_BARRIER) {
      result.error = RLP_ERR_NON_CANONICAL;
      return result;
    }

    if (remaining(decoder) < len) {
      result.error = RLP_ERR_INPUT_TOO_SHORT;
      return result;
    }

    result.data = decoder->input + decoder->pos;
    result.len = len;
    decoder->pos += len;
    result.bytes_consumed = 1 + (size_t)len_of_len + len;
    return result;
  }

  // This is a list prefix, not a string
  result.error = RLP_ERR_INVALID_PREFIX;
  return result;
}

rlp_u64_result_t rlp_decode_u64(rlp_decoder_t *decoder) {
  rlp_u64_result_t result = {.value = 0, .bytes_consumed = 0, .error = RLP_SUCCESS};

  rlp_bytes_result_t bytes_result = rlp_decode_bytes(decoder);
  if (bytes_result.error != RLP_SUCCESS) {
    result.error = bytes_result.error;
    return result;
  }

  // Empty bytes = 0
  if (bytes_result.len == 0) {
    result.bytes_consumed = bytes_result.bytes_consumed;
    return result;
  }

  // Check for integer overflow
  if (bytes_result.len > 8) {
    result.error = RLP_ERR_INTEGER_OVERFLOW;
    return result;
  }

  // Check for leading zeros (non-canonical for integers)
  if (bytes_result.len > 1 && bytes_result.data[0] == 0) {
    result.error = RLP_ERR_LEADING_ZEROS;
    return result;
  }

  // Decode big-endian
  uint64_t value = 0;
  for (size_t i = 0; i < bytes_result.len; ++i) {
    value = (value << 8) | bytes_result.data[i];
  }

  result.value = value;
  result.bytes_consumed = bytes_result.bytes_consumed;
  return result;
}

rlp_uint256_result_t rlp_decode_uint256(rlp_decoder_t *decoder) {
  rlp_uint256_result_t result = {
      .value = uint256_zero(), .bytes_consumed = 0, .error = RLP_SUCCESS};

  rlp_bytes_result_t bytes_result = rlp_decode_bytes(decoder);
  if (bytes_result.error != RLP_SUCCESS) {
    result.error = bytes_result.error;
    return result;
  }

  // Empty bytes = 0
  if (bytes_result.len == 0) {
    result.bytes_consumed = bytes_result.bytes_consumed;
    return result;
  }

  // Check for overflow
  if (bytes_result.len > 32) {
    result.error = RLP_ERR_INTEGER_OVERFLOW;
    return result;
  }

  // Check for leading zeros (non-canonical)
  if (bytes_result.len > 1 && bytes_result.data[0] == 0) {
    result.error = RLP_ERR_LEADING_ZEROS;
    return result;
  }

  // Use uint256_from_bytes_be which handles variable length
  result.value = uint256_from_bytes_be(bytes_result.data, bytes_result.len);
  result.bytes_consumed = bytes_result.bytes_consumed;
  return result;
}

rlp_address_result_t rlp_decode_address(rlp_decoder_t *decoder) {
  rlp_address_result_t result = {
      .value = address_zero(), .bytes_consumed = 0, .error = RLP_SUCCESS};

  rlp_bytes_result_t bytes_result = rlp_decode_bytes(decoder);
  if (bytes_result.error != RLP_SUCCESS) {
    result.error = bytes_result.error;
    return result;
  }

  // Address must be exactly 20 bytes
  if (bytes_result.len != ADDRESS_SIZE) {
    result.error = RLP_ERR_WRONG_SIZE;
    return result;
  }

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.value.bytes, bytes_result.data, ADDRESS_SIZE);
  result.bytes_consumed = bytes_result.bytes_consumed;
  return result;
}

rlp_list_result_t rlp_decode_list_header(rlp_decoder_t *decoder) {
  rlp_list_result_t result = {.payload_length = 0, .bytes_consumed = 0, .error = RLP_SUCCESS};

  if (!rlp_decoder_has_more(decoder)) {
    result.error = RLP_ERR_INPUT_TOO_SHORT;
    return result;
  }

  uint8_t prefix = read_byte(decoder);

  // Check it's actually a list
  if (prefix <= RLP_LONG_STRING_MAX) {
    result.error = RLP_ERR_INVALID_PREFIX;
    return result;
  }

  // Short list [0xc0, 0xf7]: payload length = prefix - 0xc0
  if (prefix <= RLP_SHORT_LIST_MAX) {
    result.payload_length = prefix - RLP_EMPTY_LIST_BYTE;
    result.bytes_consumed = 1;
    return result;
  }

  // Long list [0xf8, 0xff]: length of length = prefix - 0xf7
  int len_of_len = prefix - RLP_SHORT_LIST_MAX;

  rlp_bytes_result_t len_result = decode_length(decoder, len_of_len);
  if (len_result.error != RLP_SUCCESS) {
    result.error = len_result.error;
    return result;
  }

  size_t payload_len = len_result.len;

  // Check canonical: could this have used short list encoding?
  if (payload_len < RLP_SMALL_PREFIX_BARRIER) {
    result.error = RLP_ERR_NON_CANONICAL;
    return result;
  }

  result.payload_length = payload_len;
  result.bytes_consumed = 1 + (size_t)len_of_len;
  return result;
}

void rlp_skip_item(rlp_decoder_t *decoder) {
  if (!rlp_decoder_has_more(decoder)) {
    return;
  }

  uint8_t prefix = decoder->input[decoder->pos];

  // Single byte
  if (prefix <= RLP_SINGLE_BYTE_MAX) {
    ++decoder->pos;
    return;
  }

  // Short string
  if (prefix <= RLP_SHORT_STRING_MAX) {
    size_t len = prefix - RLP_EMPTY_STRING_BYTE;
    size_t new_pos = decoder->pos + 1 + len;
    decoder->pos = new_pos < decoder->size ? new_pos : decoder->size;
    return;
  }

  // Long string
  if (prefix <= RLP_LONG_STRING_MAX) {
    int len_of_len = prefix - RLP_SHORT_STRING_MAX;
    ++decoder->pos; // consume prefix

    size_t len = 0;
    for (int i = 0; i < len_of_len && decoder->pos < decoder->size; ++i) {
      len = (len << 8) | decoder->input[decoder->pos++];
    }
    size_t new_pos = decoder->pos + len;
    decoder->pos = new_pos < decoder->size ? new_pos : decoder->size;
    return;
  }

  // Short list
  if (prefix <= RLP_SHORT_LIST_MAX) {
    size_t payload_len = prefix - RLP_EMPTY_LIST_BYTE;
    size_t new_pos = decoder->pos + 1 + payload_len;
    decoder->pos = new_pos < decoder->size ? new_pos : decoder->size;
    return;
  }

  // Long list
  int len_of_len = prefix - RLP_SHORT_LIST_MAX;
  ++decoder->pos; // consume prefix

  size_t payload_len = 0;
  for (int i = 0; i < len_of_len && decoder->pos < decoder->size; ++i) {
    payload_len = (payload_len << 8) | decoder->input[decoder->pos++];
  }
  size_t new_pos = decoder->pos + payload_len;
  decoder->pos = new_pos < decoder->size ? new_pos : decoder->size;
}

const char *rlp_error_string(rlp_error_t error) {
  switch (error) {
  case RLP_SUCCESS:
    return "success";
  case RLP_ERR_INPUT_TOO_SHORT:
    return "input too short";
  case RLP_ERR_LEADING_ZEROS:
    return "leading zeros in length";
  case RLP_ERR_NON_CANONICAL:
    return "non-canonical encoding";
  case RLP_ERR_INTEGER_OVERFLOW:
    return "integer overflow";
  case RLP_ERR_LIST_MISMATCH:
    return "list length mismatch";
  case RLP_ERR_INVALID_PREFIX:
    return "invalid prefix";
  case RLP_ERR_WRONG_SIZE:
    return "wrong data size";
  default:
    return "unknown error";
  }
}
