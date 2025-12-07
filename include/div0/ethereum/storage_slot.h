#ifndef DIV0_ETHEREUM_STORAGE_SLOT_H
#define DIV0_ETHEREUM_STORAGE_SLOT_H

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

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_STORAGE_SLOT_H
