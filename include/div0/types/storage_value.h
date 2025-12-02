#ifndef DIV0_TYPES_STORAGE_VALUE_H
#define DIV0_TYPES_STORAGE_VALUE_H

#include "div0/types/uint256.h"

namespace div0::types {

/**
 * @brief EVM storage value.
 *
 * Use for: values read from or written to storage slots.
 */
class StorageValue {
 public:
  constexpr StorageValue() = default;
  constexpr explicit StorageValue(const Uint256& value) : value_(value) {}

  [[nodiscard]] constexpr const Uint256& get() const noexcept { return value_; }
  [[nodiscard]] constexpr explicit operator const Uint256&() const noexcept { return value_; }

  [[nodiscard]] constexpr bool operator==(const StorageValue& other) const noexcept {
    return value_ == other.value_;
  }

  [[nodiscard]] constexpr bool is_zero() const noexcept { return value_.is_zero(); }

 private:
  Uint256 value_;
};

}  // namespace div0::types

#endif  // DIV0_TYPES_STORAGE_VALUE_H
