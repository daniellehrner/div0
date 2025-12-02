#ifndef DIV0_STACK_H
#define DIV0_STACK_H

#include <array>
#include <cassert>

#include "div0/types/uint256.h"

namespace div0::evm {
// NOLINTBEGIN(*-pro-bounds-constant-array-index)

/**
 * @brief LIFO (Last-In-First-Out) stack for EVM execution.
 *
 * This stack implementation provides the core data structure for Ethereum Virtual Machine
 * execution. It follows LIFO semantics where the most recently pushed value is at the "top"
 * and is the first to be popped.
 *
 * INDEXING CONVENTION:
 * - The stack uses depth-based indexing from the top
 * - stack[0] or Top() returns the most recently pushed value (top of stack)
 * - stack[1] returns the second most recent value
 * - stack[n] returns the value at depth n from the top
 *
 * EXAMPLE:
 * @code
 *   Stack s;
 *   s.Push(100);  // Stack: [100]
 *   s.Push(200);  // Stack: [100, 200]  (200 is now top)
 *   s.Push(300);  // Stack: [100, 200, 300]  (300 is now top)
 *
 *   s[0]  // Returns 300 (top - most recent)
 *   s[1]  // Returns 200 (second from top)
 *   s[2]  // Returns 100 (bottom - oldest)
 *
 *   s.Pop();  // Stack: [100, 200]  (removed 300)
 *   s[0]     // Now returns 200 (new top)
 * @endcode
 *
 * PERFORMANCE:
 * - All operations are marked [[gnu::always_inline]] for zero-overhead abstraction
 * - Stack size is fixed at compile time (no dynamic allocation)
 * - Maximum stack depth is 1024 per EVM specification
 *
 * THREAD SAFETY:
 * - This class is NOT thread-safe
 * - Each execution context should have its own Stack instance
 */
class Stack {
 public:
  /// Maximum stack depth as defined by EVM specification
  static constexpr size_t MAX_SIZE = 1024;

  /**
   * @brief Push a value onto the top of the stack.
   *
   * @param value The 256-bit value to push
   * @return true if successful, false if stack overflow (size >= 1024)
   *
   * @note After a successful push, the new value becomes stack[0] (accessible via Top())
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]] [[nodiscard]]
  bool push(const types::Uint256& value) noexcept {
    if (next_free_slot_ptr_ >= MAX_SIZE) [[unlikely]] {
      return false;
    }

    data_[next_free_slot_ptr_++] = value;
    return true;
  }

  /**
   * @brief Remove the top value from the stack.
   *
   * @return true if successful, false if stack underflow (empty stack)
   *
   * @note This does NOT return the popped value; use PopUnsafe() if you need the value
   * @note After pop, what was stack[1] becomes the new stack[0]
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool pop() noexcept {
    if (next_free_slot_ptr_ == 0) [[unlikely]] {
      return false;
    }

    --next_free_slot_ptr_;
    return true;
  }

  /**
   * @brief Remove and return the top value from the stack without bounds checking.
   *
   * @return Reference to the popped value
   *
   * @warning UNSAFE: No bounds checking. Caller MUST ensure stack is not empty.
   * @warning Do not use twice in the same expression (undefined behavior due to unsequenced
   * modifications)
   * @note The returned reference points to data that is no longer "on" the stack
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]]
  types::Uint256& pop_unsafe() noexcept {
    return data_[--next_free_slot_ptr_];
  }

  /**
   * @brief Access a stack element by depth from the top.
   *
   * @param depth Distance from top (0 = top, 1 = second from top, etc.)
   * @return Reference to the value at the specified depth
   *
   * @warning UNSAFE: No bounds checking. Caller MUST ensure depth < Size()
   * @note This provides LIFO access: stack[0] is most recent, stack[Size()-1] is oldest
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]]
  types::Uint256& operator[](const size_t depth) noexcept {
    return data_[next_free_slot_ptr_ - 1 - depth];
  }

  /**
   * @brief Access a stack element by depth from the top (const version).
   *
   * @param depth Distance from top (0 = top, 1 = second from top, etc.)
   * @return Const reference to the value at the specified depth
   *
   * @warning UNSAFE: No bounds checking. Caller MUST ensure depth < Size()
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]]
  const types::Uint256& operator[](const size_t depth) const noexcept {
    return data_[next_free_slot_ptr_ - 1 - depth];
  }

  /**
   * @brief Get reference to the top stack element (most recently pushed).
   *
   * @return Reference to the top value (same as stack[0])
   *
   * @warning UNSAFE: No bounds checking. Caller MUST ensure stack is not empty.
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  types::Uint256& top() noexcept {
    return data_[next_free_slot_ptr_ - 1];
  }

  /**
   * @brief Get const reference to the top stack element.
   *
   * @return Const reference to the top value
   *
   * @warning UNSAFE: No bounds checking. Caller MUST ensure stack is not empty.
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  const types::Uint256& top() const noexcept {
    return data_[next_free_slot_ptr_ - 1];
  }

  /**
   * @brief Duplicate a stack element and push the copy onto the top without bounds checking.
   *
   * Implements EVM DUPn opcodes. Copies the value at the specified depth
   * and pushes it onto the top of the stack.
   *
   * @param depth Depth of element to duplicate (1 = top, 2 = second from top, etc.)
   *
   * @warning UNSAFE: Caller MUST ensure has_items(depth) && has_space(1)
   * @note Depth must be in range [1, Size()] and stack must have space
   * @note EVM DUP1 calls Dup(1) to duplicate the top element
   * @note After Dup(n), stack[0] contains a copy of what was at depth n-1
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]] void dup_unsafe(const size_t depth) noexcept {
    // DUP opcodes should never pass depth=0
    assert(depth != 0 && "Dup depth must be >= 1");
    assert(has_items(depth) && "Not enough items on the stack");
    assert(has_space(1) && "No free slot on the stack");

    data_[next_free_slot_ptr_] = data_[next_free_slot_ptr_ - depth];
    ++next_free_slot_ptr_;
  }

  /**
   * @brief Swap the top stack element with another element at specified depth.
   *
   * Implements EVM SWAPn opcodes. Exchanges the top element (stack[0])
   * with the element at the specified depth.
   *
   * @param depth Depth of element to swap with top (1 = swap top two, 2 = swap top and third, etc.)
   * @return true if successful, false if stack underflow
   *
   * @note Depth must be in range [1, Size()-1]
   * @note EVM SWAP1 calls Swap(1) to swap the top two elements
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool swap(const size_t depth) noexcept {
    // Swap opcodes should never pass depth=0
    assert(depth != 0 && "Swap depth must be >= 1");

    if (depth >= next_free_slot_ptr_) [[unlikely]] {
      return false;
    }

    std::swap(data_[next_free_slot_ptr_ - 1], data_[next_free_slot_ptr_ - 1 - depth]);

    return true;
  }

  /**
   * @brief Get the current number of elements on the stack.
   *
   * @return Number of elements currently on the stack [0, 1024]
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  size_t size() const noexcept {
    return next_free_slot_ptr_;
  }

  /**
   * @brief Check if the stack is empty.
   *
   * @return true if stack has no elements, false otherwise
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool empty() const noexcept {
    return next_free_slot_ptr_ == 0;
  }

  /**
   * @brief Check if the stack has space for N additional elements.
   *
   * @param n Number of elements to check space for
   * @return true if stack can accommodate N more pushes, false otherwise
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool has_space(const size_t n) const noexcept {
    return next_free_slot_ptr_ + n <= MAX_SIZE;
  }

  /**
   * @brief Check if the stack has at least N elements.
   *
   * @param n Number of elements to check for
   * @return true if stack contains at least N elements, false otherwise
   * @note Useful for validating before operations that require N operands
   * @note Time complexity: O(1)
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool has_items(const size_t n) const noexcept {
    return next_free_slot_ptr_ >= n;
  }

  /**
   * @brief Remove all elements from the stack.
   *
   * @note Does not zero memory; simply resets the stack pointer
   * @note Time complexity: O(1)
   */
  [[gnu::always_inline]]
  void clear() noexcept {
    next_free_slot_ptr_ = 0;
  }

 private:
  /// Fixed-size array holding all stack elements
  std::array<types::Uint256, MAX_SIZE> data_;

  /// Index of the next free slot (also equals current stack size)
  /// Valid stack elements are at indices [0, next_free_slot_ptr_)
  /// Top element is at index (next_free_slot_ptr_ - 1)
  size_t next_free_slot_ptr_{0};
};

// NOLINTEND(*-pro-bounds-constant-array-index)
}  // namespace div0::evm

#endif  // DIV0_STACK_H
