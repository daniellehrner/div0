#include "div0/trie/nibbles.h"

#include <algorithm>

namespace div0::trie {

Nibbles bytes_to_nibbles(std::span<const uint8_t> bytes) {
  Nibbles result;
  result.reserve(bytes.size() * 2);
  for (const uint8_t byte : bytes) {
    result.push_back(static_cast<uint8_t>(byte >> 4));
    result.push_back(static_cast<uint8_t>(byte & 0x0F));
  }
  return result;
}

types::Bytes nibbles_to_bytes(const Nibbles& nibbles) {
  types::Bytes result;
  result.reserve(nibbles.size() / 2);
  for (size_t i = 0; i + 1 < nibbles.size(); i += 2) {
    result.push_back(static_cast<uint8_t>((nibbles[i] << 4) | nibbles[i + 1]));
  }
  return result;
}

size_t common_prefix_length(const Nibbles& a, const Nibbles& b) {
  const size_t min_len = std::min(a.size(), b.size());
  for (size_t i = 0; i < min_len; ++i) {
    if (a[i] != b[i]) {
      return i;
    }
  }
  return min_len;
}

Nibbles nibbles_slice(const Nibbles& nibbles, size_t start, size_t len) {
  if (start >= nibbles.size()) {
    return {};
  }
  const size_t actual_len = std::min(len, nibbles.size() - start);
  return {nibbles.begin() + static_cast<ptrdiff_t>(start),
          nibbles.begin() + static_cast<ptrdiff_t>(start + actual_len)};
}

}  // namespace div0::trie
