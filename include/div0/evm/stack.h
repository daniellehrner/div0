#ifndef DIV0_EVM_STACK_H
#define DIV0_EVM_STACK_H

#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Maximum stack depth per EVM specification.
constexpr uint16_t EVM_STACK_MAX_DEPTH = 1024;

/// Initial stack capacity (slots, not bytes).
constexpr uint16_t EVM_STACK_INITIAL_CAPACITY = 32;

static_assert(EVM_STACK_MAX_DEPTH <= UINT16_MAX, "stack depth must fit in uint16_t");

/// EVM operand stack.
/// Growable LIFO structure backed by arena allocator.
typedef struct {
  uint256_t *items;    // Arena-allocated, growable
  uint16_t capacity;   // Current capacity in slots
  uint16_t top;        // Index of next free slot (equals current size)
  div0_arena_t *arena; // Arena for growth
} evm_stack_t;

/// Initializes a stack with arena backing.
/// @param stack Stack to initialize
/// @param arena Arena for allocations
/// @return true on success, false on allocation failure
[[nodiscard]] static inline bool evm_stack_init(evm_stack_t *stack, div0_arena_t *arena) {
  stack->items =
      (uint256_t *)div0_arena_alloc(arena, EVM_STACK_INITIAL_CAPACITY * sizeof(uint256_t));
  if (!stack->items) {
    return false;
  }
  stack->capacity = EVM_STACK_INITIAL_CAPACITY;
  stack->top = 0;
  stack->arena = arena;
  return true;
}

/// Returns the current number of elements on the stack.
static inline uint16_t evm_stack_size(const evm_stack_t *stack) {
  return stack->top;
}

/// Checks if the stack is empty.
static inline bool evm_stack_is_empty(const evm_stack_t *stack) {
  return stack->top == 0;
}

/// Check if the stack has at least n elements.
/// @param n Number of elements to check for
/// @return true if stack contains at least n elements
static inline bool evm_stack_has_items(const evm_stack_t *stack, uint16_t n) {
  return stack->top >= n;
}

/// Check if the stack has space for n additional elements.
/// @param n Number of elements to check space for
/// @return true if stack can accommodate n more pushes
static inline bool evm_stack_has_space(const evm_stack_t *stack, uint16_t n) {
  return (uint32_t)stack->top + n <= EVM_STACK_MAX_DEPTH;
}

/// Grow stack capacity.
/// @param stack Stack to grow
/// @return true on success, false on allocation failure or max depth reached
[[nodiscard]] static inline bool evm_stack_grow(evm_stack_t *stack) {
  // Check for overflow before doubling
  uint16_t new_capacity =
      stack->capacity >= EVM_STACK_MAX_DEPTH / 2 ? EVM_STACK_MAX_DEPTH : stack->capacity * 2;
  if (new_capacity <= stack->capacity) {
    return false; // Already at max
  }
  uint256_t *new_items = (uint256_t *)div0_arena_realloc(stack->arena, stack->items,
                                                         stack->capacity * sizeof(uint256_t),
                                                         new_capacity * sizeof(uint256_t));
  if (!new_items) {
    return false;
  }
  stack->items = new_items;
  stack->capacity = new_capacity;
  return true;
}

/// Ensure stack has capacity for n additional elements.
/// @param stack Stack to ensure capacity for
/// @param n Number of additional elements needed
/// @return true on success, false on allocation failure or would exceed max depth
[[nodiscard]] static inline bool evm_stack_ensure_space(evm_stack_t *stack, uint16_t n) {
  if (!evm_stack_has_space(stack, n)) {
    return false;
  }
  while (stack->top + n > stack->capacity) {
    if (!evm_stack_grow(stack)) {
      return false;
    }
  }
  return true;
}

/// Pushes a value onto the stack, growing if needed.
/// @param stack Stack to push onto
/// @param value Value to push
/// @return true on success, false on allocation failure or stack overflow
[[nodiscard]] static inline bool evm_stack_push(evm_stack_t *stack, uint256_t value) {
  if (stack->top >= EVM_STACK_MAX_DEPTH) {
    return false; // Stack overflow
  }
  if (stack->top >= stack->capacity) {
    if (!evm_stack_grow(stack)) {
      return false;
    }
  }
  stack->items[stack->top++] = value;
  return true;
}

/// Pushes a value onto the stack without bounds checking.
/// Caller must ensure capacity is available.
static inline void evm_stack_push_unsafe(evm_stack_t *stack, uint256_t value) {
  stack->items[stack->top++] = value;
}

/// Pops a value from the stack and returns it.
/// Caller must ensure stack is not empty.
static inline uint256_t evm_stack_pop_unsafe(evm_stack_t *stack) {
  return stack->items[--stack->top];
}

/// Returns pointer to top of stack.
/// Caller must ensure stack is not empty.
static inline uint256_t *evm_stack_top_unsafe(evm_stack_t *stack) {
  return &stack->items[stack->top - 1];
}

/// Access stack element by depth from top.
/// @param depth 0 = top, 1 = second from top, etc.
/// Caller must ensure depth < size.
static inline uint256_t *evm_stack_at_unsafe(evm_stack_t *stack, uint16_t depth) {
  return &stack->items[stack->top - 1 - depth];
}

/// Peeks at a value on the stack (returns copy).
/// @param depth 0 = top of stack, 1 = second from top, etc.
/// Caller must ensure depth < size.
static inline uint256_t evm_stack_peek_unsafe(const evm_stack_t *stack, uint16_t depth) {
  return stack->items[stack->top - 1 - depth];
}

/// Duplicate stack element and push onto top.
/// @param depth 1 = dup top, 2 = dup second from top, etc. (matches EVM DUPn)
/// Caller must ensure has_items(depth) && has_space(1).
static inline void evm_stack_dup_unsafe(evm_stack_t *stack, uint16_t depth) {
  stack->items[stack->top] = stack->items[stack->top - depth];
  ++stack->top;
}

/// Swap top element with element at depth.
/// @param depth 1 = swap top two, 2 = swap top and third, etc. (matches EVM SWAPn)
/// Caller must ensure has_items(depth + 1).
static inline void evm_stack_swap_unsafe(evm_stack_t *stack, uint16_t depth) {
  uint256_t tmp = stack->items[stack->top - 1];
  stack->items[stack->top - 1] = stack->items[stack->top - 1 - depth];
  stack->items[stack->top - 1 - depth] = tmp;
}

/// Clears the stack (resets to empty state).
static inline void evm_stack_clear(evm_stack_t *stack) {
  stack->top = 0;
}

#endif // DIV0_EVM_STACK_H