#include "div0/ethereum/transaction/hex.h"

#include <algorithm>

namespace div0::ethereum::hex {

using types::Address;
using types::Bytes;
using types::Hash;
using types::Uint256;

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
constexpr char HEX_CHARS[] = "0123456789abcdef";

// Lookup table for hex character to value (-1 for invalid)
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
constexpr int8_t HEX_VALUES[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0-15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 16-31
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 32-47
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1,  // 48-63  ('0'-'9')
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 64-79  ('A'-'F')
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 80-95
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 96-111 ('a'-'f')
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 112-127
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 128-143
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 144-159
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 160-175
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 176-191
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 192-207
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 208-223
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 224-239
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 240-255
};

}  // namespace

// =============================================================================
// Hex Encoding Functions
// =============================================================================

std::string encode_uint64(uint64_t value) {
  if (value == 0) {
    return "0x0";
  }

  // Max uint64 is 16 hex digits + "0x" prefix
  std::string result;
  result.reserve(18);
  result = "0x";

  // Find first non-zero nibble
  bool started = false;
  for (int i = 60; i >= 0; i -= 4) {
    const auto nibble = static_cast<uint8_t>((value >> i) & 0xF);
    if (nibble != 0 || started) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      result += HEX_CHARS[nibble];
      started = true;
    }
  }

  return result;
}

std::string encode_uint256(const Uint256& value) {
  if (value.is_zero()) {
    return "0x0";
  }

  std::string result;
  result.reserve(66);  // "0x" + 64 hex chars
  result = "0x";

  // Process limbs from most significant to least significant
  bool started = false;
  for (int limb_idx = 3; limb_idx >= 0; --limb_idx) {
    const uint64_t limb = value.limb(static_cast<size_t>(limb_idx));
    for (int i = 60; i >= 0; i -= 4) {
      const auto nibble = static_cast<uint8_t>((limb >> i) & 0xF);
      if (nibble != 0 || started) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        result += HEX_CHARS[nibble];
        started = true;
      }
    }
  }

  return result;
}

std::string encode_bytes(std::span<const uint8_t> bytes) {
  std::string result;
  result.reserve(2 + (bytes.size() * 2));
  result = "0x";

  for (const uint8_t b : bytes) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[b >> 4];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[b & 0xF];
  }

  return result;
}

std::string encode_address(const Address& addr) {
  std::string result;
  result.reserve(42);  // "0x" + 40 hex chars
  result = "0x";

  for (size_t i = 0; i < 20; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[addr[i] >> 4];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[addr[i] & 0xF];
  }

  return result;
}

std::string encode_hash(const Hash& hash) {
  std::string result;
  result.reserve(66);  // "0x" + 64 hex chars
  result = "0x";

  for (size_t i = 0; i < 32; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[hash[i] >> 4];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    result += HEX_CHARS[hash[i] & 0xF];
  }

  return result;
}

std::string encode_storage_slot(const StorageSlot& slot) {
  // Storage slots are always encoded as 64-char hex (with leading zeros)
  const Uint256& value = slot.get();
  std::string result;
  result.reserve(66);  // "0x" + 64 hex chars
  result = "0x";

  // Process limbs from most significant to least significant
  for (int limb_idx = 3; limb_idx >= 0; --limb_idx) {
    const uint64_t limb = value.limb(static_cast<size_t>(limb_idx));
    for (int i = 60; i >= 0; i -= 4) {
      const auto nibble = static_cast<uint8_t>((limb >> i) & 0xF);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      result += HEX_CHARS[nibble];
    }
  }

  return result;
}

// =============================================================================
// Hex Decoding Functions
// =============================================================================

std::optional<uint64_t> decode_uint64(std::string_view hex_str) {
  // Check for "0x" prefix
  if (hex_str.size() < 2 || hex_str[0] != '0' || (hex_str[1] != 'x' && hex_str[1] != 'X')) {
    return std::nullopt;
  }

  hex_str.remove_prefix(2);

  if (hex_str.empty() || hex_str.size() > 16) {
    return std::nullopt;  // Empty or would overflow uint64
  }

  uint64_t result = 0;
  for (const char c : hex_str) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t val = HEX_VALUES[static_cast<uint8_t>(c)];
    if (val < 0) {
      return std::nullopt;  // Invalid hex character
    }
    result = (result << 4) | static_cast<uint64_t>(val);
  }

  return result;
}

std::optional<Uint256> decode_uint256(std::string_view hex_str) {
  // Check for "0x" prefix
  if (hex_str.size() < 2 || hex_str[0] != '0' || (hex_str[1] != 'x' && hex_str[1] != 'X')) {
    return std::nullopt;
  }

  hex_str.remove_prefix(2);

  if (hex_str.empty() || hex_str.size() > 64) {
    return std::nullopt;  // Empty or would overflow Uint256
  }

  // Pad to 64 characters by processing from right
  std::array<uint64_t, 4> limbs{};

  // Process hex string from right to left
  size_t hex_pos = hex_str.size();
  for (int limb_idx = 0; limb_idx < 4 && hex_pos > 0; ++limb_idx) {
    uint64_t limb = 0;
    const size_t chars_for_limb = std::min(hex_pos, size_t{16});
    const size_t start_pos = hex_pos - chars_for_limb;

    for (size_t i = start_pos; i < hex_pos; ++i) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const int8_t val = HEX_VALUES[static_cast<uint8_t>(hex_str[i])];
      if (val < 0) {
        return std::nullopt;  // Invalid hex character
      }
      limb = (limb << 4) | static_cast<uint64_t>(val);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    limbs[static_cast<size_t>(limb_idx)] = limb;
    hex_pos = start_pos;
  }

  return Uint256(limbs);
}

std::optional<Bytes> decode_bytes(std::string_view hex_str) {
  // Check for "0x" prefix
  if (hex_str.size() < 2 || hex_str[0] != '0' || (hex_str[1] != 'x' && hex_str[1] != 'X')) {
    return std::nullopt;
  }

  hex_str.remove_prefix(2);

  // Empty bytes is valid
  if (hex_str.empty()) {
    return Bytes{};
  }

  // Must have even number of hex characters
  if (hex_str.size() % 2 != 0) {
    return std::nullopt;
  }

  Bytes result;
  result.reserve(hex_str.size() / 2);

  for (size_t i = 0; i < hex_str.size(); i += 2) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t hi = HEX_VALUES[static_cast<uint8_t>(hex_str[i])];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t lo = HEX_VALUES[static_cast<uint8_t>(hex_str[i + 1])];
    if (hi < 0 || lo < 0) {
      return std::nullopt;  // Invalid hex character
    }
    result.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }

  return result;
}

std::optional<Address> decode_address(std::string_view hex) {
  // Check for "0x" prefix
  if (hex.size() < 2 || hex[0] != '0' || (hex[1] != 'x' && hex[1] != 'X')) {
    return std::nullopt;
  }

  hex.remove_prefix(2);

  // Address must be exactly 40 hex characters
  if (hex.size() != 40) {
    return std::nullopt;
  }

  std::array<uint8_t, 20> bytes{};
  for (size_t i = 0; i < 20; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t hi = HEX_VALUES[static_cast<uint8_t>(hex[i * 2])];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t lo = HEX_VALUES[static_cast<uint8_t>(hex[(i * 2) + 1])];
    if (hi < 0 || lo < 0) {
      return std::nullopt;  // Invalid hex character
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
  }

  return Address(bytes);
}

std::optional<Hash> decode_hash(std::string_view hex) {
  // Check for "0x" prefix
  if (hex.size() < 2 || hex[0] != '0' || (hex[1] != 'x' && hex[1] != 'X')) {
    return std::nullopt;
  }

  hex.remove_prefix(2);

  // Hash must be exactly 64 hex characters
  if (hex.size() != 64) {
    return std::nullopt;
  }

  std::array<uint8_t, 32> bytes{};
  for (size_t i = 0; i < 32; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t hi = HEX_VALUES[static_cast<uint8_t>(hex[i * 2])];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const int8_t lo = HEX_VALUES[static_cast<uint8_t>(hex[(i * 2) + 1])];
    if (hi < 0 || lo < 0) {
      return std::nullopt;  // Invalid hex character
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
  }

  return Hash(bytes);
}

std::optional<StorageSlot> decode_storage_slot(const std::string_view hex) {
  // Storage slots can be shorter than 64 chars (without leading zeros)
  // Just decode as Uint256 and wrap
  const auto value = decode_uint256(hex);
  if (!value) {
    return std::nullopt;
  }
  return StorageSlot(*value);
}

}  // namespace div0::ethereum::hex
