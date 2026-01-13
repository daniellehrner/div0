#ifndef DIV0_EVM_CREATE_OP_H
#define DIV0_EVM_CREATE_OP_H

#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/state/state_access.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// Create Setup Result
// =============================================================================

/// Result of preparing a CREATE/CREATE2 operation.
typedef struct {
  evm_status_t status;  // EVM_OK on success
  address_t target;     // Computed contract address
  uint256_t value;      // Value to transfer (endowment)
  uint64_t child_gas;   // Gas to give child frame
  uint64_t init_offset; // Memory offset of init code
  uint64_t init_size;   // Size of init code
} create_setup_t;

// =============================================================================
// CREATE Preparation
// =============================================================================

/// Prepare a CREATE operation - validates inputs, calculates gas, computes address.
///
/// Stack: [value, offset, size] => []
///
/// @param stack Current stack (items will be popped)
/// @param gas Current gas (will be reduced by overhead costs)
/// @param memory Current memory
/// @param state State access for nonce and collision checks
/// @param sender Contract creator address
/// @param is_static Whether we're in a static context
/// @param current_depth Current call depth
/// @return CreateSetup with status and parameters for child frame creation
create_setup_t prepare_create(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                              state_access_t *state, const address_t *sender, bool is_static,
                              uint16_t current_depth);

/// Prepare a CREATE2 operation - validates inputs, calculates gas, computes address.
///
/// Stack: [value, offset, size, salt] => []
///
/// @param stack Current stack (items will be popped)
/// @param gas Current gas (will be reduced by overhead costs)
/// @param memory Current memory
/// @param state State access for collision checks
/// @param sender Contract creator address
/// @param is_static Whether we're in a static context
/// @param current_depth Current call depth
/// @return CreateSetup with status and parameters for child frame creation
create_setup_t prepare_create2(evm_stack_t *stack, uint64_t *gas, evm_memory_t *memory,
                               state_access_t *state, const address_t *sender, bool is_static,
                               uint16_t current_depth);

#endif // DIV0_EVM_CREATE_OP_H
