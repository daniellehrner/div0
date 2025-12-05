#ifndef DIV0_TYPES_ADDRESS_H
#define DIV0_TYPES_ADDRESS_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "div0/types/uint256.h"

namespace div0::types {

/**
 * @brief EVM contract address (160-bit).
 *
 * Stored as 20 bytes in big-endian order (as used in RLP/transactions).
 * bytes_[0] is the most significant byte.
 *
 * Use for: contract addresses, caller/origin addresses, msg.sender.
 */
class Address {
 public:
  static constexpr size_t SIZE = 20;

  /// Default constructor: zero address
  constexpr Address() noexcept = default;

  /// Construct from Uint256 (takes lower 160 bits, stored big-endian)
  constexpr explicit Address(const Uint256& value) noexcept {
    // Extract lower 160 bits from Uint256 (little-endian limbs) to big-endian bytes
    // limb(0) = least significant 64 bits -> bytes[12..19]
    // limb(1) = next 64 bits -> bytes[4..11]
    // limb(2) lower 32 bits -> bytes[0..3]
    const uint64_t l0 = value.limb(0);
    const uint64_t l1 = value.limb(1);
    const auto l2 = static_cast<uint32_t>(value.limb(2));

    bytes_[0] = static_cast<uint8_t>(l2 >> 24);
    bytes_[1] = static_cast<uint8_t>(l2 >> 16);
    bytes_[2] = static_cast<uint8_t>(l2 >> 8);
    bytes_[3] = static_cast<uint8_t>(l2);

    bytes_[4] = static_cast<uint8_t>(l1 >> 56);
    bytes_[5] = static_cast<uint8_t>(l1 >> 48);
    bytes_[6] = static_cast<uint8_t>(l1 >> 40);
    bytes_[7] = static_cast<uint8_t>(l1 >> 32);
    bytes_[8] = static_cast<uint8_t>(l1 >> 24);
    bytes_[9] = static_cast<uint8_t>(l1 >> 16);
    bytes_[10] = static_cast<uint8_t>(l1 >> 8);
    bytes_[11] = static_cast<uint8_t>(l1);

    bytes_[12] = static_cast<uint8_t>(l0 >> 56);
    bytes_[13] = static_cast<uint8_t>(l0 >> 48);
    bytes_[14] = static_cast<uint8_t>(l0 >> 40);
    bytes_[15] = static_cast<uint8_t>(l0 >> 32);
    bytes_[16] = static_cast<uint8_t>(l0 >> 24);
    bytes_[17] = static_cast<uint8_t>(l0 >> 16);
    bytes_[18] = static_cast<uint8_t>(l0 >> 8);
    bytes_[19] = static_cast<uint8_t>(l0);
  }

  /// Construct from 20 bytes (big-endian, as used in RLP/transactions)
  explicit Address(const std::span<const uint8_t, 20> bytes) noexcept {
    std::memcpy(bytes_.data(), bytes.data(), 20);
  }

  /// Construct from std::array
  explicit constexpr Address(const std::array<uint8_t, 20>& bytes) noexcept : bytes_(bytes) {}

  /// Convert to Uint256 (for stack operations)
  [[nodiscard]] Uint256 to_uint256() const noexcept {
    // Convert big-endian bytes back to little-endian limbs
    const uint64_t l0 =
        (static_cast<uint64_t>(bytes_[12]) << 56) | (static_cast<uint64_t>(bytes_[13]) << 48) |
        (static_cast<uint64_t>(bytes_[14]) << 40) | (static_cast<uint64_t>(bytes_[15]) << 32) |
        (static_cast<uint64_t>(bytes_[16]) << 24) | (static_cast<uint64_t>(bytes_[17]) << 16) |
        (static_cast<uint64_t>(bytes_[18]) << 8) | static_cast<uint64_t>(bytes_[19]);

    const uint64_t l1 =
        (static_cast<uint64_t>(bytes_[4]) << 56) | (static_cast<uint64_t>(bytes_[5]) << 48) |
        (static_cast<uint64_t>(bytes_[6]) << 40) | (static_cast<uint64_t>(bytes_[7]) << 32) |
        (static_cast<uint64_t>(bytes_[8]) << 24) | (static_cast<uint64_t>(bytes_[9]) << 16) |
        (static_cast<uint64_t>(bytes_[10]) << 8) | static_cast<uint64_t>(bytes_[11]);

    const uint64_t l2 = (static_cast<uint64_t>(bytes_[0]) << 24) |
                        (static_cast<uint64_t>(bytes_[1]) << 16) |
                        (static_cast<uint64_t>(bytes_[2]) << 8) | static_cast<uint64_t>(bytes_[3]);

    return Uint256(l0, l1, l2, 0);
  }

  /// Check if zero address
  [[nodiscard]] constexpr bool is_zero() const noexcept {
    return std::ranges::all_of(bytes_, [](uint8_t b) { return b == 0; });
  }

  /// Access raw bytes (big-endian)
  [[nodiscard]] constexpr const uint8_t* data() const noexcept { return bytes_.data(); }
  [[nodiscard]] constexpr uint8_t* data() noexcept { return bytes_.data(); }

  /// Access as span
  [[nodiscard]] constexpr std::span<const uint8_t, 20> span() const noexcept { return bytes_; }

  /// Array access (bounds checked by assert in debug builds)
  [[nodiscard]] constexpr uint8_t operator[](const size_t i) const noexcept {
    assert(i < SIZE && "Address index out of bounds");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return bytes_[i];
  }

  [[nodiscard]] constexpr uint8_t& operator[](const size_t i) noexcept {
    assert(i < SIZE && "Address index out of bounds");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return bytes_[i];
  }

  /// Size (always 20)
  [[nodiscard]] static constexpr size_t size() noexcept { return SIZE; }

  /// Iterators
  [[nodiscard]] constexpr auto begin() const noexcept { return bytes_.begin(); }
  [[nodiscard]] constexpr auto end() const noexcept { return bytes_.end(); }
  [[nodiscard]] constexpr auto begin() noexcept { return bytes_.begin(); }
  [[nodiscard]] constexpr auto end() noexcept { return bytes_.end(); }

  [[nodiscard]] constexpr bool operator==(const Address& other) const noexcept = default;

  /// Less-than comparison (lexicographic, for use in std::map)
  [[nodiscard]] constexpr bool operator<(const Address& other) const noexcept {
    return bytes_ < other.bytes_;
  }

  /// Create zero address
  [[nodiscard]] static constexpr Address zero() noexcept { return Address{}; }

 private:
  std::array<uint8_t, 20> bytes_{};  // Big-endian order
};

/// Hash functor for Address (for use in unordered_map)
struct AddressHash {
  std::size_t operator()(const Address& addr) const noexcept {
    // Use first 8 bytes as hash (already random for keccak-derived addresses)
    std::size_t result = 0;
    std::memcpy(&result, addr.data(), sizeof(result));
    return result;
  }
};

}  // namespace div0::types

#endif  // DIV0_TYPES_ADDRESS_H
