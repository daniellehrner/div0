#ifndef DIV0_EVM_MEMORY_POOL_H
#define DIV0_EVM_MEMORY_POOL_H

#include <array>
#include <cassert>

#include "div0/evm/memory.h"

namespace div0::evm {

/// Pool of pre-allocated Memory instances for nested call frames.
/// Each call frame needs its own memory. This pool pre-allocates 1024 memories
/// (max call depth) and rents them out as needed.
///
/// Memory instances retain their internal allocation after clear() - only the
/// logical size resets. This means the pool "warms up" over time: once a Memory
/// at depth N has been used for a large contract, subsequent calls reuse that
/// allocation without re-allocating.
class MemoryPool {
 public:
  static constexpr size_t MAX_DEPTH = 1024;

  MemoryPool() = default;
  ~MemoryPool() = default;

  // Non-copyable, non-moveable
  MemoryPool(const MemoryPool&) = delete;
  MemoryPool(MemoryPool&&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;
  MemoryPool& operator=(MemoryPool&&) = delete;

  /// Rent a memory instance for a new call frame. Memory is cleared (size=0)
  /// but retains its underlying allocation.
  [[nodiscard]] Memory* rent() noexcept {
    assert(depth_ < MAX_DEPTH && "Memory pool exhausted (call depth exceeded 1024)");

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) - depth_ is bounds-checked
    // above
    Memory& memory = pool_[depth_++];
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    memory.clear();
    return &memory;
  }

  /// Return a memory instance when call frame completes.
  void release() noexcept {
    assert(depth_ > 0 && "Cannot release from empty memory pool");
    --depth_;
  }

  /// Current number of memories in use
  [[nodiscard]] size_t depth() const noexcept { return depth_; }

 private:
  std::array<Memory, MAX_DEPTH> pool_{};
  size_t depth_{0};
};

}  // namespace div0::evm

#endif  // DIV0_EVM_MEMORY_POOL_H
