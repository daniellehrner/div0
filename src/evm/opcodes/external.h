#ifndef DIV0_EVM_OPCODES_EXTERNAL_IMPL_H
#define DIV0_EVM_OPCODES_EXTERNAL_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/gas.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/state/state_access.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdint.h>
#include <string.h>

// =============================================================================
// State-Dependent Opcodes (require state access)
// =============================================================================

/// BALANCE opcode (0x31): Get balance of an account
/// Stack: [address] => [balance]
/// Gas: 100 (warm) or 2600 (cold) per EIP-2929
static inline evm_status_t op_balance(call_frame_t *frame, state_access_t *state) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t addr_u256 = evm_stack_pop_unsafe(frame->stack);
  const address_t addr = address_from_uint256(&addr_u256);

  // EIP-2929: Warm/cold access cost
  const bool is_cold = state_warm_address(state, &addr);
  const uint64_t gas_cost = is_cold ? GAS_COLD_ACCOUNT_ACCESS : GAS_WARM_ACCESS;

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t balance = state_get_balance(state, &addr);

  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  evm_stack_push_unsafe(frame->stack, balance);

  return EVM_OK;
}

/// EXTCODESIZE opcode (0x3B): Get code size of external account
/// Stack: [address] => [size]
/// Gas: 100 (warm) or 2600 (cold) per EIP-2929
static inline evm_status_t op_extcodesize(call_frame_t *frame, state_access_t *state) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t addr_u256 = evm_stack_pop_unsafe(frame->stack);
  const address_t addr = address_from_uint256(&addr_u256);

  // EIP-2929: Warm/cold access cost
  const bool is_cold = state_warm_address(state, &addr);
  const uint64_t gas_cost = is_cold ? GAS_COLD_ACCOUNT_ACCESS : GAS_WARM_ACCESS;

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const size_t code_size = state_get_code_size(state, &addr);

  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(code_size));

  return EVM_OK;
}

/// EXTCODECOPY opcode (0x3C): Copy external code to memory
/// Stack: [address, destOffset, srcOffset, size] => []
/// Gas: 100/2600 + 3 * ceil(size/32) + memory_expansion_cost
static inline evm_status_t op_extcodecopy(call_frame_t *frame, state_access_t *state) {
  if (!evm_stack_has_items(frame->stack, 4)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t addr_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t dest_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t src_offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  const address_t addr = address_from_uint256(&addr_u256);

  // EIP-2929: Warm/cold access cost
  const bool is_cold = state_warm_address(state, &addr);
  const uint64_t access_cost = is_cold ? GAS_COLD_ACCOUNT_ACCESS : GAS_WARM_ACCESS;

  // Zero-size copy: only charge access cost
  if (uint256_is_zero(size_u256)) {
    if (frame->gas < access_cost) {
      return EVM_OUT_OF_GAS;
    }
    frame->gas -= access_cost;
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

  // Check for overflow
  if (dest_offset > UINT64_MAX - size) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, dest_offset, size, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Calculate word cost: 3 * ceil(size/32)
  const uint64_t word_cost = GAS_COPY * ((size + 31) / 32);

  // Total cost
  const uint64_t total_cost = access_cost + word_cost + mem_cost;
  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  // Get external code
  const bytes_t code = state_get_code(state, &addr);

  // Get destination pointer
  uint8_t *dest = (uint8_t *)evm_memory_ptr_unsafe(frame->memory, dest_offset);

  // Source offset handling
  uint64_t src_offset = 0;
  bool src_in_bounds = false;
  if (uint256_fits_u64(src_offset_u256)) {
    src_offset = uint256_to_u64_unsafe(src_offset_u256);
    src_in_bounds = src_offset < code.size;
  }

  if (src_in_bounds) {
    // Copy what we can from code
    const size_t bytes_available = code.size - src_offset;
    const size_t bytes_to_copy = bytes_available < size ? bytes_available : size;
    __builtin___memcpy_chk(dest, code.data + src_offset, bytes_to_copy,
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

/// EXTCODEHASH opcode (0x3F): Get code hash of external account
/// Stack: [address] => [hash]
/// Gas: 100 (warm) or 2600 (cold) per EIP-2929
/// Returns 0 for non-existent accounts (per EIP-1052).
static inline evm_status_t op_extcodehash(call_frame_t *frame, state_access_t *state) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }

  const uint256_t addr_u256 = evm_stack_pop_unsafe(frame->stack);
  const address_t addr = address_from_uint256(&addr_u256);

  // EIP-2929: Warm/cold access cost
  const bool is_cold = state_warm_address(state, &addr);
  const uint64_t gas_cost = is_cold ? GAS_COLD_ACCOUNT_ACCESS : GAS_WARM_ACCESS;

  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  // EIP-1052: Return 0 for non-existent accounts
  // Note: Empty accounts (no code, zero balance, zero nonce) return EMPTY_CODE_HASH
  uint256_t result;
  if (!state_account_exists(state, &addr)) {
    result = uint256_zero();
  } else {
    const hash_t code_hash = state_get_code_hash(state, &addr);
    result = uint256_from_bytes_be(code_hash.bytes, HASH_SIZE);
  }

  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  evm_stack_push_unsafe(frame->stack, result);

  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_EXTERNAL_IMPL_H
