#include "div0/trie/hex_prefix.h"

/// Hex-prefix flag bits in the first nibble.
enum {
  HP_FLAG_ODD = 0x01,  // Bit 0: odd number of nibbles
  HP_FLAG_LEAF = 0x02, // Bit 1: leaf node
};

bytes_t hex_prefix_encode(const nibbles_t *const nibbles, const bool is_leaf,
                          div0_arena_t *const arena) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  const bool is_odd = (nibbles->len % 2) == 1;

  // Calculate encoded length:
  // - 1 byte for prefix (contains flags + optional first nibble if odd)
  // - (nibbles->len / 2) bytes for remaining nibble pairs
  // - If even, we have an extra padding nibble in the first byte
  const size_t encoded_len = 1 + ((nibbles->len + 1) / 2);

  if (!bytes_reserve(&result, encoded_len)) {
    return result; // Empty on allocation failure
  }

  // Build the first byte with flags
  uint8_t flags = 0;
  if (is_leaf) {
    flags |= HP_FLAG_LEAF;
  }
  if (is_odd) {
    flags |= HP_FLAG_ODD;
  }

  if (is_odd) {
    // Odd: flags in high nibble, first nibble in low nibble
    const uint8_t first_byte = (uint8_t)((flags << 4) | nibbles->data[0]);
    bytes_append_byte(&result, first_byte);

    // Remaining nibbles as pairs
    for (size_t i = 1; i < nibbles->len; i += 2) {
      const uint8_t byte = (uint8_t)((nibbles->data[i] << 4) | nibbles->data[i + 1]);
      bytes_append_byte(&result, byte);
    }
  } else {
    // Even: flags in high nibble, zero padding in low nibble
    const uint8_t first_byte = (uint8_t)(flags << 4);
    bytes_append_byte(&result, first_byte);

    // All nibbles as pairs
    for (size_t i = 0; i < nibbles->len; i += 2) {
      const uint8_t byte = (uint8_t)((nibbles->data[i] << 4) | nibbles->data[i + 1]);
      bytes_append_byte(&result, byte);
    }
  }

  return result;
}

hex_prefix_result_t hex_prefix_decode(const uint8_t *const data, const size_t len,
                                      div0_arena_t *const arena) {
  hex_prefix_result_t result = {
      .nibbles = NIBBLES_EMPTY,
      .is_leaf = false,
      .success = false,
  };

  if (data == nullptr || len == 0) {
    return result;
  }

  // Extract flags from first nibble
  const uint8_t first_byte = data[0];
  const uint8_t flags = (first_byte >> 4) & 0x03; // Only bits 0 and 1

  result.is_leaf = (flags & HP_FLAG_LEAF) != 0;
  const bool is_odd = (flags & HP_FLAG_ODD) != 0;

  // Calculate nibble count
  size_t nibble_count;
  if (is_odd) {
    // First byte contains one nibble, rest are pairs
    nibble_count = 1 + ((len - 1) * 2);
  } else {
    // First byte is just flags, rest are pairs
    nibble_count = (len - 1) * 2;
  }

  if (nibble_count == 0) {
    result.nibbles = NIBBLES_EMPTY;
    result.success = true;
    return result;
  }

  // Allocate nibbles
  uint8_t *const nibble_data = div0_arena_alloc(arena, nibble_count);
  if (nibble_data == nullptr) {
    return result;
  }

  size_t nibble_idx = 0;

  if (is_odd) {
    // First nibble from low nibble of first byte
    nibble_data[nibble_idx++] = first_byte & 0x0F;
  }

  // Remaining bytes as nibble pairs
  for (size_t i = 1; i < len; i++) {
    nibble_data[nibble_idx++] = (data[i] >> 4) & 0x0F;
    nibble_data[nibble_idx++] = data[i] & 0x0F;
  }

  result.nibbles.data = nibble_data;
  result.nibbles.len = nibble_count;
  result.success = true;

  return result;
}
