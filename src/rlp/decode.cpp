#include "div0/rlp/decode.h"

#include <cstring>

#include "div0/utils/bytes.h"

namespace div0::rlp {

DecodeResult<size_t> RlpDecoder::decode_length(int num_bytes) {
  if (remaining() < static_cast<size_t>(num_bytes)) {
    return {{}, DecodeError::InputTooShort, 0};
  }

  // Check for leading zeros (non-canonical)
  if (input_[pos_] == 0) {
    return {{}, DecodeError::LeadingZeros, 0};
  }

  size_t length = 0;
  for (int i = 0; i < num_bytes; ++i) {
    length = (length << 8) | read_byte();
  }

  return {length, DecodeError::Success, static_cast<size_t>(num_bytes)};
}

DecodeResult<std::span<const uint8_t>> RlpDecoder::decode_string_internal() {
  if (!has_more()) {
    return {{}, DecodeError::InputTooShort, 0};
  }

  const size_t start_pos = pos_;
  const uint8_t prefix = read_byte();

  // Single byte [0x00, 0x7f]
  if (prefix <= SINGLE_BYTE_MAX) {
    // Return span pointing to the prefix byte itself
    return {input_.subspan(start_pos, 1), DecodeError::Success, 1};
  }

  // Short string [0x80, 0xb7]: length = prefix - 0x80
  if (prefix <= SHORT_STRING_MAX) {
    const size_t len = prefix - EMPTY_STRING_BYTE;

    if (remaining() < len) {
      return {{}, DecodeError::InputTooShort, 0};
    }

    // Check canonical: single byte in [0x00, 0x7f] should not use string encoding
    if (len == 1 && input_[pos_] <= SINGLE_BYTE_MAX) {
      return {{}, DecodeError::NonCanonical, 0};
    }

    const auto data = input_.subspan(pos_, len);
    pos_ += len;
    return {data, DecodeError::Success, 1 + len};
  }

  // Long string [0xb8, 0xbf]: length of length = prefix - 0xb7
  if (prefix <= LONG_STRING_MAX) {
    const int len_of_len = prefix - SHORT_STRING_MAX;

    auto len_result = decode_length(len_of_len);
    if (!len_result.ok()) {
      return {{}, len_result.error, 0};
    }

    const size_t len = len_result.value;

    // Check canonical: could this have used short string encoding?
    if (len < SMALL_PREFIX_BARRIER) {
      return {{}, DecodeError::NonCanonical, 0};
    }

    if (remaining() < len) {
      return {{}, DecodeError::InputTooShort, 0};
    }

    const auto data = input_.subspan(pos_, len);
    pos_ += len;
    return {data, DecodeError::Success, 1 + static_cast<size_t>(len_of_len) + len};
  }

  // This is a list prefix, not a string
  return {{}, DecodeError::InvalidPrefix, 0};
}

DecodeResult<std::span<const uint8_t>> RlpDecoder::decode_bytes() {
  return decode_string_internal();
}

DecodeResult<uint64_t> RlpDecoder::decode_uint64() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {0, bytes_result.error, 0};
  }

  const auto bytes = bytes_result.value;

  // Empty bytes = 0
  if (bytes.empty()) {
    return {0, DecodeError::Success, bytes_result.bytes_consumed};
  }

  // Check for integer overflow
  if (bytes.size() > 8) {
    return {0, DecodeError::IntegerOverflow, 0};
  }

  // Check for leading zeros (non-canonical for integers)
  if (bytes.size() > 1 && bytes[0] == 0) {
    return {0, DecodeError::LeadingZeros, 0};
  }

  // Decode big-endian
  uint64_t value = 0;
  for (const uint8_t b : bytes) {
    value = (value << 8) | b;
  }

  return {value, DecodeError::Success, bytes_result.bytes_consumed};
}

DecodeResult<types::Uint256> RlpDecoder::decode_uint256() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {{}, bytes_result.error, 0};
  }

  const auto bytes = bytes_result.value;

  // Empty bytes = 0
  if (bytes.empty()) {
    return {types::Uint256{}, DecodeError::Success, bytes_result.bytes_consumed};
  }

  // Check for overflow
  if (bytes.size() > 32) {
    return {{}, DecodeError::IntegerOverflow, 0};
  }

  // Check for leading zeros (non-canonical)
  if (bytes.size() > 1 && bytes[0] == 0) {
    return {{}, DecodeError::LeadingZeros, 0};
  }

  // Pad to 32 bytes (big-endian input, so pad at start)
  std::array<uint64_t, 4> be_limbs{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto* be_bytes = reinterpret_cast<uint8_t*>(be_limbs.data());
  std::memcpy(be_bytes + (32 - bytes.size()), bytes.data(), bytes.size());

  // Convert from big-endian bytes to little-endian limbs
  const auto le_limbs = utils::swap_endian_256(be_limbs);

  return {types::Uint256(le_limbs), DecodeError::Success, bytes_result.bytes_consumed};
}

DecodeResult<types::Address> RlpDecoder::decode_address() {
  auto bytes_result = decode_bytes();
  if (!bytes_result.ok()) {
    return {{}, bytes_result.error, 0};
  }

  const auto bytes = bytes_result.value;

  // Address must be exactly 20 bytes
  if (bytes.size() != 20) {
    return {{}, DecodeError::InvalidPrefix, 0};
  }

  // Create a fixed-size span for Address constructor
  std::array<uint8_t, 20> addr_bytes{};
  std::memcpy(addr_bytes.data(), bytes.data(), 20);

  return {types::Address(std::span<const uint8_t, 20>(addr_bytes)), DecodeError::Success,
          bytes_result.bytes_consumed};
}

DecodeResult<size_t> RlpDecoder::decode_list_header() {
  if (!has_more()) {
    return {0, DecodeError::InputTooShort, 0};
  }

  const uint8_t prefix = read_byte();

  // Check it's actually a list
  if (prefix <= LONG_STRING_MAX) {
    return {0, DecodeError::InvalidPrefix, 0};
  }

  // Short list [0xc0, 0xf7]: payload length = prefix - 0xc0
  if (prefix <= SHORT_LIST_MAX) {
    const size_t payload_len = prefix - EMPTY_LIST_BYTE;
    return {payload_len, DecodeError::Success, 1};
  }

  // Long list [0xf8, 0xff]: length of length = prefix - 0xf7
  const int len_of_len = prefix - SHORT_LIST_MAX;

  auto len_result = decode_length(len_of_len);
  if (!len_result.ok()) {
    return {0, len_result.error, 0};
  }

  const size_t payload_len = len_result.value;

  // Check canonical: could this have used short list encoding?
  if (payload_len < SMALL_PREFIX_BARRIER) {
    return {0, DecodeError::NonCanonical, 0};
  }

  return {payload_len, DecodeError::Success, 1 + static_cast<size_t>(len_of_len)};
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