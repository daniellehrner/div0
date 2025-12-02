#include "div0/evm/memory.h"

#include <cstring>

#include "div0/utils/bytes.h"

namespace div0::evm {

Memory::Memory(size_t initial_capacity) { data_.reserve(initial_capacity); }

types::Uint256 Memory::load_word_unsafe(uint64_t offset) const noexcept {
  // Read 32 bytes as 4 uint64_t in memory order
  std::array<uint64_t, 4> raw{};
  std::memcpy(raw.data(), &data_[offset], 32);

  // EVM memory is big-endian, Uint256 is little-endian limbs
  return types::Uint256(utils::swap_endian_256(raw));
}

void Memory::store_word_unsafe(uint64_t offset, const types::Uint256& value) noexcept {
  // Convert Uint256 (little-endian limbs) to big-endian bytes
  const auto swapped = utils::swap_endian_256(value.data_unsafe());

  // Write 32 bytes to memory
  std::memcpy(&data_[offset], swapped.data(), 32);
}

void Memory::expand(uint64_t new_size_bytes) {
  const uint64_t aligned_size = word_aligned_size(new_size_bytes);

  if (aligned_size <= size_) {
    return;  // No expansion needed
  }

  // Resize vector to new size, zero-filling new bytes
  // This may throw std::bad_alloc on OOM
  data_.resize(aligned_size, 0);

  size_ = aligned_size;
}

}  // namespace div0::evm
