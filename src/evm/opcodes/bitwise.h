#ifndef DIV0_EVM_OPCODES_BITWISE_IMPL_H
#define DIV0_EVM_OPCODES_BITWISE_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Bitwise Logic Operations
// =============================================================================

/// AND opcode: a & b
static inline evm_status_t op_and(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_and(a, b));
  return EVM_OK;
}

/// OR opcode: a | b
static inline evm_status_t op_or(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_or(a, b));
  return EVM_OK;
}

/// XOR opcode: a ^ b
static inline evm_status_t op_xor(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_xor(a, b));
  return EVM_OK;
}

/// NOT opcode: ~a
static inline evm_status_t op_not(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_not(a));
  return EVM_OK;
}

// =============================================================================
// Byte Extraction
// =============================================================================

/// BYTE opcode: extract byte at index i from value x
/// Stack: [i, x] -> [byte]
static inline evm_status_t op_byte(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t i = evm_stack_pop_unsafe(frame->stack);
  const uint256_t x = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_byte(i, x));
  return EVM_OK;
}

// =============================================================================
// Shift Operations
// =============================================================================

/// SHL opcode: shift left
/// Stack: [shift, value] -> [value << shift]
static inline evm_status_t op_shl(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t shift = evm_stack_pop_unsafe(frame->stack);
  const uint256_t value = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_shl(shift, value));
  return EVM_OK;
}

/// SHR opcode: logical shift right (zero-fill)
/// Stack: [shift, value] -> [value >> shift]
static inline evm_status_t op_shr(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t shift = evm_stack_pop_unsafe(frame->stack);
  const uint256_t value = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_shr(shift, value));
  return EVM_OK;
}

/// SAR opcode: arithmetic shift right (sign-extending)
/// Stack: [shift, value] -> [value >> shift] with sign extension
static inline evm_status_t op_sar(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t shift = evm_stack_pop_unsafe(frame->stack);
  const uint256_t value = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_sar(shift, value));
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_BITWISE_IMPL_H
