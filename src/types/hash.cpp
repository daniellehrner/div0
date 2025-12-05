#include "div0/types/hash.h"

#include <cstring>

#include "div0/utils/bytes.h"

namespace div0::types {

Uint256 Hash::to_uint256() const noexcept {
  // Hash is stored as big-endian bytes
  // Uint256 uses little-endian limbs
  // Read 32 bytes as 4 uint64_t, swap byte order, and reverse limb order
  std::array<uint64_t, 4> raw{};
  std::memcpy(raw.data(), data_.data(), 32);

  const auto swapped = utils::swap_endian_256(raw);
  return Uint256(swapped);
}

}  // namespace div0::types
