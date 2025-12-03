#ifndef DIV0_RLP_HELPERS_H
#define DIV0_RLP_HELPERS_H

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace div0::rlp {

// RLP encoding thresholds
inline constexpr size_t SMALL_PREFIX_BARRIER = 56;
inline constexpr uint8_t EMPTY_STRING_BYTE = 0x80;
inline constexpr uint8_t EMPTY_LIST_BYTE = 0xC0;

// Prefix ranges
inline constexpr uint8_t SINGLE_BYTE_MAX = 0x7F;
inline constexpr uint8_t SHORT_STRING_MAX = 0xB7;
inline constexpr uint8_t LONG_STRING_MAX = 0xBF;
inline constexpr uint8_t SHORT_LIST_MAX = 0xF7;

// Pre-computed lookup table for prefix lengths (256 entries)
// Returns the total header length for a given prefix byte
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,bugprone-branch-clone)
inline constexpr std::array<uint8_t, 256> PREFIX_LENGTH_TABLE = [] {
  std::array<uint8_t, 256> table{};
  for (size_t i = 0; i < 256; ++i) {
    if (i <= SINGLE_BYTE_MAX) {
      table[i] = 0;  // single byte, no header
    } else if (i <= SHORT_STRING_MAX) {
      table[i] = 1;  // short string: 1-byte prefix
    } else if (i <= LONG_STRING_MAX) {
      table[i] = static_cast<uint8_t>(1 + (i - SHORT_STRING_MAX));  // long string
    } else if (i <= SHORT_LIST_MAX) {
      table[i] = 1;  // short list: 1-byte prefix (intentionally same as short string)
    } else {
      table[i] = static_cast<uint8_t>(1 + (i - SHORT_LIST_MAX));  // long list
    }
  }
  return table;
}();
// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,bugprone-branch-clone)

// Single-byte values cache (for values 0-127)
// Used to avoid allocations when decoding single-byte values
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
inline constexpr std::array<uint8_t, 128> SINGLE_BYTES = [] {
  std::array<uint8_t, 128> arr{};
  for (size_t i = 0; i < 128; ++i) {
    arr[i] = static_cast<uint8_t>(i);
  }
  return arr;
}();
// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

/// Get the prefix length for a given prefix byte (O(1) lookup)
[[nodiscard]] constexpr uint8_t prefix_length(uint8_t prefix) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return PREFIX_LENGTH_TABLE[prefix];
}

/// Compute how many bytes are needed to encode a length value (big-endian)
/// For lengths < 56, this returns 0 (length encoded in prefix byte)
/// For lengths >= 56, returns 1-8 depending on the value
[[nodiscard]] constexpr int length_of_length(size_t value) noexcept {
  if (value < SMALL_PREFIX_BARRIER) {
    return 0;
  }
  // Use leading zero count to determine byte length
  // Example: value=256 needs 2 bytes, value=65536 needs 3 bytes
  const int bits = 64 - std::countl_zero(static_cast<uint64_t>(value));
  return (bits + 7) / 8;  // ceiling division
}

/// Compute how many bytes are needed to represent a uint64 value (without leading zeros)
[[nodiscard]] constexpr int byte_length(uint64_t value) noexcept {
  if (value == 0) {
    return 0;
  }
  const int leading_zeros = std::countl_zero(value);
  return 8 - (leading_zeros / 8);
}

/// Check if a prefix byte indicates a string (not a list)
[[nodiscard]] constexpr bool is_string_prefix(uint8_t prefix) noexcept {
  return prefix <= LONG_STRING_MAX;
}

/// Check if a prefix byte indicates a list
[[nodiscard]] constexpr bool is_list_prefix(uint8_t prefix) noexcept {
  return prefix > LONG_STRING_MAX;
}

/// Check if a prefix byte indicates a single byte value (0x00-0x7f)
[[nodiscard]] constexpr bool is_single_byte(uint8_t prefix) noexcept {
  return prefix <= SINGLE_BYTE_MAX;
}

}  // namespace div0::rlp

#endif  // DIV0_RLP_HELPERS_H
