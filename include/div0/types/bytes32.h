#ifndef DIV0_TYPES_BYTES32_H
#define DIV0_TYPES_BYTES32_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "div0/types/uint256.h"

namespace div0::types {

/**
 * @brief 32-byte fixed-size array in big-endian order.
 *
 * Used for hashes, signatures, and other cryptographic data where
 * big-endian byte order is standard (network/storage format).
 *
 * Unlike Uint256 which stores limbs in little-endian order for efficient
 * arithmetic, Bytes32 stores bytes in big-endian order (MSB first) for
 * direct use with cryptographic libraries and serialization.
 */
class Bytes32 {
 public:
  /// Default constructor - creates zero value
  constexpr Bytes32() noexcept = default;

  /// Construct from 32 bytes (copied as-is, assumed big-endian)
  explicit Bytes32(const std::span<const uint8_t, 32> bytes) noexcept {
    std::memcpy(data_.data(), bytes.data(), 32);
  }

  /// Construct from std::array
  explicit constexpr Bytes32(const std::array<uint8_t, 32>& bytes) noexcept : data_(bytes) {}

  /// Access raw bytes
  [[nodiscard]] constexpr const uint8_t* data() const noexcept { return data_.data(); }
  [[nodiscard]] constexpr uint8_t* data() noexcept { return data_.data(); }

  /// Access as span
  [[nodiscard]] constexpr std::span<const uint8_t, 32> span() const noexcept { return data_; }

  /// Array access
  [[nodiscard]] constexpr uint8_t operator[](size_t i) const noexcept { return data_[i]; }
  [[nodiscard]] constexpr uint8_t& operator[](size_t i) noexcept { return data_[i]; }

  /// Size (always 32)
  [[nodiscard]] static constexpr size_t size() noexcept { return 32; }

  /// Iterators
  [[nodiscard]] constexpr auto begin() const noexcept { return data_.begin(); }
  [[nodiscard]] constexpr auto end() const noexcept { return data_.end(); }
  [[nodiscard]] constexpr auto begin() noexcept { return data_.begin(); }
  [[nodiscard]] constexpr auto end() noexcept { return data_.end(); }

  /// Check if zero
  [[nodiscard]] constexpr bool is_zero() const noexcept {
    return std::ranges::all_of(data_, [](uint8_t b) { return b == 0; });
  }

  /// Comparison
  [[nodiscard]] constexpr bool operator==(const Bytes32& other) const noexcept {
    return data_ == other.data_;
  }
  [[nodiscard]] constexpr bool operator!=(const Bytes32& other) const noexcept {
    return data_ != other.data_;
  }

  /// Create zero value
  [[nodiscard]] static constexpr Bytes32 zero() noexcept { return Bytes32{}; }

  /// Convert to Uint256 (for stack operations)
  /// Bytes32 is big-endian: data_[0] is MSB, data_[31] is LSB
  /// Uint256 is little-endian limbs: limb(0) is least significant
  [[nodiscard]] Uint256 to_uint256() const noexcept {
    // Convert big-endian bytes to little-endian limbs
    // data_[24..31] -> limb 0 (least significant)
    // data_[16..23] -> limb 1
    // data_[8..15]  -> limb 2
    // data_[0..7]   -> limb 3 (most significant)
    const uint64_t l0 = (static_cast<uint64_t>(data_[24]) << 56) |
                        (static_cast<uint64_t>(data_[25]) << 48) |
                        (static_cast<uint64_t>(data_[26]) << 40) |
                        (static_cast<uint64_t>(data_[27]) << 32) |
                        (static_cast<uint64_t>(data_[28]) << 24) |
                        (static_cast<uint64_t>(data_[29]) << 16) |
                        (static_cast<uint64_t>(data_[30]) << 8) | static_cast<uint64_t>(data_[31]);

    const uint64_t l1 = (static_cast<uint64_t>(data_[16]) << 56) |
                        (static_cast<uint64_t>(data_[17]) << 48) |
                        (static_cast<uint64_t>(data_[18]) << 40) |
                        (static_cast<uint64_t>(data_[19]) << 32) |
                        (static_cast<uint64_t>(data_[20]) << 24) |
                        (static_cast<uint64_t>(data_[21]) << 16) |
                        (static_cast<uint64_t>(data_[22]) << 8) | static_cast<uint64_t>(data_[23]);

    const uint64_t l2 = (static_cast<uint64_t>(data_[8]) << 56) |
                        (static_cast<uint64_t>(data_[9]) << 48) |
                        (static_cast<uint64_t>(data_[10]) << 40) |
                        (static_cast<uint64_t>(data_[11]) << 32) |
                        (static_cast<uint64_t>(data_[12]) << 24) |
                        (static_cast<uint64_t>(data_[13]) << 16) |
                        (static_cast<uint64_t>(data_[14]) << 8) | static_cast<uint64_t>(data_[15]);

    const uint64_t l3 = (static_cast<uint64_t>(data_[0]) << 56) |
                        (static_cast<uint64_t>(data_[1]) << 48) |
                        (static_cast<uint64_t>(data_[2]) << 40) |
                        (static_cast<uint64_t>(data_[3]) << 32) |
                        (static_cast<uint64_t>(data_[4]) << 24) |
                        (static_cast<uint64_t>(data_[5]) << 16) |
                        (static_cast<uint64_t>(data_[6]) << 8) | static_cast<uint64_t>(data_[7]);

    return Uint256(l0, l1, l2, l3);
  }

  /// Create from span of arbitrary size (right-padded with zeros)
  /// If input is smaller than 32 bytes, fills remaining bytes with zeros.
  /// If input is larger, only first 32 bytes are used.
  [[nodiscard]] static Bytes32 from_span_padded(std::span<const uint8_t> data) noexcept {
    Bytes32 result{};
    const size_t copy_size = std::min(data.size(), size_t{32});
    std::memcpy(result.data_.data(), data.data(), copy_size);
    // Remaining bytes are already zero-initialized
    return result;
  }

 private:
  std::array<uint8_t, 32> data_{};
};

}  // namespace div0::types

#endif  // DIV0_TYPES_BYTES32_H
