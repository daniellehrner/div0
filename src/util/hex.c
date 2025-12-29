#include "div0/util/hex.h"

#include <string.h>

// Lookup table for nibble to hex char conversion
static const char HEX_CHARS[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

// ============================================================================
// Helper to skip 0x prefix
// ============================================================================

static const char *skip_0x_prefix(const char *hex) {
  if (hex[0] == '0' && hex[1] != '\0' && (hex[1] == 'x' || hex[1] == 'X')) {
    return hex + 2;
  }
  return hex;
}

// ============================================================================
// Hex Decoding
// ============================================================================

bool hex_decode(const char *hex, uint8_t *out, size_t out_len) {
  if (hex == nullptr || out == nullptr) {
    return false;
  }

  // Skip optional 0x prefix
  hex = skip_0x_prefix(hex);

  // Validate length
  size_t hex_len = strlen(hex);
  if (hex_len != out_len * 2) {
    return false;
  }

  // Parse hex pairs
  for (size_t i = 0; i < out_len; i++) {
    uint8_t high;
    uint8_t low;

    if (!hex_char_to_nibble(hex[i * 2], &high)) {
      return false;
    }
    if (!hex_char_to_nibble(hex[(i * 2) + 1], &low)) {
      return false;
    }

    out[i] = (uint8_t)((high << 4) | low);
  }

  return true;
}

bool hex_decode_u64(const char *hex, uint64_t *out) {
  if (hex == nullptr || out == nullptr) {
    return false;
  }

  hex = skip_0x_prefix(hex);
  size_t len = strlen(hex);

  // Empty string or too long (max 16 hex digits = 64 bits)
  if (len == 0 || len > 16) {
    return false;
  }

  uint64_t value = 0;
  for (size_t i = 0; i < len; i++) {
    uint8_t nibble;
    if (!hex_char_to_nibble(hex[i], &nibble)) {
      return false;
    }
    value = (value << 4) | nibble;
  }

  *out = value;
  return true;
}

bool hex_decode_uint256(const char *hex, uint256_t *out) {
  if (hex == nullptr || out == nullptr) {
    return false;
  }

  hex = skip_0x_prefix(hex);
  size_t len = strlen(hex);

  // Empty string or too long (max 64 hex digits = 256 bits)
  if (len == 0 || len > 64) {
    return false;
  }

  // Zero-initialize output
  *out = uint256_zero();

  // Parse nibbles from most significant to least significant
  // uint256 is stored in little-endian limb order
  for (size_t i = 0; i < len; i++) {
    uint8_t nibble;
    if (!hex_char_to_nibble(hex[i], &nibble)) {
      return false;
    }

    // Shift left by 4 bits and add nibble
    // We need to shift the entire uint256 left by 4 bits
    uint64_t carry = 0;
    for (size_t limb = 0; limb < 4; limb++) {
      uint64_t new_carry = out->limbs[limb] >> 60;
      out->limbs[limb] = (out->limbs[limb] << 4) | carry;
      carry = new_carry;
    }
    out->limbs[0] |= nibble;
  }

  return true;
}

size_t hex_strlen(const char *hex) {
  if (hex == nullptr) {
    return 0;
  }
  hex = skip_0x_prefix(hex);
  return strlen(hex);
}

// ============================================================================
// Hex Encoding
// ============================================================================

void hex_encode(const uint8_t *data, size_t len, char *out) {
  out[0] = '0';
  out[1] = 'x';

  for (size_t i = 0; i < len; i++) {
    out[2 + (i * 2)] = HEX_CHARS[(data[i] >> 4) & 0x0F];
    out[2 + (i * 2) + 1] = HEX_CHARS[data[i] & 0x0F];
  }

  out[2 + (len * 2)] = '\0';
}

void hex_encode_u64(uint64_t value, char *out) {
  out[0] = '0';
  out[1] = 'x';

  if (value == 0) {
    out[2] = '0';
    out[3] = '\0';
    return;
  }

  // Find the highest non-zero nibble
  int nibbles = 0;
  uint64_t tmp = value;
  while (tmp > 0) {
    nibbles++;
    tmp >>= 4;
  }

  // Write nibbles from most to least significant
  for (int i = 0; i < nibbles; i++) {
    int shift = (nibbles - 1 - i) * 4;
    out[2 + i] = HEX_CHARS[(value >> shift) & 0x0F];
  }

  out[2 + nibbles] = '\0';
}

void hex_encode_uint256(const uint256_t *value, char *out) {
  out[0] = '0';
  out[1] = 'x';

  if (uint256_is_zero(*value)) {
    out[2] = '0';
    out[3] = '\0';
    return;
  }

  // Convert to big-endian bytes
  uint8_t bytes[32];
  uint256_to_bytes_be(*value, bytes);

  // Find first non-zero byte
  size_t start = 0;
  while (start < 32 && bytes[start] == 0) {
    start++;
  }

  // Check if first nibble is zero (for minimal encoding)
  bool skip_first_nibble = (bytes[start] < 0x10);

  size_t pos = 2;
  for (size_t i = start; i < 32; i++) {
    if (i == start && skip_first_nibble) {
      // Only write low nibble for first byte if high nibble is zero
      out[pos++] = HEX_CHARS[bytes[i] & 0x0F];
    } else {
      out[pos++] = HEX_CHARS[(bytes[i] >> 4) & 0x0F];
      out[pos++] = HEX_CHARS[bytes[i] & 0x0F];
    }
  }

  out[pos] = '\0';
}

void hex_encode_uint256_padded(const uint256_t *value, char *out) {
  out[0] = '0';
  out[1] = 'x';

  // Convert to big-endian bytes
  uint8_t bytes[32];
  uint256_to_bytes_be(*value, bytes);

  // Write all 32 bytes (64 hex chars)
  for (size_t i = 0; i < 32; i++) {
    out[2 + (i * 2)] = HEX_CHARS[(bytes[i] >> 4) & 0x0F];
    out[2 + (i * 2) + 1] = HEX_CHARS[bytes[i] & 0x0F];
  }

  out[66] = '\0';
}

void hex_encode_address(const address_t *addr, char *out) {
  out[0] = '0';
  out[1] = 'x';

  // Write all 20 bytes (40 hex chars)
  for (size_t i = 0; i < ADDRESS_SIZE; i++) {
    out[2 + (i * 2)] = HEX_CHARS[(addr->bytes[i] >> 4) & 0x0F];
    out[2 + (i * 2) + 1] = HEX_CHARS[addr->bytes[i] & 0x0F];
  }

  out[42] = '\0';
}

void hex_encode_hash(const hash_t *hash, char *out) {
  out[0] = '0';
  out[1] = 'x';

  // Write all 32 bytes (64 hex chars)
  for (size_t i = 0; i < HASH_SIZE; i++) {
    out[2 + (i * 2)] = HEX_CHARS[(hash->bytes[i] >> 4) & 0x0F];
    out[2 + (i * 2) + 1] = HEX_CHARS[hash->bytes[i] & 0x0F];
  }

  out[66] = '\0';
}
