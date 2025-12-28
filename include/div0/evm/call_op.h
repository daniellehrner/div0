#ifndef DIV0_EVM_CALL_OP_H
#define DIV0_EVM_CALL_OP_H

#include "div0/evm/call_frame.h"
#include "div0/evm/gas.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/state/state_access.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// Call Setup Result
// =============================================================================

/// Result of preparing a CALL operation.
typedef struct {
  evm_status_t status;  // EVM_OK on success
  address_t target;     // Target address
  uint256_t value;      // Value to transfer
  uint64_t child_gas;   // Gas to give child frame
  uint64_t args_offset; // Memory offset of input data
  uint64_t args_size;   // Size of input data
  uint64_t ret_offset;  // Memory offset for return data
  uint64_t ret_size;    // Max size of return data
} call_setup_t;

// =============================================================================
// Helper Functions
// =============================================================================

/// Calculate child gas using EIP-150 63/64 rule.
/// @param gas_left Remaining gas after overhead costs
/// @param requested Requested gas from stack
/// @return Gas to allocate to child (min of 63/64 rule and requested)
static inline uint64_t call_child_gas(uint64_t gas_left, uint64_t requested) {
  uint64_t max_child = gas_cap_call(gas_left);
  return (requested < max_child) ? requested : max_child;
}

/// Calculate memory expansion cost for CALL operations.
/// Charges for max(args_end, ret_end) since expansion is done once.
/// @param mem Current memory
/// @param args_offset Input data offset
/// @param args_size Input data size
/// @param ret_offset Return data offset
/// @param ret_size Return data max size
/// @param gas_cost Output: gas cost for expansion
/// @return true on success, false on overflow
static inline bool call_memory_cost(evm_memory_t *mem, uint64_t args_offset, uint64_t args_size,
                                    uint64_t ret_offset, uint64_t ret_size, uint64_t *gas_cost) {
  uint64_t max_end = 0;

  if (args_size > 0) {
    if (args_offset > UINT64_MAX - args_size) {
      return false;
    }
    uint64_t args_end = args_offset + args_size;
    if (args_end > max_end) {
      max_end = args_end;
    }
  }

  if (ret_size > 0) {
    if (ret_offset > UINT64_MAX - ret_size) {
      return false;
    }
    uint64_t ret_end = ret_offset + ret_size;
    if (ret_end > max_end) {
      max_end = ret_end;
    }
  }

  if (max_end == 0) {
    *gas_cost = 0;
    return true;
  }

  // Calculate expansion cost
  size_t current_words = evm_memory_size(mem) / 32;
  size_t new_words = (max_end + 31) / 32;
  *gas_cost = evm_memory_expansion_cost(current_words, new_words);
  return true;
}

// =============================================================================
// CALL Preparation
// =============================================================================

/// Prepare a CALL operation - validates inputs, calculates gas, prepares setup.
///
/// Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
///
/// @param stack Current stack (items will be popped)
/// @param gas Current gas (will be reduced by overhead costs)
/// @param memory Current memory
/// @param state State access for warm/cold checks
/// @param is_static Whether we're in a static context
/// @param current_depth Current call depth
/// @return CallSetup with status and parameters for child frame creation
call_setup_t prepare_call(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                          state_access_t *state, bool is_static, uint16_t current_depth);

/// Prepare a STATICCALL operation.
///
/// Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
call_setup_t prepare_staticcall(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                                state_access_t *state, uint16_t current_depth);

/// Prepare a DELEGATECALL operation.
///
/// Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
call_setup_t prepare_delegatecall(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                                  state_access_t *state, uint16_t current_depth);

/// Prepare a CALLCODE operation.
///
/// Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
call_setup_t prepare_callcode(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                              state_access_t *state, bool is_static, uint16_t current_depth);

#endif // DIV0_EVM_CALL_OP_H
