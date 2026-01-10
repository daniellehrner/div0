#ifndef DIV0_EVM_OPCODES_CONTROL_FLOW_H
#define DIV0_EVM_OPCODES_CONTROL_FLOW_H

#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include "../jumpdest.h"

#include <stdint.h>

// =============================================================================
// Control Flow Operations
// =============================================================================

/// PC opcode (0x58): Push the program counter value.
/// Pushes the position of THIS instruction, not the next one.
/// Stack: [] => [pc]
/// Gas: 2
static inline evm_status_t op_pc(call_frame_t *frame, uint64_t pc_value, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(pc_value));
  return EVM_OK;
}

/// GAS opcode (0x5A): Push remaining gas after deducting this instruction's cost.
/// Stack: [] => [gas]
/// Gas: 2
static inline evm_status_t op_gas(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  // Push remaining gas AFTER deducting the cost
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(frame->gas));
  return EVM_OK;
}

/// JUMPDEST opcode (0x5B): Valid jump destination marker.
/// This is a no-op that just consumes gas. Its presence marks a valid jump target.
/// Stack: [] => []
/// Gas: 1
static inline evm_status_t op_jumpdest(call_frame_t *frame, const uint64_t gas_cost) {
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  return EVM_OK;
}

/// JUMP opcode (0x56): Unconditional jump.
/// Pops destination from stack, validates it's a JUMPDEST, sets PC.
/// Stack: [dest] => []
/// Gas: 8
/// @param bitmap Jumpdest bitmap (must not be nullptr)
static inline evm_status_t op_jump(call_frame_t *frame, const uint8_t *bitmap,
                                   const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t dest_u256 = evm_stack_pop_unsafe(frame->stack);

  // Destination must fit in uint64
  if (!uint256_fits_u64(dest_u256)) {
    return EVM_INVALID_JUMP;
  }

  const uint64_t dest = uint256_to_u64_unsafe(dest_u256);

  // Validate jump destination
  if (!jumpdest_is_valid(bitmap, frame->code_size, dest)) {
    return EVM_INVALID_JUMP;
  }

  frame->pc = dest;
  return EVM_OK;
}

/// JUMPI opcode (0x57): Conditional jump.
/// Pops destination and condition from stack. If condition is non-zero,
/// validates destination and jumps. If zero, continues to next instruction.
/// Stack: [dest, condition] => []
/// Gas: 10
/// @param bitmap Jumpdest bitmap (must not be nullptr)
static inline evm_status_t op_jumpi(call_frame_t *frame, const uint8_t *bitmap,
                                    const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t dest_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t condition = evm_stack_pop_unsafe(frame->stack);

  // If condition is zero, no jump - continue to next instruction
  if (uint256_is_zero(condition)) {
    return EVM_OK;
  }

  // Condition is non-zero: validate and perform jump
  if (!uint256_fits_u64(dest_u256)) {
    return EVM_INVALID_JUMP;
  }

  const uint64_t dest = uint256_to_u64_unsafe(dest_u256);

  if (!jumpdest_is_valid(bitmap, frame->code_size, dest)) {
    return EVM_INVALID_JUMP;
  }

  frame->pc = dest;
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_CONTROL_FLOW_H
