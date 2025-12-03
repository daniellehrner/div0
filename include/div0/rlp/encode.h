#ifndef DIV0_RLP_ENCODE_H
#define DIV0_RLP_ENCODE_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "div0/rlp/helpers.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::rlp {

/// High-performance RLP encoder using two-phase encoding.
///
/// 1. Compute total encoded length using static methods
/// 2. Allocate buffer once
/// 3. Encode into buffer
///
/// Example:
///   size_t len = RlpEncoder::encoded_length(my_bytes);
///   std::vector<uint8_t> buf(len);
///   RlpEncoder encoder(buf);
///   encoder.encode(my_bytes);
class RlpEncoder {
 public:
  // ===========================================================================
  // Phase 1: Length calculation (static methods)
  // ===========================================================================

  /// Compute encoded length for a byte string
  [[nodiscard]] static constexpr size_t encoded_length(
      const std::span<const uint8_t> data) noexcept {
    const size_t len = data.size();

    // Single byte in [0x00, 0x7f] range: encode as itself
    if (len == 1 && data[0] <= SINGLE_BYTE_MAX) {
      return 1;
    }

    // String: prefix + data
    if (len < SMALL_PREFIX_BARRIER) {
      return 1 + len;  // 1-byte prefix
    }

    return 1 + static_cast<size_t>(length_of_length(len)) + len;  // multi-byte length prefix
  }

  /// Compute encoded length for uint64
  [[nodiscard]] static constexpr size_t encoded_length(const uint64_t value) noexcept {
    if (value == 0) {
      return 1;  // 0x80 (empty string)
    }
    if (value < 128) {
      return 1;  // single byte
    }
    const auto bytes = static_cast<size_t>(byte_length(value));
    return 1 + bytes;  // prefix + value bytes
  }

  /// Compute encoded length for Uint256
  [[nodiscard]] static size_t encoded_length(const types::Uint256& value) noexcept;

  /// Compute encoded length for Address (always 21 bytes: 0x94 + 20 bytes)
  [[nodiscard]] static constexpr size_t encoded_length_address() noexcept {
    return 21;  // 0x94 (0x80 + 20) + 20 address bytes
  }

  /// Compute list header length for a given payload size
  [[nodiscard]] static constexpr size_t list_header_length(const size_t payload_length) noexcept {
    if (payload_length < SMALL_PREFIX_BARRIER) {
      return 1;
    }
    return 1 + static_cast<size_t>(length_of_length(payload_length));
  }

  /// Compute total list length (header + payload)
  [[nodiscard]] static constexpr size_t list_length(const size_t payload_length) noexcept {
    return list_header_length(payload_length) + payload_length;
  }

  // ===========================================================================
  // Phase 2: Encoding into pre-allocated buffer
  // ===========================================================================

  /// Construct encoder with output buffer
  explicit RlpEncoder(const std::span<uint8_t> output) noexcept : buf_(output) {}

  /// Encode a byte string
  void encode(std::span<const uint8_t> data);

  /// Encode a uint64 value
  void encode(uint64_t value);

  /// Encode a Uint256 value
  void encode(const types::Uint256& value);

  /// Encode an address (20 bytes)
  void encode_address(const types::Address& addr);

  /// Write list header for given payload length
  void start_list(size_t payload_length);

  /// Get current position in output buffer
  [[nodiscard]] size_t position() const noexcept { return pos_; }

  /// Get remaining space in output buffer
  [[nodiscard]] size_t remaining() const noexcept { return buf_.size() - pos_; }

 private:
  std::span<uint8_t> buf_;
  size_t pos_ = 0;

  /// Write a single byte
  void write_byte(uint8_t b) noexcept { buf_[pos_++] = b; }

  /// Write multiple bytes
  void write_bytes(std::span<const uint8_t> data) noexcept {
    if (!data.empty()) {
      std::memcpy(&buf_[pos_], data.data(), data.size());
      pos_ += data.size();
    }
  }

  /// Write length prefix (for strings >= 56 bytes or lists >= 56 bytes payload)
  void write_length_bytes(size_t len) noexcept;
};

}  // namespace div0::rlp

#endif  // DIV0_RLP_ENCODE_H
