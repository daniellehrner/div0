#ifndef DIV0_ETHEREUM_STORAGE_SLOT_H
#define DIV0_ETHEREUM_STORAGE_SLOT_H

#include <optional>
#include <string>
#include <string_view>

#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EVM storage slot key.
 *
 * Use for: the key/index parameter in SLOAD and SSTORE operations.
 */
class StorageSlot {
 public:
  constexpr StorageSlot() = default;
  constexpr explicit StorageSlot(const types::Uint256& value) : value_(value) {}

  [[nodiscard]] constexpr const types::Uint256& get() const noexcept { return value_; }
  [[nodiscard]] constexpr explicit operator const types::Uint256&() const noexcept {
    return value_;
  }

  [[nodiscard]] constexpr bool operator==(const StorageSlot& other) const noexcept {
    return value_ == other.value_;
  }

  [[nodiscard]] constexpr bool operator<(const StorageSlot& other) const noexcept {
    return value_ < other.value_;
  }

 private:
  types::Uint256 value_;
};

// =============================================================================
// Hex Encoding/Decoding for StorageSlot
// =============================================================================

/**
 * @brief Encode storage slot as "0x" prefixed 64-char lowercase hex.
 * StorageSlot is a wrapper around Uint256, encoded with leading zeros.
 */
[[nodiscard]] std::string encode_storage_slot(const StorageSlot& slot);

/**
 * @brief Decode "0x" prefixed hex string to storage slot.
 * Storage slots can be shorter than 64 chars (without leading zeros).
 * @return nullopt on parse error
 */
[[nodiscard]] std::optional<StorageSlot> decode_storage_slot(std::string_view hex);

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_STORAGE_SLOT_H
