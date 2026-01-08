#ifndef DIV0_EVM_OPCODES_BLOCK_IMPL_H
#define DIV0_EVM_OPCODES_BLOCK_IMPL_H

#include "div0/evm/block_context.h"
#include "div0/evm/call_frame.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/evm/tx_context.h"
#include "div0/state/state_access.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Simple Block Information Opcodes
// =============================================================================

/// COINBASE opcode (0x41): Push block coinbase address onto stack
/// Stack: [] => [coinbase]
/// Gas: 2
static inline evm_status_t op_coinbase(call_frame_t *frame, const block_context_t *block,
                                       const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, address_to_uint256(&block->coinbase));
  return EVM_OK;
}

/// TIMESTAMP opcode (0x42): Push block timestamp onto stack
/// Stack: [] => [timestamp]
/// Gas: 2
static inline evm_status_t op_timestamp(call_frame_t *frame, const block_context_t *block,
                                        const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(block->timestamp));
  return EVM_OK;
}

/// NUMBER opcode (0x43): Push block number onto stack
/// Stack: [] => [block_number]
/// Gas: 2
static inline evm_status_t op_number(call_frame_t *frame, const block_context_t *block,
                                     const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(block->number));
  return EVM_OK;
}

/// PREVRANDAO opcode (0x44): Push previous RANDAO value onto stack
/// Stack: [] => [prevrandao]
/// Gas: 2
/// Note: This was DIFFICULTY pre-merge
static inline evm_status_t op_prevrandao(call_frame_t *frame, const block_context_t *block,
                                         const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, block->prev_randao);
  return EVM_OK;
}

/// GASLIMIT opcode (0x45): Push block gas limit onto stack
/// Stack: [] => [gas_limit]
/// Gas: 2
static inline evm_status_t op_gaslimit(call_frame_t *frame, const block_context_t *block,
                                       const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(block->gas_limit));
  return EVM_OK;
}

/// CHAINID opcode (0x46): Push chain ID onto stack
/// Stack: [] => [chain_id]
/// Gas: 2
static inline evm_status_t op_chainid(call_frame_t *frame, const block_context_t *block,
                                      const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(block->chain_id));
  return EVM_OK;
}

/// BASEFEE opcode (0x48): Push EIP-1559 base fee onto stack
/// Stack: [] => [base_fee]
/// Gas: 2
static inline evm_status_t op_basefee(call_frame_t *frame, const block_context_t *block,
                                      const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, block->base_fee);
  return EVM_OK;
}

/// BLOBBASEFEE opcode (0x4A): Push EIP-4844 blob base fee onto stack
/// Stack: [] => [blob_base_fee]
/// Gas: 2
static inline evm_status_t op_blobbasefee(call_frame_t *frame, const block_context_t *block,
                                          const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, block->blob_base_fee);
  return EVM_OK;
}

// =============================================================================
// Special Block Information Opcodes
// =============================================================================

/// BLOCKHASH opcode (0x40): Get hash of a recent block
/// Stack: [block_number] => [hash]
/// Gas: 20
/// Returns zero hash if:
/// - Block number is out of range [current-256, current-1]
/// - Callback is nullptr
/// - Callback returns failure
static inline evm_status_t op_blockhash(call_frame_t *frame, const block_context_t *block,
                                        const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t block_num_u256 = evm_stack_pop_unsafe(frame->stack);

  // Default to zero hash
  uint256_t result = uint256_zero();

  // Check if block number fits in u64 and is in valid range [current-256, current-1]
  if (uint256_fits_u64(block_num_u256)) {
    const uint64_t block_num = uint256_to_u64_unsafe(block_num_u256);
    const uint64_t current = block->number;

    // Valid range: block_num >= current - 256 && block_num < current
    // Also need to handle current < 256 case
    const uint64_t min_block = (current >= 256) ? (current - 256) : 0;
    if (block_num >= min_block && block_num < current && block->get_block_hash != nullptr) {
      hash_t hash;
      if (block->get_block_hash(block_num, block->block_hash_user_data, &hash)) {
        result = uint256_from_bytes_be(hash.bytes, 32);
      }
    }
  }

  evm_stack_push_unsafe(frame->stack, result);
  return EVM_OK;
}

/// SELFBALANCE opcode (0x47): Push balance of current contract onto stack
/// Stack: [] => [balance]
/// Gas: 5
/// Requires state access
static inline evm_status_t op_selfbalance(call_frame_t *frame, state_access_t *state,
                                          const uint64_t gas_cost) {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, state_get_balance(state, &frame->address));
  return EVM_OK;
}

/// BLOBHASH opcode (0x49): Get versioned hash of blob at index
/// Stack: [index] => [blob_versioned_hash]
/// Gas: 3
/// Returns zero hash if index is out of bounds or no blobs
static inline evm_status_t op_blobhash(call_frame_t *frame, const tx_context_t *tx,
                                       const uint64_t gas_cost) {
  if (!evm_stack_has_items(frame->stack, 1)) {
    return EVM_STACK_UNDERFLOW;
  }
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  const uint256_t index_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t result = uint256_zero();

  // Check if index fits in size_t and is within bounds
  if (uint256_fits_u64(index_u256)) {
    const size_t index = (size_t)uint256_to_u64_unsafe(index_u256);
    if (index < tx->blob_hashes_count && tx->blob_hashes != nullptr) {
      result = uint256_from_bytes_be(tx->blob_hashes[index].bytes, 32);
    }
  }

  evm_stack_push_unsafe(frame->stack, result);
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_BLOCK_IMPL_H
