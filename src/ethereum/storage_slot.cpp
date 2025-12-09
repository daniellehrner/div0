#include "div0/ethereum/storage_slot.h"

#include "div0/utils/hex.h"

namespace div0::ethereum {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
constexpr char HEX_CHARS[] = "0123456789abcdef";

}  // namespace

std::string encode_storage_slot(const StorageSlot& slot) {
  // Storage slots are always encoded as 64-char hex (with leading zeros)
  const types::Uint256& value = slot.get();
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

std::optional<StorageSlot> decode_storage_slot(const std::string_view hex) {
  // Storage slots can be shorter than 64 chars (without leading zeros)
  // Just decode as Uint256 and wrap
  const auto value = ::div0::hex::decode_uint256(hex);
  if (!value) {
    return std::nullopt;
  }
  return StorageSlot(*value);
}

}  // namespace div0::ethereum
