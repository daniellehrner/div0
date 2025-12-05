#ifndef DIV0_ETHEREUM_STORAGE_VALUE_H
#define DIV0_ETHEREUM_STORAGE_VALUE_H

#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EVM storage value.
 *
 * Use for: values read from or written to storage slots.
 */
class StorageValue {
 public:
  constexpr StorageValue() = default;
  constexpr explicit StorageValue(const types::Uint256& value) : value_(value) {}

  [[nodiscard]] constexpr const types::Uint256& get() const noexcept { return value_; }
  [[nodiscard]] constexpr explicit operator const types::Uint256&() const noexcept {
    return value_;
  }

  [[nodiscard]] constexpr bool operator==(const StorageValue& other) const noexcept {
    return value_ == other.value_;
  }

  [[nodiscard]] constexpr bool is_zero() const noexcept { return value_.is_zero(); }

 private:
  types::Uint256 value_;
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_STORAGE_VALUE_H
