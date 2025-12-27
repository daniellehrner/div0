#ifndef DIV0_UTIL_HEX_H
#define DIV0_UTIL_HEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Parse a hex string into a byte buffer.
///
/// Handles optional "0x" or "0X" prefix. Each byte requires exactly 2 hex
/// characters. Accepts both uppercase and lowercase hex digits.
///
/// @param hex Input hex string (may have 0x prefix)
/// @param out Output buffer for parsed bytes
/// @param out_len Expected number of output bytes (hex string must have 2x this many chars)
/// @return true if parsing succeeded, false on invalid input
///
/// Error conditions (returns false):
/// - hex is nullptr
/// - out is nullptr
/// - hex string has wrong length (after stripping prefix)
/// - hex string contains non-hex characters
bool hex_decode(const char *hex, uint8_t *out, size_t out_len);

/// Convert a single hex character to its nibble value.
///
/// @param c Hex character ('0'-'9', 'a'-'f', 'A'-'F')
/// @param out Output nibble (0-15)
/// @return true if valid hex character, false otherwise
static inline bool hex_char_to_nibble(char c, uint8_t *out) {
  if (c >= '0' && c <= '9') {
    *out = (uint8_t)(c - '0');
    return true;
  }
  if (c >= 'a' && c <= 'f') {
    *out = (uint8_t)(c - 'a' + 10);
    return true;
  }
  if (c >= 'A' && c <= 'F') {
    *out = (uint8_t)(c - 'A' + 10);
    return true;
  }
  return false;
}

#endif // DIV0_UTIL_HEX_H