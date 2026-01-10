#include "div0/evm/call_op.h"

#include "div0/evm/gas.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

// =============================================================================
// Internal Helpers
// =============================================================================

/// Raw stack arguments for CALL-family opcodes.
typedef struct {
  uint256_t requested_gas;
  uint256_t addr;
  uint256_t value; // Only for CALL/CALLCODE
  uint256_t args_offset;
  uint256_t args_size;
  uint256_t ret_offset;
  uint256_t ret_size;
} call_stack_args_t;

/// Pops 7 arguments for CALL/CALLCODE (includes value).
static call_stack_args_t pop_call_args(evm_stack_t *stack) {
  call_stack_args_t args;
  args.requested_gas = evm_stack_pop_unsafe(stack);
  args.addr = evm_stack_pop_unsafe(stack);
  args.value = evm_stack_pop_unsafe(stack);
  args.args_offset = evm_stack_pop_unsafe(stack);
  args.args_size = evm_stack_pop_unsafe(stack);
  args.ret_offset = evm_stack_pop_unsafe(stack);
  args.ret_size = evm_stack_pop_unsafe(stack);
  return args;
}

/// Pops 6 arguments for STATICCALL/DELEGATECALL (no value).
static call_stack_args_t pop_call_args_no_value(evm_stack_t *stack) {
  call_stack_args_t args;
  args.requested_gas = evm_stack_pop_unsafe(stack);
  args.addr = evm_stack_pop_unsafe(stack);
  args.value = uint256_zero();
  args.args_offset = evm_stack_pop_unsafe(stack);
  args.args_size = evm_stack_pop_unsafe(stack);
  args.ret_offset = evm_stack_pop_unsafe(stack);
  args.ret_size = evm_stack_pop_unsafe(stack);
  return args;
}

/// Validates memory arguments and populates result offsets/sizes.
/// Returns false if any offset/size doesn't fit in uint64.
static bool validate_memory_args(const call_stack_args_t *args, call_setup_t *result) {
  if (!uint256_fits_u64(args->args_offset) || !uint256_fits_u64(args->args_size) ||
      !uint256_fits_u64(args->ret_offset) || !uint256_fits_u64(args->ret_size)) {
    return false;
  }
  result->args_offset = uint256_to_u64_unsafe(args->args_offset);
  result->args_size = uint256_to_u64_unsafe(args->args_size);
  result->ret_offset = uint256_to_u64_unsafe(args->ret_offset);
  result->ret_size = uint256_to_u64_unsafe(args->ret_size);
  return true;
}

/// Calculates gas costs and child gas allocation.
/// Returns false on out-of-gas.
static bool calculate_call_gas(uint64_t *const parent_gas, const call_setup_t *const setup,
                               const call_stack_args_t *const args, evm_memory_t *const memory,
                               state_access_t *const state, const bool transfers_value,
                               const bool is_new_account, uint64_t *const out_child_gas) {
  uint64_t call_gas_cost = 0;

  // 1. EIP-2929: Cold/warm address access cost
  const bool was_cold = state_warm_address(state, &setup->target);
  call_gas_cost += was_cold ? GAS_COLD_ACCOUNT_ACCESS : GAS_WARM_ACCESS;

  // 2. Value transfer cost
  if (transfers_value) {
    call_gas_cost += GAS_CALL_VALUE;
    // 3. New account cost (only for CALL with value to empty account)
    if (is_new_account) {
      call_gas_cost += GAS_NEW_ACCOUNT;
    }
  }

  // 4. Memory expansion cost
  uint64_t mem_cost = 0;
  if (!call_memory_cost(memory, setup->args_offset, setup->args_size, setup->ret_offset,
                        setup->ret_size, &mem_cost)) {
    return false;
  }
  call_gas_cost += mem_cost;

  // Deduct call overhead from parent
  if (*parent_gas < call_gas_cost) {
    return false;
  }
  *parent_gas -= call_gas_cost;

  // 5. Calculate child gas using EIP-150 63/64 rule
  const uint64_t requested_gas = uint256_fits_u64(args->requested_gas)
                                     ? uint256_to_u64_unsafe(args->requested_gas)
                                     : UINT64_MAX;
  uint64_t child_gas = call_child_gas(*parent_gas, requested_gas);

  // Add gas stipend if transferring value
  if (transfers_value) {
    child_gas += GAS_CALL_STIPEND;
  }

  // Deduct child gas from parent (excluding stipend)
  uint64_t gas_to_deduct = child_gas - (transfers_value ? GAS_CALL_STIPEND : 0);
  if (gas_to_deduct > *parent_gas) {
    gas_to_deduct = *parent_gas;
  }
  *parent_gas -= gas_to_deduct;

  *out_child_gas = child_gas;
  return true;
}

/// Expands memory to cover both input and output regions.
static void expand_call_memory(evm_memory_t *memory, const call_setup_t *setup) {
  uint64_t max_end = 0;
  if (setup->args_size > 0) {
    const uint64_t args_end = setup->args_offset + setup->args_size;
    if (args_end > max_end) {
      max_end = args_end;
    }
  }
  if (setup->ret_size > 0) {
    const uint64_t ret_end = setup->ret_offset + setup->ret_size;
    if (ret_end > max_end) {
      max_end = ret_end;
    }
  }
  if (max_end > evm_memory_size(memory)) {
    uint64_t dummy_cost = 0;
    (void)evm_memory_expand(memory, 0, max_end, &dummy_cost);
  }
}

// =============================================================================
// Public API
// =============================================================================

call_setup_t prepare_call(evm_stack_t *const stack, uint64_t *const gas, evm_memory_t *const memory,
                          state_access_t *const state, const bool is_static,
                          const uint16_t current_depth) {
  call_setup_t result = {.status = EVM_OK};

  // Check stack underflow (need 7 items)
  if (!evm_stack_has_items(stack, 7)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  // Check call depth limit
  if (current_depth >= MAX_CALL_DEPTH) {
    for (size_t i = 0; i < 7; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  // Pop and validate arguments
  const call_stack_args_t args = pop_call_args(stack);
  result.value = args.value;

  if (!validate_memory_args(&args, &result)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // Check static context violation
  const bool transfers_value = !uint256_is_zero(result.value);
  if (is_static && transfers_value) {
    result.status = EVM_WRITE_PROTECTION;
    return result;
  }

  result.target = address_from_uint256(&args.addr);

  // Calculate gas and allocate child gas
  const bool is_new_account = transfers_value && state_account_is_empty(state, &result.target);
  if (!calculate_call_gas(gas, &result, &args, memory, state, transfers_value, is_new_account,
                          &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  expand_call_memory(memory, &result);
  return result;
}

call_setup_t prepare_staticcall(evm_stack_t *const stack, uint64_t *const gas,
                                evm_memory_t *const memory, state_access_t *const state,
                                const uint16_t current_depth) {
  call_setup_t result = {.status = EVM_OK, .value = uint256_zero()};

  if (!evm_stack_has_items(stack, 6)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  if (current_depth >= MAX_CALL_DEPTH) {
    for (size_t i = 0; i < 6; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  const call_stack_args_t args = pop_call_args_no_value(stack);

  if (!validate_memory_args(&args, &result)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  result.target = address_from_uint256(&args.addr);

  // STATICCALL: no value transfer, no new account
  if (!calculate_call_gas(gas, &result, &args, memory, state, false, false, &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  expand_call_memory(memory, &result);
  return result;
}

call_setup_t prepare_delegatecall(evm_stack_t *const stack, uint64_t *const gas,
                                  evm_memory_t *const memory, state_access_t *const state,
                                  const uint16_t current_depth) {
  call_setup_t result = {.status = EVM_OK, .value = uint256_zero()};

  if (!evm_stack_has_items(stack, 6)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  if (current_depth >= MAX_CALL_DEPTH) {
    for (size_t i = 0; i < 6; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  const call_stack_args_t args = pop_call_args_no_value(stack);

  if (!validate_memory_args(&args, &result)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  result.target = address_from_uint256(&args.addr);

  // DELEGATECALL: no value transfer, no new account
  if (!calculate_call_gas(gas, &result, &args, memory, state, false, false, &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  expand_call_memory(memory, &result);
  return result;
}

call_setup_t prepare_callcode(evm_stack_t *const stack, uint64_t *const gas,
                              evm_memory_t *const memory, state_access_t *const state,
                              const bool is_static, const uint16_t current_depth) {
  call_setup_t result = {.status = EVM_OK};

  if (!evm_stack_has_items(stack, 7)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  if (current_depth >= MAX_CALL_DEPTH) {
    for (size_t i = 0; i < 7; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  const call_stack_args_t args = pop_call_args(stack);
  result.value = args.value;

  if (!validate_memory_args(&args, &result)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  const bool transfers_value = !uint256_is_zero(result.value);
  if (is_static && transfers_value) {
    result.status = EVM_WRITE_PROTECTION;
    return result;
  }

  result.target = address_from_uint256(&args.addr);

  // CALLCODE: value affects gas but doesn't create new accounts
  if (!calculate_call_gas(gas, &result, &args, memory, state, transfers_value, false,
                          &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  expand_call_memory(memory, &result);
  return result;
}
