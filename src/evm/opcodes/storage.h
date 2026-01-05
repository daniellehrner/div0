#ifndef DIV0_EVM_OPCODES_STORAGE_H
#define DIV0_EVM_OPCODES_STORAGE_H

#include "div0/evm/call_frame.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/state/state_access.h"
#include "div0/types/uint256.h"

/// SLOAD - Load from storage (0x54)
/// Stack: [slot] => [value]
static inline evm_status_t op_sload(call_frame_t *frame, state_access_t *state,
                                    const gas_schedule_t *gas) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }

  uint256_t slot = evm_stack_pop_unsafe(frame->stack);

  // Mark slot as warm and check if it was cold (first access).
  // Returns true if slot was cold, false if already warm.
  bool is_cold = state_warm_slot(state, &frame->address, slot);
  uint64_t gas_cost = gas->sload(is_cold);

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  uint256_t value = state_get_storage(state, &frame->address, slot);

  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  evm_stack_push_unsafe(frame->stack, value);

  return EVM_OK;
}

/// SSTORE - Store to storage (0x55)
/// Stack: [slot, value] => []
static inline evm_status_t op_sstore(call_frame_t *frame, state_access_t *state,
                                     const gas_schedule_t *gas, uint64_t *gas_refund) {
  if (frame->is_static) {
    return EVM_WRITE_PROTECTION;
  }

  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }

  uint256_t slot = evm_stack_pop_unsafe(frame->stack);
  uint256_t new_value = evm_stack_pop_unsafe(frame->stack);

  uint256_t current_value = state_get_storage(state, &frame->address, slot);
  uint256_t original_value = state_get_original_storage(state, &frame->address, slot);

  // Mark slot as warm and check if it was cold (first access).
  // Returns true if slot was cold, false if already warm.
  bool is_cold = state_warm_slot(state, &frame->address, slot);

  uint64_t gas_cost = gas->sstore(is_cold, current_value, original_value, new_value);

  if (frame->gas <= gas->sstore_min_gas) {
    return EVM_OUT_OF_GAS;
  }

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  int64_t refund = gas->sstore_refund(current_value, original_value, new_value);
  if (refund >= 0) {
    *gas_refund += (uint64_t)refund;
  } else {
    uint64_t to_subtract = (uint64_t)(-refund);
    if (*gas_refund >= to_subtract) {
      *gas_refund -= to_subtract;
    } else {
      *gas_refund = 0;
    }
  }

  state_set_storage(state, &frame->address, slot, new_value);

  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_STORAGE_H
