#include "div0/trie/hex_prefix.h"

namespace div0::trie {

types::Bytes hex_prefix_encode(const Nibbles& nibbles, bool is_leaf) {
  // Flag nibble: bit 1 (0x20) = leaf, bit 0 (0x10) = odd length
  // NOLINTNEXTLINE(readability-math-missing-parentheses)
  auto flag = static_cast<uint8_t>((is_leaf ? 2 : 0) + (nibbles.size() % 2));

  types::Bytes result;

  if (nibbles.size() % 2 == 1) {
    // Odd length: first byte = (flag << 4) | first_nibble
    // NOLINTNEXTLINE(readability-math-missing-parentheses)
    result.reserve(1 + nibbles.size() / 2);
    result.push_back(static_cast<uint8_t>((flag << 4) | nibbles[0]));

    // Remaining nibbles as pairs
    for (size_t i = 1; i + 1 < nibbles.size(); i += 2) {
      result.push_back(static_cast<uint8_t>((nibbles[i] << 4) | nibbles[i + 1]));
    }
  } else {
    // Even length: first byte = (flag << 4) | 0, then nibble pairs
    // NOLINTNEXTLINE(readability-math-missing-parentheses)
    result.reserve(1 + nibbles.size() / 2);
    result.push_back(static_cast<uint8_t>(flag << 4));

    for (size_t i = 0; i + 1 < nibbles.size(); i += 2) {
      result.push_back(static_cast<uint8_t>((nibbles[i] << 4) | nibbles[i + 1]));
    }
  }

  return result;
}

std::pair<Nibbles, bool> hex_prefix_decode(std::span<const uint8_t> encoded) {
  if (encoded.empty()) {
    return {{}, false};
  }

  const uint8_t first = encoded[0];
  auto flag = static_cast<uint8_t>(first >> 4);
  const bool is_leaf = (flag & 0x02) != 0;
  const bool is_odd = (flag & 0x01) != 0;

  Nibbles nibbles;

  if (is_odd) {
    // First byte contains first nibble
    // NOLINTNEXTLINE(readability-math-missing-parentheses)
    nibbles.reserve(1 + (encoded.size() - 1) * 2);
    nibbles.push_back(static_cast<uint8_t>(first & 0x0F));

    for (size_t i = 1; i < encoded.size(); ++i) {
      nibbles.push_back(static_cast<uint8_t>(encoded[i] >> 4));
      nibbles.push_back(static_cast<uint8_t>(encoded[i] & 0x0F));
    }
  } else {
    // First byte is just flags (low nibble is padding zero)
    nibbles.reserve((encoded.size() - 1) * 2);

    for (size_t i = 1; i < encoded.size(); ++i) {
      nibbles.push_back(static_cast<uint8_t>(encoded[i] >> 4));
      nibbles.push_back(static_cast<uint8_t>(encoded[i] & 0x0F));
    }
  }

  return {nibbles, is_leaf};
}

}  // namespace div0::trie
