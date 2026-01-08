#ifndef DIV0_EVM_OPCODES_CONTEXT_IMPL_H
#define DIV0_EVM_OPCODES_CONTEXT_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/gas.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdint.h>
#include <string.h>

// =============================================================================
// Simple Push Opcodes (no memory, no state)
// =============================================================================

/// ADDRESS opcode (0x30): Push contract address onto stack
/// Stack: [] => [address]
/// Gas: 2
static inline evm_status_t op_address(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, address_to_uint256(&frame->address));
  return EVM_OK;
}

/// CALLER opcode (0x33): Push msg.sender onto stack
/// Stack: [] => [caller]
/// Gas: 2
static inline evm_status_t op_caller(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, address_to_uint256(&frame->caller));
  return EVM_OK;
}

/// CALLVALUE opcode (0x34): Push msg.value onto stack
/// Stack: [] => [value]
/// Gas: 2
static inline evm_status_t op_callvalue(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, frame->value);
  return EVM_OK;
}

/// CALLDATASIZE opcode (0x36): Push calldata size onto stack
/// Stack: [] => [size]
/// Gas: 2
static inline evm_status_t op_calldatasize(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(frame->input_size));
  return EVM_OK;
}

/// CODESIZE opcode (0x38): Push code size onto stack
/// Stack: [] => [size]
/// Gas: 2
static inline evm_status_t op_codesize(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(frame->code_size));
  return EVM_OK;
}

// =============================================================================
// Call Data Operations
// =============================================================================

/// CALLDATALOAD opcode (0x35): Load 32 bytes from calldata onto stack
/// Stack: [offset] => [data]
/// Gas: 3
/// Bytes beyond calldata size are zero-padded.
static inline evm_status_t op_calldataload(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);

  // If offset doesn't fit in u64 or is beyond calldata, result is zero
  if (!uint256_fits_u64(offset_u256)) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return EVM_OK;
  }

  const uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // If completely beyond calldata bounds, result is zero
  if (offset >= frame->input_size) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return EVM_OK;
  }

  // Read up to 32 bytes, zero-padding if needed
  uint8_t buf[32] = {0};
  const size_t available = frame->input_size - offset;
  const size_t to_copy = available < 32 ? available : 32;
  __builtin___memcpy_chk(buf, frame->input + offset, to_copy, sizeof(buf));

  evm_stack_push_unsafe(frame->stack, uint256_from_bytes_be(buf, 32));
  return EVM_OK;
}

/// CALLDATACOPY opcode (0x37): Copy calldata to memory
/// Stack: [destOffset, srcOffset, size] => []
/// Gas: 3 + 3 * ceil(size/32) + memory_expansion_cost
/// Bytes beyond calldata size are zero-padded.
static inline evm_status_t op_calldatacopy(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 3)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t dest_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t src_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Zero-size copy: only charge base gas, no memory expansion
  if (uint256_is_zero(size_u256)) {
    if (frame->gas < gas_cost) {
      return EVM_OUT_OF_GAS;
    }
    frame->gas -= gas_cost;
    return EVM_OK;
  }

  // Size must fit in uint64
  if (!uint256_fits_u64(size_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Destination offset must fit in uint64
  if (!uint256_fits_u64(dest_offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t dest_offset = uint256_to_u64_unsafe(dest_offset_u256);

  // Check for overflow in dest_offset + size
  if (dest_offset > UINT64_MAX - size) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, dest_offset, size, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate word cost: GAS_COPY * ceil(size/32)
  const uint64_t word_cost = GAS_COPY * ((size + 31) / 32);

  // Total cost
  const uint64_t total_cost = gas_cost + word_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  // Get destination pointer
  uint8_t *dest = (uint8_t *)evm_memory_ptr_unsafe(frame->memory, dest_offset);

  // Source offset handling
  uint64_t src_offset = 0;
  bool src_in_bounds = false;
  if (uint256_fits_u64(src_offset_u256)) {
    src_offset = uint256_to_u64_unsafe(src_offset_u256);
    src_in_bounds = src_offset < frame->input_size;
  }

  if (src_in_bounds) {
    // Copy what we can from calldata
    const size_t bytes_available = frame->input_size - src_offset;
    const size_t bytes_to_copy = bytes_available < size ? bytes_available : size;
    __builtin___memcpy_chk(dest, frame->input + src_offset, bytes_to_copy,
                           __builtin_object_size(dest, 0));

    // Zero-fill the rest
    if (bytes_to_copy < size) {
      __builtin___memset_chk(dest + bytes_to_copy, 0, size - bytes_to_copy,
                             __builtin_object_size(dest + bytes_to_copy, 0));
    }
  } else {
    // Source is entirely out of bounds, fill with zeros
    __builtin___memset_chk(dest, 0, size, __builtin_object_size(dest, 0));
  }

  return EVM_OK;
}

/// CODECOPY opcode (0x39): Copy code to memory
/// Stack: [destOffset, srcOffset, size] => []
/// Gas: 3 + 3 * ceil(size/32) + memory_expansion_cost
/// Bytes beyond code size are zero-padded.
static inline evm_status_t op_codecopy(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 3)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t dest_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t src_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Zero-size copy: only charge base gas, no memory expansion
  if (uint256_is_zero(size_u256)) {
    if (frame->gas < gas_cost) {
      return EVM_OUT_OF_GAS;
    }
    frame->gas -= gas_cost;
    return EVM_OK;
  }

  // Size must fit in uint64
  if (!uint256_fits_u64(size_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Destination offset must fit in uint64
  if (!uint256_fits_u64(dest_offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t dest_offset = uint256_to_u64_unsafe(dest_offset_u256);

  // Check for overflow in dest_offset + size
  if (dest_offset > UINT64_MAX - size) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, dest_offset, size, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate word cost: GAS_COPY * ceil(size/32)
  const uint64_t word_cost = GAS_COPY * ((size + 31) / 32);

  // Total cost
  const uint64_t total_cost = gas_cost + word_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  // Get destination pointer
  uint8_t *dest = (uint8_t *)evm_memory_ptr_unsafe(frame->memory, dest_offset);

  // Source offset handling
  uint64_t src_offset = 0;
  bool src_in_bounds = false;
  if (uint256_fits_u64(src_offset_u256)) {
    src_offset = uint256_to_u64_unsafe(src_offset_u256);
    src_in_bounds = src_offset < frame->code_size;
  }

  if (src_in_bounds) {
    // Copy what we can from code
    const size_t bytes_available = frame->code_size - src_offset;
    const size_t bytes_to_copy = bytes_available < size ? bytes_available : size;
    __builtin___memcpy_chk(dest, frame->code + src_offset, bytes_to_copy,
                           __builtin_object_size(dest, 0));

    // Zero-fill the rest
    if (bytes_to_copy < size) {
      __builtin___memset_chk(dest + bytes_to_copy, 0, size - bytes_to_copy,
                             __builtin_object_size(dest + bytes_to_copy, 0));
    }
  } else {
    // Source is entirely out of bounds, fill with zeros
    __builtin___memset_chk(dest, 0, size, __builtin_object_size(dest, 0));
  }

  return EVM_OK;
}

// =============================================================================
// Return Data Operations
// =============================================================================

/// RETURNDATACOPY opcode (0x3E): Copy return data to memory
/// Stack: [destOffset, srcOffset, size] => []
/// Gas: 3 + GAS_COPY * ceil(size/32) + memory_expansion_cost
/// Unlike CALLDATACOPY, this REVERTS on out-of-bounds (no zero-padding).
static inline evm_status_t op_returndatacopy(call_frame_t *frame, const uint64_t gas_cost,
                                             const uint8_t *return_data, size_t return_data_size) {
  if (!evm_stack_has_items(frame->stack, 3)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t dest_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t src_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Zero-size copy: only charge base gas, no memory expansion
  if (uint256_is_zero(size_u256)) {
    if (frame->gas < gas_cost) {
      return EVM_OUT_OF_GAS;
    }
    frame->gas -= gas_cost;
    return EVM_OK;
  }

  // Size must fit in uint64
  if (!uint256_fits_u64(size_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Source offset must fit in uint64
  if (!uint256_fits_u64(src_offset_u256)) {
    // Out of bounds access - exceptional halt
    return EVM_OUT_OF_GAS;
  }
  const uint64_t src_offset = uint256_to_u64_unsafe(src_offset_u256);

  // Check bounds: src_offset + size must not exceed return_data_size
  // This is the key difference from CALLDATACOPY!
  if (src_offset > return_data_size || size > return_data_size - src_offset) {
    // Out of bounds access - exceptional halt
    return EVM_OUT_OF_GAS;
  }

  // Destination offset must fit in uint64
  if (!uint256_fits_u64(dest_offset_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t dest_offset = uint256_to_u64_unsafe(dest_offset_u256);

  // Check for overflow in dest_offset + size
  if (dest_offset > UINT64_MAX - size) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, dest_offset, size, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate word cost: GAS_COPY * ceil(size/32)
  const uint64_t word_cost = GAS_COPY * ((size + 31) / 32);

  // Total cost
  const uint64_t total_cost = gas_cost + word_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  // Copy return data to memory
  uint8_t *dest = (uint8_t *)evm_memory_ptr_unsafe(frame->memory, dest_offset);
  __builtin___memcpy_chk(dest, return_data + src_offset, size, __builtin_object_size(dest, 0));

  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_CONTEXT_IMPL_H
