#ifndef DIV0_TYPES_BYTES32_H
#define DIV0_TYPES_BYTES32_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>

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

 private:
  std::array<uint8_t, 32> data_{};
};

}  // namespace div0::types

#endif  // DIV0_TYPES_BYTES32_H
