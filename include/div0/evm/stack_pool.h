#ifndef DIV0_EVM_STACK_POOL_H
#define DIV0_EVM_STACK_POOL_H

#include <array>
#include <cassert>

#include "div0/evm/stack.h"

namespace div0::evm {

/// Pool of pre-allocated Stack instances for nested call frames.
/// Each call frame needs its own stack, but allocating on each CALL is expensive.
/// This pool pre-allocates 1024 stacks (max call depth) and rents them out as needed.
class StackPool {
 public:
  static constexpr size_t MAX_DEPTH = 1024;

  StackPool() = default;
  ~StackPool() = default;

  // Non-copyable, non-moveable
  StackPool(const StackPool&) = delete;
  StackPool(StackPool&&) = delete;
  StackPool& operator=(const StackPool&) = delete;
  StackPool& operator=(StackPool&&) = delete;

  /// Rent a stack for a new call frame. Stack is cleared before returning.
  [[nodiscard]] Stack* rent() noexcept {
    assert(depth_ < MAX_DEPTH && "Stack pool exhausted (call depth exceeded 1024)");

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) - depth_ is bounds-checked
    // above
    Stack& stack = pool_[depth_++];
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    stack.clear();
    return &stack;
  }

  /// Return a stack when call frame completes.
  void release() noexcept {
    assert(depth_ > 0 && "Cannot release from empty stack pool");
    --depth_;
  }

  /// Current number of stacks in use
  [[nodiscard]] size_t depth() const noexcept { return depth_; }

 private:
  std::array<Stack, MAX_DEPTH> pool_{};
  size_t depth_{0};
};

}  // namespace div0::evm

#endif  // DIV0_EVM_STACK_POOL_H
