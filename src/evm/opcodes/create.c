#include "div0/evm/opcodes/create.h"

#include "div0/evm/call_frame_pool.h"
#include "div0/evm/create_op.h"
#include "div0/evm/evm.h"
#include "div0/evm/memory.h"
#include "div0/evm/memory_pool.h"
#include "div0/evm/stack.h"
#include "div0/evm/stack_pool.h"
#include "div0/state/state_access.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

// =============================================================================
// Child Frame Initialization Helper
// =============================================================================

/// Allocates and initializes a child frame for CREATE/CREATE2.
/// @param evm EVM instance
/// @param parent Parent frame
/// @param setup Create setup from prepare_* function
/// @param exec_type EXEC_CREATE or EXEC_CREATE2
/// @return Initialized child frame, or nullptr if pool exhausted
static call_frame_t *init_create_child_frame(evm_t *const evm, const call_frame_t *const parent,
                                             const create_setup_t *const setup,
                                             const exec_type_t exec_type) {
  call_frame_t *const child = call_frame_pool_rent(&evm->frame_pool);
  if (child == nullptr) {
    return nullptr;
  }

  // Get init code pointer from parent's memory
  const uint8_t *init_code =
      setup->init_size > 0 ? evm_memory_ptr_unsafe(parent->memory, setup->init_offset) : nullptr;

  // Common initialization
  child->pc = 0;
  child->gas = setup->child_gas;
  child->stack = evm_stack_pool_borrow(&evm->stack_pool);
  child->memory = evm_memory_pool_borrow(&evm->memory_pool);
  if (child->stack == nullptr || child->memory == nullptr) {
    // Return in LIFO order: memory first (if borrowed), then frame
    // Stack is arena-allocated and return is a no-op
    if (child->memory != nullptr) {
      evm_memory_pool_return(&evm->memory_pool);
    }
    call_frame_pool_return(&evm->frame_pool);
    return nullptr;
  }
  child->code = init_code;
  child->code_size = setup->init_size;
  child->output_offset = 0; // CREATE has no output location in parent memory
  child->output_size = 0;
  child->depth = (uint16_t)(parent->depth + 1);
  child->input = nullptr; // CREATE has no input data
  child->input_size = 0;

  // CREATE-specific fields
  child->exec_type = exec_type;
  child->is_static = false; // CREATE always allows writes
  child->caller = parent->address;
  child->address = setup->target;
  child->value = setup->value;

  // Jump destination analysis (lazy: computed on first JUMP/JUMPI)
  child->jumpdest_bitmap = nullptr;
  child->code_hash = hash_zero();

  return child;
}

// =============================================================================
// CREATE Opcode Implementations
// =============================================================================

create_op_result_t op_create(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return create_op_error(EVM_STATE_UNAVAILABLE);
  }

  const create_setup_t setup = prepare_create(frame->stack, &frame->gas, frame->memory, state,
                                              &frame->address, frame->is_static, frame->depth);

  // Soft failures: depth exceeded, nonce overflow, or address collision
  // These push 0 and continue execution (not fatal errors)
  if (setup.status == EVM_CALL_DEPTH_EXCEEDED || setup.status == EVM_NONCE_OVERFLOW ||
      setup.status == EVM_CREATE_COLLISION) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return create_op_continue();
  }
  if (setup.status != EVM_OK) {
    return create_op_error(setup.status);
  }

  // Check caller balance for value transfer
  if (!uint256_is_zero(setup.value)) {
    const uint256_t balance = state_get_balance(state, &frame->address);
    if (uint256_lt(balance, setup.value)) {
      evm_stack_push_unsafe(frame->stack, uint256_zero());
      return create_op_continue();
    }
  }

  // Take state snapshot for rollback on failure
  const uint64_t snapshot = state_snapshot(state);

  // Increment sender nonce BEFORE contract creation
  (void)state_increment_nonce(state, &frame->address);

  // Transfer value to new contract address
  if (!uint256_is_zero(setup.value)) {
    (void)state_sub_balance(state, &frame->address, setup.value);
    (void)state_add_balance(state, &setup.target, setup.value);
  }

  // Create contract account with nonce = 1 (EIP-161)
  state_create_contract(state, &setup.target);

  // Initialize child frame with init code
  call_frame_t *const child = init_create_child_frame(evm, frame, &setup, EXEC_CREATE);
  if (child == nullptr) {
    state_revert_to_snapshot(state, snapshot);
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return create_op_continue();
  }

  // Store snapshot for potential rollback after child execution
  // Note: The main loop will handle rollback if child fails
  child->output_offset = snapshot; // Repurpose output_offset to store snapshot ID

  evm->pending_frame = child;
  return create_op_create();
}

create_op_result_t op_create2(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return create_op_error(EVM_STATE_UNAVAILABLE);
  }

  const create_setup_t setup = prepare_create2(frame->stack, &frame->gas, frame->memory, state,
                                               &frame->address, frame->is_static, frame->depth);

  // Soft failures: depth exceeded, nonce overflow, or address collision
  // These push 0 and continue execution (not fatal errors)
  if (setup.status == EVM_CALL_DEPTH_EXCEEDED || setup.status == EVM_NONCE_OVERFLOW ||
      setup.status == EVM_CREATE_COLLISION) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return create_op_continue();
  }
  if (setup.status != EVM_OK) {
    return create_op_error(setup.status);
  }

  // Check caller balance for value transfer
  if (!uint256_is_zero(setup.value)) {
    const uint256_t balance = state_get_balance(state, &frame->address);
    if (uint256_lt(balance, setup.value)) {
      evm_stack_push_unsafe(frame->stack, uint256_zero());
      return create_op_continue();
    }
  }

  // Take state snapshot for rollback on failure
  const uint64_t snapshot = state_snapshot(state);

  // Increment sender nonce BEFORE contract creation
  (void)state_increment_nonce(state, &frame->address);

  // Transfer value to new contract address
  if (!uint256_is_zero(setup.value)) {
    (void)state_sub_balance(state, &frame->address, setup.value);
    (void)state_add_balance(state, &setup.target, setup.value);
  }

  // Create contract account with nonce = 1 (EIP-161)
  state_create_contract(state, &setup.target);

  // Initialize child frame with init code
  call_frame_t *const child = init_create_child_frame(evm, frame, &setup, EXEC_CREATE2);
  if (child == nullptr) {
    state_revert_to_snapshot(state, snapshot);
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return create_op_continue();
  }

  // Store snapshot for potential rollback after child execution
  child->output_offset = snapshot; // Repurpose output_offset to store snapshot ID

  evm->pending_frame = child;
  return create_op_create();
}
