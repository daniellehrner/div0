// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-pointer-arithmetic)
// Address uses 3-limb array with indexed access for hash functor.
// Pointer arithmetic for memcpy from span is bounds-safe (fixed 20-byte input).

#ifndef DIV0_TYPES_ADDRESS_H
#define DIV0_TYPES_ADDRESS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "div0/types/uint256.h"

namespace div0::types {

/**
 * @brief EVM contract address (160-bit).
 *
 * Stored as 3 x 64-bit limbs (24 bytes). The upper 32 bits of limb[2] are always zero.
 * Little-endian limb order: limb[0] contains the least significant 64 bits.
 *
 * Use for: contract addresses, caller/origin addresses, msg.sender.
 */
class Address {
 public:
  /// Default constructor: zero address
  constexpr Address() noexcept : limbs_{0, 0, 0} {}

  /// Construct from Uint256 (takes lower 160 bits)
  constexpr explicit Address(const Uint256& value) noexcept
      : limbs_{value.limb(0), value.limb(1), value.limb(2) & 0xFFFFFFFF} {}

  /// Construct from 20 bytes (big-endian, as used in RLP/transactions)
  explicit Address(std::span<const uint8_t, 20> bytes) noexcept : limbs_{} {
    // bytes[0..19] is big-endian: bytes[0] is most significant
    // limb[2] gets bytes[0..3] (upper 32 bits, masked)
    // limb[1] gets bytes[4..11]
    // limb[0] gets bytes[12..19]
    limbs_[2] = (static_cast<uint64_t>(bytes[0]) << 24) | (static_cast<uint64_t>(bytes[1]) << 16) |
                (static_cast<uint64_t>(bytes[2]) << 8) | static_cast<uint64_t>(bytes[3]);
    std::memcpy(&limbs_[1], bytes.data() + 4, 8);
    limbs_[1] = __builtin_bswap64(limbs_[1]);
    std::memcpy(limbs_.data(), bytes.data() + 12, 8);
    limbs_[0] = __builtin_bswap64(limbs_[0]);
  }

  /// Convert to Uint256 (for stack operations)
  [[nodiscard]] Uint256 to_uint256() const noexcept {
    return Uint256(limbs_[0], limbs_[1], limbs_[2], 0);
  }

  /// Check if zero address
  [[nodiscard]] constexpr bool is_zero() const noexcept {
    return limbs_[0] == 0 && limbs_[1] == 0 && limbs_[2] == 0;
  }

  /// Access limb by index (for hashing)
  [[nodiscard]] constexpr uint64_t limb(size_t i) const noexcept { return limbs_[i]; }

  [[nodiscard]] constexpr bool operator==(const Address& other) const noexcept {
    return limbs_[0] == other.limbs_[0] && limbs_[1] == other.limbs_[1] &&
           limbs_[2] == other.limbs_[2];
  }

  [[nodiscard]] constexpr bool operator!=(const Address& other) const noexcept {
    return !(*this == other);
  }

  /// Create zero address
  [[nodiscard]] static constexpr Address zero() noexcept { return Address{}; }

 private:
  std::array<uint64_t, 3> limbs_;  // 24 bytes, little-endian limb order
};

/// Hash functor for Address (for use in unordered_map)
struct AddressHash {
  std::size_t operator()(const Address& addr) const noexcept {
    // Combine all 3 limbs using boost::hash_combine style mixing
    std::size_t h = addr.limb(0);
    h ^= addr.limb(1) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= addr.limb(2) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

}  // namespace div0::types

#endif  // DIV0_TYPES_ADDRESS_H

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-pointer-arithmetic)
