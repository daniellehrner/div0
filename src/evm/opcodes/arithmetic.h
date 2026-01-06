#ifndef DIV0_EVM_OPCODES_ARITHMETIC_IMPL_H
#define DIV0_EVM_OPCODES_ARITHMETIC_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// EXP opcode gas costs (dynamic)
static constexpr uint64_t GAS_EXP_BASE = 10; // EXP base cost
static constexpr uint64_t GAS_EXP_BYTE = 50; // EXP per-byte cost

// =============================================================================
// Binary operations (pop 2, push 1)
// =============================================================================

/// SUB opcode: a - b
static inline evm_status_t op_sub(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_sub(a, b));
  return EVM_OK;
}

/// MUL opcode: a * b
static inline evm_status_t op_mul(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_mul(a, b));
  return EVM_OK;
}

/// DIV opcode: a / b (returns 0 if b is 0)
static inline evm_status_t op_div(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_div(a, b));
  return EVM_OK;
}

/// SDIV opcode: signed a / b (returns 0 if b is 0)
static inline evm_status_t op_sdiv(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_sdiv(a, b));
  return EVM_OK;
}

/// MOD opcode: a % b (returns 0 if b is 0)
static inline evm_status_t op_mod(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_mod(a, b));
  return EVM_OK;
}

/// SMOD opcode: signed a % b (returns 0 if b is 0)
static inline evm_status_t op_smod(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_smod(a, b));
  return EVM_OK;
}

/// SIGNEXTEND opcode: sign extend x at byte position b
static inline evm_status_t op_signextend(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t b = evm_stack_pop_unsafe(frame->stack); // byte position
  const uint256_t x = evm_stack_pop_unsafe(frame->stack); // value
  evm_stack_push_unsafe(frame->stack, uint256_signextend(b, x));
  return EVM_OK;
}

// =============================================================================
// Ternary operations (pop 3, push 1)
// =============================================================================

/// ADDMOD opcode: (a + b) % n (returns 0 if n is 0)
static inline evm_status_t op_addmod(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 3)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  const uint256_t n = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_addmod(a, b, n));
  return EVM_OK;
}

/// MULMOD opcode: (a * b) % n (returns 0 if n is 0)
static inline evm_status_t op_mulmod(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 3)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  const uint256_t a = evm_stack_pop_unsafe(frame->stack);
  const uint256_t b = evm_stack_pop_unsafe(frame->stack);
  const uint256_t n = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_mulmod(a, b, n));
  return EVM_OK;
}

// =============================================================================
// EXP opcode (dynamic gas)
// =============================================================================

/// EXP opcode: base^exponent
/// Gas = 10 + 50 * byte_length(exponent)
static inline evm_status_t op_exp(call_frame_t *frame) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }

  // Peek at exponent to calculate gas (base is at top/depth 0, exponent is at depth 1)
  const uint256_t exponent_peek = evm_stack_peek_unsafe(frame->stack, 1);
  const size_t exp_byte_len = uint256_byte_length(exponent_peek);

  // Calculate total gas cost
  uint64_t gas_cost = GAS_EXP_BASE;
  if (exp_byte_len > 0) {
    // Check for overflow
    if (exp_byte_len > (UINT64_MAX - GAS_EXP_BASE) / GAS_EXP_BYTE) {
      return EVM_OUT_OF_GAS;
    }
    gas_cost += GAS_EXP_BYTE * (uint64_t)exp_byte_len;
  }

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t base = evm_stack_pop_unsafe(frame->stack);
  const uint256_t exponent = evm_stack_pop_unsafe(frame->stack);
  evm_stack_push_unsafe(frame->stack, uint256_exp(base, exponent));
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_ARITHMETIC_IMPL_H
