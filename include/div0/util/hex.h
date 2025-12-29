#ifndef DIV0_UTIL_HEX_H
#define DIV0_UTIL_HEX_H

#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Hex Decoding
// ============================================================================

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

/// Parse a hex string to uint64 with variable length.
///
/// Accepts optional "0x" prefix. Parses up to 16 hex digits.
/// Returns false on overflow or invalid characters.
///
/// @param hex Input hex string
/// @param out Output value
/// @return true on success, false on error
bool hex_decode_u64(const char *hex, uint64_t *out);

/// Parse a hex string to uint256 with variable length.
///
/// Accepts optional "0x" prefix. Parses up to 64 hex digits.
///
/// @param hex Input hex string
/// @param out Output value
/// @return true on success, false on error
bool hex_decode_uint256(const char *hex, uint256_t *out);

/// Get the length of a hex string after stripping optional 0x prefix.
///
/// @param hex Input hex string
/// @return Length of hex digits (excluding prefix)
size_t hex_strlen(const char *hex);

// ============================================================================
// Hex Encoding
// ============================================================================

/// Encode a byte buffer to hex string with "0x" prefix.
///
/// @param data Input bytes
/// @param len Number of bytes
/// @param out Output buffer (must be at least 2 + len*2 + 1 bytes)
void hex_encode(const uint8_t *data, size_t len, char *out);

/// Encode uint64 to minimal hex string with "0x" prefix.
///
/// Leading zeros are stripped (e.g., 255 -> "0xff", 0 -> "0x0").
///
/// @param value Value to encode
/// @param out Output buffer (must be at least 19 bytes: "0x" + 16 digits + null)
void hex_encode_u64(uint64_t value, char *out);

/// Encode uint256 to minimal hex string with "0x" prefix.
///
/// Leading zeros are stripped.
///
/// @param value Value to encode
/// @param out Output buffer (must be at least 67 bytes: "0x" + 64 digits + null)
void hex_encode_uint256(const uint256_t *value, char *out);

/// Encode uint256 to zero-padded 64-char hex string with "0x" prefix.
///
/// Always produces exactly 64 hex digits (32 bytes).
///
/// @param value Value to encode
/// @param out Output buffer (must be at least 67 bytes)
void hex_encode_uint256_padded(const uint256_t *value, char *out);

/// Encode address to hex string with "0x" prefix.
///
/// Always produces exactly 40 hex digits (20 bytes).
///
/// @param addr Address to encode
/// @param out Output buffer (must be at least 43 bytes: "0x" + 40 digits + null)
void hex_encode_address(const address_t *addr, char *out);

/// Encode hash to hex string with "0x" prefix.
///
/// Always produces exactly 64 hex digits (32 bytes).
///
/// @param hash Hash to encode
/// @param out Output buffer (must be at least 67 bytes: "0x" + 64 digits + null)
void hex_encode_hash(const hash_t *hash, char *out);

#endif // DIV0_UTIL_HEX_H
