#ifndef DIV0_EVM_OPCODES_MEMORY_H
#define DIV0_EVM_OPCODES_MEMORY_H

#include "div0/evm/call_frame.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Memory Operations
// =============================================================================

/// MLOAD opcode (0x51): Load 32 bytes from memory
/// Stack: [offset] => [value]
/// Gas: 3 + memory_expansion_cost
static inline evm_status_t op_mload(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);

  if (!uint256_fits_u64(offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // Prevent offset + 32 overflow
  if (offset > UINT64_MAX - 32) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, 32, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Check for base + mem_cost overflow
  if (mem_cost > UINT64_MAX - gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t total_cost = gas_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  uint256_t value = evm_memory_load32_unsafe(frame->memory, offset);
  evm_stack_push_unsafe(frame->stack, value);
  return EVM_OK;
}

/// MSTORE opcode (0x52): Store 32 bytes to memory
/// Stack: [offset, value] => []
/// Gas: 3 + memory_expansion_cost
static inline evm_status_t op_mstore(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t value = evm_stack_pop_unsafe(frame->stack);

  if (!uint256_fits_u64(offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // Prevent offset + 32 overflow
  if (offset > UINT64_MAX - 32) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, 32, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Check for base + mem_cost overflow
  if (mem_cost > UINT64_MAX - gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t total_cost = gas_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  evm_memory_store32_unsafe(frame->memory, offset, value);
  return EVM_OK;
}

/// MSTORE8 opcode (0x53): Store single byte to memory
/// Stack: [offset, value] => []
/// Gas: 3 + memory_expansion_cost
static inline evm_status_t op_mstore8(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t value = evm_stack_pop_unsafe(frame->stack);

  if (!uint256_fits_u64(offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // Prevent offset + 1 overflow
  if (offset > UINT64_MAX - 1) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, 1, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Check for base + mem_cost overflow
  if (mem_cost > UINT64_MAX - gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t total_cost = gas_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  // Store low byte of value
  evm_memory_store8_unsafe(frame->memory, offset, (uint8_t)(value.limbs[0] & 0xFF));
  return EVM_OK;
}

/// MSIZE opcode (0x59): Get size of active memory in bytes
/// Stack: [] => [size]
/// Gas: 2
static inline evm_status_t op_msize(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(evm_memory_size(frame->memory)));
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_MEMORY_H
