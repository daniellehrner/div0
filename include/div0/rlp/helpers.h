#ifndef DIV0_RLP_HELPERS_H
#define DIV0_RLP_HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// RLP encoding threshold for short vs long string/list encoding.
static constexpr size_t RLP_SMALL_PREFIX_BARRIER = 56;

/// Empty string encoding (0x80).
static constexpr uint8_t RLP_EMPTY_STRING_BYTE = 0x80;

/// Empty list encoding (0xC0).
static constexpr uint8_t RLP_EMPTY_LIST_BYTE = 0xC0;

/// Prefix range boundaries.
static constexpr uint8_t RLP_SINGLE_BYTE_MAX = 0x7F;
static constexpr uint8_t RLP_SHORT_STRING_MAX = 0xB7;
static constexpr uint8_t RLP_LONG_STRING_MAX = 0xBF;
static constexpr uint8_t RLP_SHORT_LIST_MAX = 0xF7;

/// Pre-computed lookup table for prefix lengths (256 entries).
/// Returns the total header length for a given prefix byte.
extern const uint8_t RLP_PREFIX_LENGTH_TABLE[256];

/// Get the prefix length for a given prefix byte (O(1) lookup).
/// @param prefix The RLP prefix byte
/// @return Header length in bytes (0 for single-byte values)
static inline uint8_t rlp_prefix_length(uint8_t prefix) {
  return RLP_PREFIX_LENGTH_TABLE[prefix];
}

/// Compute how many bytes are needed to encode a length value (big-endian).
/// For lengths < 56, returns 0 (length encoded in prefix byte).
/// For lengths >= 56, returns 1-8 depending on the value.
/// @param value The length value
/// @return Number of bytes needed for the length encoding
static inline int rlp_length_of_length(size_t value) {
  if (value < RLP_SMALL_PREFIX_BARRIER) {
    return 0;
  }
  int bits = 64 - __builtin_clzll((uint64_t)value);
  return (bits + 7) / 8; // ceiling division
}

/// Compute how many bytes are needed to represent a uint64 value.
/// @param value The uint64 value
/// @return Number of bytes (0 for zero, 1-8 otherwise)
static inline int rlp_byte_length_u64(uint64_t value) {
  if (value == 0) {
    return 0;
  }
  int leading_zeros = __builtin_clzll(value);
  return 8 - (leading_zeros / 8);
}

/// Check if a prefix byte indicates a string (not a list).
/// @param prefix The RLP prefix byte
/// @return true if string/bytes prefix, false if list prefix
static inline bool rlp_is_string_prefix(uint8_t prefix) {
  return prefix <= RLP_LONG_STRING_MAX;
}

/// Check if a prefix byte indicates a list.
/// @param prefix The RLP prefix byte
/// @return true if list prefix, false otherwise
static inline bool rlp_is_list_prefix(uint8_t prefix) {
  return prefix > RLP_LONG_STRING_MAX;
}

/// Check if a prefix byte indicates a single byte value (0x00-0x7f).
/// @param prefix The RLP prefix byte
/// @return true if single-byte value
static inline bool rlp_is_single_byte(uint8_t prefix) {
  return prefix <= RLP_SINGLE_BYTE_MAX;
}

#endif // DIV0_RLP_HELPERS_H
