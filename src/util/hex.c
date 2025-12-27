#include "div0/util/hex.h"

#include <string.h>

bool hex_decode(const char *hex, uint8_t *out, size_t out_len) {
  if (hex == nullptr || out == nullptr) {
    return false;
  }

  // Skip optional 0x prefix
  // Note: hex[0] == '0' implies hex[0] != '\0', so hex[1] access is safe
  if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
    hex += 2;
  }

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