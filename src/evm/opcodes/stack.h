#ifndef DIV0_EVM_OPCODES_STACK_IMPL_H
#define DIV0_EVM_OPCODES_STACK_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"

#include <stdint.h>

// =============================================================================
// POP Operation
// =============================================================================

/// POP opcode: Remove the top item from the stack.
/// Stack: [a] => []
static inline evm_status_t op_pop(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  (void)evm_stack_pop_unsafe(frame->stack);
  return EVM_OK;
}

// =============================================================================
// DUP Operations
// =============================================================================

/// DUP opcode: Duplicate the nth stack item and push onto top.
/// @param depth 1 = DUP1 (duplicate top), 2 = DUP2, ..., 16 = DUP16
/// Stack: [a, ..., nth] => [a, ..., nth, a]
static inline evm_status_t op_dup(call_frame_t *frame, const uint16_t depth,
                                  const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, depth)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (!evm_stack_has_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_dup_unsafe(frame->stack, depth);
  return EVM_OK;
}

// =============================================================================
// SWAP Operations
// =============================================================================

/// SWAP opcode: Exchange top stack item with the (n+1)th item.
/// @param depth 1 = SWAP1 (swap top with 2nd), 2 = SWAP2 (swap top with 3rd), etc.
/// Stack: [a, ..., nth+1] => [nth+1, ..., a]
static inline evm_status_t op_swap(call_frame_t *frame, const uint16_t depth,
                                   const uint64_t gas_cost) {
  // SWAP1 needs 2 items (top + 1 below), SWAP16 needs 17 items
  if (!evm_stack_has_items(frame->stack, depth + 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_swap_unsafe(frame->stack, depth);
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_STACK_IMPL_H
