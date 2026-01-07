#ifndef DIV0_EVM_OPCODES_COMPARISON_IMPL_H
#define DIV0_EVM_OPCODES_COMPARISON_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Unsigned Comparison Operations
// =============================================================================

/// LT opcode: unsigned a < b
/// Note: Stack push is safe because we pop 2 and push 1 (net -1).
static inline evm_status_t op_lt(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_lt(a, b) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

/// GT opcode: unsigned a > b
static inline evm_status_t op_gt(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_gt(a, b) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

/// EQ opcode: a == b
static inline evm_status_t op_eq(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_eq(a, b) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

/// ISZERO opcode: a == 0
/// Note: Stack push is safe because we pop 1 and push 1 (net 0).
static inline evm_status_t op_iszero(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_is_zero(a) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

// =============================================================================
// Signed Comparison Operations
// =============================================================================

/// SLT opcode: signed a < b
static inline evm_status_t op_slt(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_slt(a, b) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

/// SGT opcode: signed a > b
static inline evm_status_t op_sgt(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_sgt(a, b) ? uint256_from_u64(1) : uint256_zero());
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_COMPARISON_IMPL_H
