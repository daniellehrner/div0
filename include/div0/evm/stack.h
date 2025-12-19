#ifndef DIV0_EVM_STACK_H
#define DIV0_EVM_STACK_H

#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Maximum stack depth per EVM specification.
#define EVM_STACK_MAX_DEPTH 1024

/// EVM operand stack.
/// LIFO structure with uint256 elements.
/// Use the inline functions below for access.
typedef struct {
  uint256_t items[EVM_STACK_MAX_DEPTH];
  uint16_t top; // Index of next free slot (equals current size)
} evm_stack_t;

/// Initializes a stack to empty state.
static inline void evm_stack_init(evm_stack_t *stack) {
  stack->top = 0;
}

/// Returns the current number of elements on the stack.
static inline uint16_t evm_stack_size(const evm_stack_t *stack) {
  return stack->top;
}

/// Checks if the stack is empty.
static inline bool evm_stack_is_empty(const evm_stack_t *stack) {
  return stack->top == 0;
}

/// Checks if push is possible.
static inline bool evm_stack_can_push(const evm_stack_t *stack) {
  return stack->top < EVM_STACK_MAX_DEPTH;
}

/// Checks if pop is possible.
static inline bool evm_stack_can_pop(const evm_stack_t *stack) {
  return stack->top > 0;
}

/// Pushes a value onto the stack.
/// Caller must ensure stack is not full (use evm_stack_can_push).
static inline void evm_stack_push_unsafe(evm_stack_t *stack, uint256_t value) {
  stack->items[stack->top++] = value;
}

/// Pops a value from the stack and returns it.
/// Caller must ensure stack is not empty (use evm_stack_can_pop).
static inline uint256_t evm_stack_pop_unsafe(evm_stack_t *stack) {
  return stack->items[--stack->top];
}

/// Returns reference to top of stack.
/// Caller must ensure stack is not empty.
static inline uint256_t *evm_stack_top_unsafe(evm_stack_t *stack) {
  return &stack->items[stack->top - 1];
}

/// Peeks at a value on the stack.
/// @param depth 0 = top of stack, 1 = second from top, etc.
/// Caller must ensure depth < size.
static inline uint256_t evm_stack_peek_unsafe(const evm_stack_t *stack, uint16_t depth) {
  return stack->items[stack->top - 1 - depth];
}

/// Clears the stack (resets to empty state).
static inline void evm_stack_clear(evm_stack_t *stack) {
  stack->top = 0;
}

#endif // DIV0_EVM_STACK_H