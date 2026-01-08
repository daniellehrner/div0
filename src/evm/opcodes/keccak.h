#ifndef DIV0_EVM_OPCODES_KECCAK_H
#define DIV0_EVM_OPCODES_KECCAK_H

#include "div0/crypto/keccak256.h"
#include "div0/evm/call_frame.h"
#include "div0/evm/gas.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Cryptographic Operations
// =============================================================================

/// KECCAK256 opcode (0x20): Compute Keccak-256 hash
/// Stack: [offset, size] => [hash]
/// Gas: 30 + 6 * ceil(size/32) + memory_expansion_cost
static inline evm_status_t op_keccak256(call_frame_t *frame, const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 2)) {
    return EVM_STACK_UNDERFLOW;
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Zero-size hash: keccak256 of empty input (base cost only, no word cost)
  if (uint256_is_zero(size_u256)) {
    if (frame->gas < gas_cost) {
      return EVM_OUT_OF_GAS;
    }
    frame->gas -= gas_cost;
    // keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
    hash_t h = keccak256(nullptr, 0);
    evm_stack_push_unsafe(frame->stack, hash_to_uint256(&h));
    return EVM_OK;
  }

  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Prevent offset + size overflow
  if (offset > UINT64_MAX - size) {
    return EVM_OUT_OF_GAS;
  }

  // Memory expansion
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, size, &mem_cost)) {
    return EVM_OUT_OF_GAS;
  }

  // Word cost: 6 * ceil(size/32)
  // Check for size + 31 overflow
  if (size > UINT64_MAX - 31) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t words = (size + 31) / 32;
  // Check for word_cost overflow
  if (words > UINT64_MAX / GAS_KECCAK256_WORD) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t word_cost = GAS_KECCAK256_WORD * words;

  // Check for base + word_cost overflow
  if (word_cost > UINT64_MAX - gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t dynamic_cost = gas_cost + word_cost;

  // Check for dynamic_cost + mem_cost overflow
  if (mem_cost > UINT64_MAX - dynamic_cost) {
    return EVM_OUT_OF_GAS;
  }
  uint64_t total_cost = dynamic_cost + mem_cost;

  if (frame->gas < total_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= total_cost;

  const uint8_t *data = evm_memory_ptr_unsafe(frame->memory, offset);
  hash_t h = keccak256(data, size);
  evm_stack_push_unsafe(frame->stack, hash_to_uint256(&h));
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_KECCAK_H
