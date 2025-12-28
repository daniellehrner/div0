#ifndef DIV0_EVM_EVM_H
#define DIV0_EVM_EVM_H

#include "div0/evm/block_context.h"
#include "div0/evm/call_frame.h"
#include "div0/evm/call_frame_pool.h"
#include "div0/evm/execution_env.h"
#include "div0/evm/frame_result.h"
#include "div0/evm/memory_pool.h"
#include "div0/evm/stack.h"
#include "div0/evm/stack_pool.h"
#include "div0/evm/status.h"
#include "div0/evm/tx_context.h"
#include "div0/mem/arena.h"

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// EVM Execution API
// ============================================================================

/// EVM execution result with return data.
typedef struct {
  evm_result_t result;   // Success/error status
  evm_status_t error;    // Specific error if result == EVM_RESULT_ERROR
  uint64_t gas_used;     // Total gas consumed
  uint64_t gas_refund;   // Gas to refund
  const uint8_t *output; // Return data (points into EVM's return buffer)
  size_t output_size;    // Return data size
} evm_execution_result_t;

/// EVM instance with full call frame support.
/// This is a large structure - allocate on heap, not stack.
typedef struct evm {
  // Resource pools (pre-allocated for zero-allocation execution)
  call_frame_pool_t frame_pool;
  evm_stack_pool_t stack_pool;
  evm_memory_pool_t memory_pool;

  // Backing arena allocator
  div0_arena_t *arena;

  // Current execution state
  call_frame_t *current_frame;
  call_frame_t *pending_frame; // Set by CALL handler, consumed by main loop

  // Context references (non-owning, set per block/transaction)
  const block_context_t *block;
  const tx_context_t *tx;

  // Return data buffer (RETURNDATASIZE, RETURNDATACOPY)
  uint8_t *return_data;
  size_t return_data_size;
  size_t return_data_capacity;

  // State access (optional, for SLOAD/SSTORE)
  void *state; // state_access_t* when state library is linked
} evm_t;

/// Initializes an EVM instance with an arena allocator.
/// @param evm EVM instance to initialize
/// @param arena Arena allocator for all allocations
void evm_init(evm_t *evm, div0_arena_t *arena);

/// Resets an EVM instance for reuse.
/// Clears return data and resets pools, but keeps arena.
void evm_reset(evm_t *evm);

/// Sets the block context for execution.
/// Call once per block before executing transactions.
static inline void evm_set_block_context(evm_t *evm, const block_context_t *block) {
  evm->block = block;
}

/// Sets the transaction context for execution.
/// Call once per transaction before executing.
static inline void evm_set_tx_context(evm_t *evm, const tx_context_t *tx) {
  evm->tx = tx;
}

/// Executes bytecode with the new call frame architecture.
/// @param evm Initialized EVM instance
/// @param env Execution environment (block, tx, call params)
/// @return Execution result with status, gas, and return data
[[nodiscard]] evm_execution_result_t evm_execute_env(evm_t *evm, const execution_env_t *env);

#endif // DIV0_EVM_EVM_H
