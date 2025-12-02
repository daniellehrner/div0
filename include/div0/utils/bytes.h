#ifndef DIV0_UTILS_BYTES_H
#define DIV0_UTILS_BYTES_H

#include <array>
#include <cstdint>
#include <span>

namespace div0::utils {

/**
 * @brief Swap byte order and reverse limb order for 256-bit values.
 *
 * Converts between:
 * - Little-endian limbs (Uint256 internal format) ↔ Big-endian bytes (storage/network)
 *
 * The transformation is symmetric - calling twice returns the original.
 *
 * @param limbs Four 64-bit limbs
 * @return Byte-swapped and reversed limbs
 */
[[nodiscard]] [[gnu::always_inline]] inline std::array<uint64_t, 4> swap_endian_256(
    const std::span<const uint64_t, 4> limbs) noexcept {
  return {__builtin_bswap64(limbs[3]), __builtin_bswap64(limbs[2]), __builtin_bswap64(limbs[1]),
          __builtin_bswap64(limbs[0])};
}

}  // namespace div0::utils

#endif  // DIV0_UTILS_BYTES_H
