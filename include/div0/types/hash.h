#ifndef DIV0_TYPES_HASH_H
#define DIV0_TYPES_HASH_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "div0/types/uint256.h"

namespace div0::types {

/**
 * @brief 32-byte cryptographic hash (Keccak-256 output).
 *
 * Used for trie node hashes, state roots, transaction hashes, etc.
 * Stored in big-endian order (MSB first) as per Ethereum convention.
 *
 * Provides std::hash specialization for use in unordered containers.
 */
class Hash {
 public:
  static constexpr size_t SIZE = 32;

  /// Default constructor: zero hash
  constexpr Hash() noexcept = default;

  /// Construct from 32 bytes (copied as-is, assumed big-endian)
  explicit Hash(std::span<const uint8_t, 32> bytes) noexcept {
    std::memcpy(data_.data(), bytes.data(), 32);
  }

  /// Construct from std::array
  explicit constexpr Hash(std::array<uint8_t, 32> bytes) noexcept : data_(bytes) {}

  /// Construct from hex string (without 0x prefix)
  [[nodiscard]] static constexpr Hash from_hex(std::string_view hex) noexcept {
    Hash h;
    if (hex.size() != 64) {
      return h;  // Return zero hash for invalid input
    }
    for (size_t i = 0; i < 32; ++i) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const uint8_t hi = hex_digit(hex[i * 2]);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const uint8_t lo = hex_digit(hex[(i * 2) + 1]);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      h.data_[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return h;
  }

  /// Access raw bytes
  [[nodiscard]] constexpr const uint8_t* data() const noexcept { return data_.data(); }
  [[nodiscard]] constexpr uint8_t* data() noexcept { return data_.data(); }

  /// Access as span
  [[nodiscard]] constexpr std::span<const uint8_t, 32> span() const noexcept { return data_; }

  /// Array access (bounds checked by assert in debug builds)
  [[nodiscard]] constexpr uint8_t operator[](size_t i) const noexcept {
    assert(i < SIZE && "Hash index out of bounds");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return data_[i];
  }
  [[nodiscard]] constexpr uint8_t& operator[](size_t i) noexcept {
    assert(i < SIZE && "Hash index out of bounds");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return data_[i];
  }

  /// Size (always 32)
  [[nodiscard]] static constexpr size_t size() noexcept { return SIZE; }

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
  [[nodiscard]] constexpr bool operator==(const Hash& other) const noexcept = default;

  /// Create zero hash
  [[nodiscard]] static constexpr Hash zero() noexcept { return Hash{}; }

  /// Convert to Uint256 (for EVM stack operations)
  /// Hash is big-endian, Uint256 uses little-endian limbs
  [[nodiscard]] Uint256 to_uint256() const noexcept;

 private:
  alignas(32) std::array<uint8_t, 32> data_{};

  /// Convert hex character to nibble value
  [[nodiscard]] static constexpr uint8_t hex_digit(char c) noexcept {
    if (c >= '0' && c <= '9') {
      return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
      return static_cast<uint8_t>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
      return static_cast<uint8_t>(c - 'A' + 10);
    }
    return 0;
  }
};

/// Hash functor for use in std::unordered_map
struct HashHash {
  std::size_t operator()(const Hash& h) const noexcept {
    // Use first 8 bytes as hash (already random for keccak output)
    std::size_t result = 0;
    std::memcpy(&result, h.data(), sizeof(result));
    return result;
  }
};

}  // namespace div0::types

/// std::hash specialization for Hash
template <>
struct std::hash<div0::types::Hash> {
  std::size_t operator()(const div0::types::Hash& h) const noexcept {
    return div0::types::HashHash{}(h);
  }
};

#endif  // DIV0_TYPES_HASH_H
