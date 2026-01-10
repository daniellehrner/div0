#include "div0/evm/opcodes/call.h"

#include "div0/evm/call_frame_pool.h"
#include "div0/evm/call_op.h"
#include "div0/evm/evm.h"
#include "div0/evm/memory.h"
#include "div0/evm/memory_pool.h"
#include "div0/evm/stack.h"
#include "div0/evm/stack_pool.h"
#include "div0/state/state_access.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

// =============================================================================
// Child Frame Initialization Helper
// =============================================================================

/// Parameters for child frame that vary by opcode.
typedef struct {
  exec_type_t exec_type;
  bool is_static;
  address_t caller;
  address_t address;
  uint256_t value;
} child_frame_params_t;

/// Allocates and initializes a child frame with common settings.
/// @param evm EVM instance
/// @param parent Parent frame
/// @param setup Call setup from prepare_* function
/// @param code Target contract code
/// @param params Opcode-specific parameters
/// @return Initialized child frame, or nullptr if pool exhausted
static call_frame_t *init_child_frame(evm_t *const evm, call_frame_t *const parent,
                                      const call_setup_t *const setup, const bytes_t *const code,
                                      const child_frame_params_t *const params) {
  call_frame_t *const child = call_frame_pool_rent(&evm->frame_pool);
  if (child == nullptr) {
    return nullptr;
  }

  // Common initialization
  child->pc = 0;
  child->gas = setup->child_gas;
  child->stack = evm_stack_pool_borrow(&evm->stack_pool);
  child->memory = evm_memory_pool_borrow(&evm->memory_pool);
  child->code = code->data;
  child->code_size = code->size;
  child->output_offset = setup->ret_offset;
  child->output_size = (uint32_t)setup->ret_size;
  child->depth = (uint16_t)(parent->depth + 1);
  child->input =
      setup->args_size > 0 ? evm_memory_ptr_unsafe(parent->memory, setup->args_offset) : nullptr;
  child->input_size = (size_t)setup->args_size;

  // Opcode-specific fields
  child->exec_type = params->exec_type;
  child->is_static = params->is_static;
  child->caller = params->caller;
  child->address = params->address;
  child->value = params->value;

  // Jump destination analysis (lazy: computed on first JUMP/JUMPI)
  child->jumpdest_bitmap = nullptr;
  child->code_hash = hash_zero(); // TODO: set from state if code hash is known

  // Store parent's output location
  parent->output_offset = setup->ret_offset;
  parent->output_size = (uint32_t)setup->ret_size;

  return child;
}

// =============================================================================
// CALL Opcode Implementations
// =============================================================================

call_op_result_t op_call(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return call_op_error(EVM_INVALID_OPCODE);
  }

  const call_setup_t setup =
      prepare_call(frame->stack, &frame->gas, frame->memory, state, frame->is_static, frame->depth);

  if (setup.status == EVM_CALL_DEPTH_EXCEEDED) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }
  if (setup.status != EVM_OK) {
    return call_op_error(setup.status);
  }

  // Check caller balance for value transfer
  if (!uint256_is_zero(setup.value)) {
    const uint256_t balance = state_get_balance(state, &frame->address);
    if (uint256_lt(balance, setup.value)) {
      evm_stack_push_unsafe(frame->stack, uint256_zero());
      return call_op_continue();
    }
    // Transfer value
    (void)state_sub_balance(state, &frame->address, setup.value);
    (void)state_add_balance(state, &setup.target, setup.value);
  }

  const bytes_t code = state_get_code(state, &setup.target);
  const child_frame_params_t params = {
      .exec_type = EXEC_CALL,
      .is_static = frame->is_static,
      .caller = frame->address,
      .address = setup.target,
      .value = setup.value,
  };

  call_frame_t *const child = init_child_frame(evm, frame, &setup, &code, &params);
  if (child == nullptr) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }

  evm->pending_frame = child;
  return call_op_call();
}

call_op_result_t op_staticcall(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return call_op_error(EVM_INVALID_OPCODE);
  }

  const call_setup_t setup =
      prepare_staticcall(frame->stack, &frame->gas, frame->memory, state, frame->depth);

  if (setup.status == EVM_CALL_DEPTH_EXCEEDED) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }
  if (setup.status != EVM_OK) {
    return call_op_error(setup.status);
  }

  const bytes_t code = state_get_code(state, &setup.target);
  const child_frame_params_t params = {
      .exec_type = EXEC_STATICCALL,
      .is_static = true, // STATICCALL always sets static context
      .caller = frame->address,
      .address = setup.target,
      .value = uint256_zero(),
  };

  call_frame_t *const child = init_child_frame(evm, frame, &setup, &code, &params);
  if (child == nullptr) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }

  evm->pending_frame = child;
  return call_op_call();
}

call_op_result_t op_delegatecall(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return call_op_error(EVM_INVALID_OPCODE);
  }

  const call_setup_t setup =
      prepare_delegatecall(frame->stack, &frame->gas, frame->memory, state, frame->depth);

  if (setup.status == EVM_CALL_DEPTH_EXCEEDED) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }
  if (setup.status != EVM_OK) {
    return call_op_error(setup.status);
  }

  // DELEGATECALL: get code from target, but run in current context
  const bytes_t code = state_get_code(state, &setup.target);
  const child_frame_params_t params = {
      .exec_type = EXEC_DELEGATECALL,
      .is_static = frame->is_static, // Inherit static context
      .caller = frame->caller,       // Keep original caller
      .address = frame->address,     // Keep current address (storage context)
      .value = frame->value,         // Inherit value
  };

  call_frame_t *const child = init_child_frame(evm, frame, &setup, &code, &params);
  if (child == nullptr) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }

  evm->pending_frame = child;
  return call_op_call();
}

call_op_result_t op_callcode(evm_t *const evm, call_frame_t *const frame) {
  state_access_t *const state = evm->state;
  if (state == nullptr) {
    return call_op_error(EVM_INVALID_OPCODE);
  }

  const call_setup_t setup = prepare_callcode(frame->stack, &frame->gas, frame->memory, state,
                                              frame->is_static, frame->depth);

  if (setup.status == EVM_CALL_DEPTH_EXCEEDED) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }
  if (setup.status != EVM_OK) {
    return call_op_error(setup.status);
  }

  // Check caller balance for value transfer (CALLCODE can transfer value)
  if (!uint256_is_zero(setup.value)) {
    const uint256_t balance = state_get_balance(state, &frame->address);
    if (uint256_lt(balance, setup.value)) {
      evm_stack_push_unsafe(frame->stack, uint256_zero());
      return call_op_continue();
    }
    // Note: CALLCODE doesn't actually transfer value, it's just for gas calculation
  }

  // CALLCODE: get code from target, but run at current address
  const bytes_t code = state_get_code(state, &setup.target);
  const child_frame_params_t params = {
      .exec_type = EXEC_CALLCODE,
      .is_static = frame->is_static,
      .caller = frame->address,  // Caller is current address
      .address = frame->address, // Execute at current address
      .value = setup.value,
  };

  call_frame_t *const child = init_child_frame(evm, frame, &setup, &code, &params);
  if (child == nullptr) {
    evm_stack_push_unsafe(frame->stack, uint256_zero());
    return call_op_continue();
  }

  evm->pending_frame = child;
  return call_op_call();
}
