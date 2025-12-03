#include "div0/rlp/decode.h"

#include <bit>
#include <cstring>

#include "div0/utils/bytes.h"

namespace div0::rlp {

DecodeResult<size_t> RlpDecoder::decode_length(const int num_bytes) {
  if (remaining() < static_cast<size_t>(num_bytes)) {
    return {.value = {}, .error = DecodeError::InputTooShort, .bytes_consumed = 0};
  }

  // Check for leading zeros (non-canonical)
  if (input_[pos_] == 0) {
    return {.value = {}, .error = DecodeError::LeadingZeros, .bytes_consumed = 0};
  }

  size_t length = 0;
  for (int i = 0; i < num_bytes; ++i) {
    length = (length << 8) | read_byte();
  }

  return {.value = length,
          .error = DecodeError::Success,
          .bytes_consumed = static_cast<size_t>(num_bytes)};
}

DecodeResult<std::span<const uint8_t>> RlpDecoder::decode_string_internal() {
  if (!has_more()) {
    return {.value = {}, .error = DecodeError::InputTooShort, .bytes_consumed = 0};
  }

  const size_t start_pos = pos_;
  const uint8_t prefix = read_byte();

  // Single byte [0x00, 0x7f]
  if (prefix <= SINGLE_BYTE_MAX) {
    // Return span pointing to the prefix byte itself
    return {
        .value = input_.subspan(start_pos, 1), .error = DecodeError::Success, .bytes_consumed = 1};
  }

  // Short string [0x80, 0xb7]: length = prefix - 0x80
  if (prefix <= SHORT_STRING_MAX) {
    const size_t len = prefix - EMPTY_STRING_BYTE;

    if (remaining() < len) {
      return {.value = {}, .error = DecodeError::InputTooShort, .bytes_consumed = 0};
    }

    // Check canonical: single byte in [0x00, 0x7f] should not use string encoding
    if (len == 1 && input_[pos_] <= SINGLE_BYTE_MAX) {
      return {.value = {}, .error = DecodeError::NonCanonical, .bytes_consumed = 0};
    }

    const auto data = input_.subspan(pos_, len);
    pos_ += len;
    return {.value = data, .error = DecodeError::Success, .bytes_consumed = 1 + len};
  }

  // Long string [0xb8, 0xbf]: length of length = prefix - 0xb7
  if (prefix <= LONG_STRING_MAX) {
    const int len_of_len = prefix - SHORT_STRING_MAX;

    auto len_result = decode_length(len_of_len);
    if (!len_result.ok()) {
      return {.value = {}, .error = len_result.error, .bytes_consumed = 0};
    }

    const size_t len = len_result.value;

    // Check canonical: could this have used short string encoding?
    if (len < SMALL_PREFIX_BARRIER) {
      return {.value = {}, .error = DecodeError::NonCanonical, .bytes_consumed = 0};
    }

    if (remaining() < len) {
      return {.value = {}, .error = DecodeError::InputTooShort, .bytes_consumed = 0};
    }

    const auto data = input_.subspan(pos_, len);
    pos_ += len;
    return {.value = data,
            .error = DecodeError::Success,
            .bytes_consumed = 1 + static_cast<size_t>(len_of_len) + len};
  }

  // This is a list prefix, not a string
  return {.value = {}, .error = DecodeError::InvalidPrefix, .bytes_consumed = 0};
}

DecodeResult<std::span<const uint8_t>> RlpDecoder::decode_bytes() {
  return decode_string_internal();
}

DecodeResult<uint64_t> RlpDecoder::decode_uint64() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {.value = 0, .error = bytes_result.error, .bytes_consumed = 0};
  }

  const auto bytes = bytes_result.value;

  // Empty bytes = 0
  if (bytes.empty()) {
    return {
        .value = 0, .error = DecodeError::Success, .bytes_consumed = bytes_result.bytes_consumed};
  }

  // Check for integer overflow
  if (bytes.size() > 8) {
    return {.value = 0, .error = DecodeError::IntegerOverflow, .bytes_consumed = 0};
  }

  // Check for leading zeros (non-canonical for integers)
  if (bytes.size() > 1 && bytes[0] == 0) {
    return {.value = 0, .error = DecodeError::LeadingZeros, .bytes_consumed = 0};
  }

  // Decode big-endian
  uint64_t value = 0;
  for (const uint8_t b : bytes) {
    value = (value << 8) | b;
  }

  return {
      .value = value, .error = DecodeError::Success, .bytes_consumed = bytes_result.bytes_consumed};
}

DecodeResult<types::Uint256> RlpDecoder::decode_uint256() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {.value = {}, .error = bytes_result.error, .bytes_consumed = 0};
  }

  const auto bytes = bytes_result.value;

  // Empty bytes = 0
  if (bytes.empty()) {
    return {.value = types::Uint256{},
            .error = DecodeError::Success,
            .bytes_consumed = bytes_result.bytes_consumed};
  }

  // Check for overflow
  if (bytes.size() > 32) {
    return {.value = {}, .error = DecodeError::IntegerOverflow, .bytes_consumed = 0};
  }

  // Check for leading zeros (non-canonical)
  if (bytes.size() > 1 && bytes[0] == 0) {
    return {.value = {}, .error = DecodeError::LeadingZeros, .bytes_consumed = 0};
  }

  // Pad to 32 bytes (big-endian input, so pad at start)
  std::array<uint8_t, 32> be_bytes{};
  const size_t offset = 32 - bytes.size();
  for (size_t i = 0; i < bytes.size(); ++i) {
    be_bytes.at(offset + i) = bytes[i];
  }

  // Convert from big-endian bytes to little-endian limbs
  const auto be_limbs = std::bit_cast<std::array<uint64_t, 4>>(be_bytes);
  const auto le_limbs = utils::swap_endian_256(be_limbs);

  return {.value = types::Uint256(le_limbs),
          .error = DecodeError::Success,
          .bytes_consumed = bytes_result.bytes_consumed};
}

DecodeResult<types::Address> RlpDecoder::decode_address() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {.value = {}, .error = bytes_result.error, .bytes_consumed = 0};
  }

  const auto bytes = bytes_result.value;

  // Address must be exactly 20 bytes
  if (bytes.size() != 20) {
    return {.value = {}, .error = DecodeError::InvalidPrefix, .bytes_consumed = 0};
  }

  // Create a fixed-size span for Address constructor
  std::array<uint8_t, 20> addr_bytes{};
  std::memcpy(addr_bytes.data(), bytes.data(), 20);

  return {.value = types::Address(std::span<const uint8_t, 20>(addr_bytes)),
          .error = DecodeError::Success,
          .bytes_consumed = bytes_result.bytes_consumed};
}

DecodeResult<size_t> RlpDecoder::decode_list_header() {
  if (!has_more()) {
    return {.value = 0, .error = DecodeError::InputTooShort, .bytes_consumed = 0};
  }

  const uint8_t prefix = read_byte();

  // Check it's actually a list
  if (prefix <= LONG_STRING_MAX) {
    return {.value = 0, .error = DecodeError::InvalidPrefix, .bytes_consumed = 0};
  }

  // Short list [0xc0, 0xf7]: payload length = prefix - 0xc0
  if (prefix <= SHORT_LIST_MAX) {
    const size_t payload_len = prefix - EMPTY_LIST_BYTE;
    return {.value = payload_len, .error = DecodeError::Success, .bytes_consumed = 1};
  }

  // Long list [0xf8, 0xff]: length of length = prefix - 0xf7
  const int len_of_len = prefix - SHORT_LIST_MAX;

  auto len_result = decode_length(len_of_len);
  if (!len_result.ok()) {
    return {.value = 0, .error = len_result.error, .bytes_consumed = 0};
  }

  const size_t payload_len = len_result.value;

  // Check canonical: could this have used short list encoding?
  if (payload_len < SMALL_PREFIX_BARRIER) {
    return {.value = 0, .error = DecodeError::NonCanonical, .bytes_consumed = 0};
  }

  return {.value = payload_len,
          .error = DecodeError::Success,
          .bytes_consumed = 1 + static_cast<size_t>(len_of_len)};
}

void RlpDecoder::skip_item() {
  if (!has_more()) {
    return;
  }

  const uint8_t prefix = input_[pos_];

  // Single byte
  if (prefix <= SINGLE_BYTE_MAX) {
    ++pos_;
    return;
  }

  // Short string
  if (prefix <= SHORT_STRING_MAX) {
    const size_t len = prefix - EMPTY_STRING_BYTE;
    pos_ += 1 + len;
    return;
  }

  // Long string
  if (prefix <= LONG_STRING_MAX) {
    const int len_of_len = prefix - SHORT_STRING_MAX;
    ++pos_;  // consume prefix

    size_t len = 0;
    for (int i = 0; i < len_of_len && pos_ < input_.size(); ++i) {
      len = (len << 8) | input_[pos_++];
    }
    pos_ += len;
    return;
  }

  // Short list
  if (prefix <= SHORT_LIST_MAX) {
    const size_t payload_len = prefix - EMPTY_LIST_BYTE;
    pos_ += 1 + payload_len;
    return;
  }

  // Long list
  const int len_of_len = prefix - SHORT_LIST_MAX;
  ++pos_;  // consume prefix

  size_t payload_len = 0;
  for (int i = 0; i < len_of_len && pos_ < input_.size(); ++i) {
    payload_len = (payload_len << 8) | input_[pos_++];
  }
  pos_ += payload_len;
}

}  // namespace div0::rlp
