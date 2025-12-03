#ifndef DIV0_RLP_DECODE_H
#define DIV0_RLP_DECODE_H

#include <cstddef>
#include <cstdint>
#include <span>

#include "div0/rlp/helpers.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::rlp {

/// RLP decode error codes
enum class DecodeError : uint8_t {
  Success,
  InputTooShort,       // Unexpected end of input
  LeadingZeros,        // Non-canonical encoding (leading zeros in length)
  NonCanonical,        // Could have been encoded more efficiently
  IntegerOverflow,     // Integer too large for target type
  ListLengthMismatch,  // List length doesn't match consumed bytes
  InvalidPrefix,       // Invalid RLP prefix byte
};

/// Result of a decode operation
template <typename T>
struct DecodeResult {
  T value{};
  DecodeError error{DecodeError::Success};
  size_t bytes_consumed{0};

  [[nodiscard]] bool ok() const noexcept { return error == DecodeError::Success; }
  explicit operator bool() const noexcept { return ok(); }
};

/// High-performance RLP decoder with zero-copy string decoding.
///
/// The decoder maintains position state and returns spans into the original
/// input buffer when decoding byte strings, avoiding copies.
///
/// Example:
///   RlpDecoder decoder(input);
///   auto bytes_result = decoder.decode_bytes();
///   if (bytes_result.ok()) {
///     // bytes_result.value is a span into input
///   }
class RlpDecoder {
 public:
  /// Construct decoder with input buffer
  explicit RlpDecoder(const std::span<const uint8_t> input) noexcept : input_(input) {}

  // ===========================================================================
  // Decoding methods (zero-copy where possible)
  // ===========================================================================

  /// Decode a byte string, returning a span into the original input (zero-copy)
  [[nodiscard]] DecodeResult<std::span<const uint8_t>> decode_bytes();

  /// Decode a uint64 value
  [[nodiscard]] DecodeResult<uint64_t> decode_uint64();

  /// Decode a Uint256 value
  [[nodiscard]] DecodeResult<types::Uint256> decode_uint256();

  /// Decode an address (expects exactly 20 bytes)
  [[nodiscard]] DecodeResult<types::Address> decode_address();

  /// Decode a list header, returning the payload length
  /// After this, call decode methods for each list item
  [[nodiscard]] DecodeResult<size_t> decode_list_header();

  // ===========================================================================
  // Navigation and state
  // ===========================================================================

  /// Check if there's more data to decode
  [[nodiscard]] bool has_more() const noexcept { return pos_ < input_.size(); }

  /// Get current position in input buffer
  [[nodiscard]] size_t position() const noexcept { return pos_; }

  /// Get remaining bytes in input buffer
  [[nodiscard]] size_t remaining() const noexcept { return input_.size() - pos_; }

  /// Skip the current item (string or list)
  void skip_item();

  /// Peek at the next prefix byte without consuming it
  [[nodiscard]] uint8_t peek() const noexcept { return pos_ < input_.size() ? input_[pos_] : 0; }

  /// Check if next item is a list
  [[nodiscard]] bool next_is_list() const noexcept { return is_list_prefix(peek()); }

  /// Check if next item is a string/bytes
  [[nodiscard]] bool next_is_string() const noexcept { return is_string_prefix(peek()); }

 private:
  std::span<const uint8_t> input_;
  size_t pos_ = 0;

  /// Read a single byte and advance position
  [[nodiscard]] uint8_t read_byte() noexcept { return input_[pos_++]; }

  /// Decode length from multi-byte encoding
  [[nodiscard]] DecodeResult<size_t> decode_length(int num_bytes);

  /// Internal: decode string/bytes with length info
  [[nodiscard]] DecodeResult<std::span<const uint8_t>> decode_string_internal();
};

}  // namespace div0::rlp

#endif  // DIV0_RLP_DECODE_H
