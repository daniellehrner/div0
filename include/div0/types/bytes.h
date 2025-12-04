#ifndef DIV0_TYPES_BYTES_H
#define DIV0_TYPES_BYTES_H

#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

namespace div0::types {

/// Variable-length byte array
using Bytes = std::vector<uint8_t>;

/// Hash functor for Bytes (for use in std::unordered_map/set)
struct BytesHash {
  std::size_t operator()(const Bytes& bytes) const noexcept {
    if (bytes.size() >= sizeof(std::size_t)) {
      std::size_t result = 0;
      std::memcpy(&result, bytes.data(), sizeof(result));
      return result;
    }
    // For small byte arrays, use std::hash on individual bytes
    std::size_t result = 0;
    for (const auto byte : bytes) {
      result = (result * 31) + byte;
    }
    return result;
  }
};

}  // namespace div0::types

/// std::hash specialization for Bytes
template <>
struct std::hash<div0::types::Bytes> {
  std::size_t operator()(const div0::types::Bytes& bytes) const noexcept {
    return div0::types::BytesHash{}(bytes);
  }
};

#endif  // DIV0_TYPES_BYTES_H
