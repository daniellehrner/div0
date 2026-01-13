#include "div0/evm/create_op.h"

#include "div0/crypto/keccak256.h"
#include "div0/evm/gas.h"
#include "div0/executor/block_executor.h"
#include "div0/types/hash.h"

// =============================================================================
// Internal Helpers
// =============================================================================

/// Raw stack arguments for CREATE opcode.
typedef struct {
  uint256_t value;
  uint256_t offset;
  uint256_t size;
} create_stack_args_t;

/// Raw stack arguments for CREATE2 opcode (includes salt).
typedef struct {
  uint256_t value;
  uint256_t offset;
  uint256_t size;
  uint256_t salt;
} create2_stack_args_t;

/// Pops 3 arguments for CREATE.
static create_stack_args_t pop_create_args(evm_stack_t *const stack) {
  create_stack_args_t args;
  args.value = evm_stack_pop_unsafe(stack);
  args.offset = evm_stack_pop_unsafe(stack);
  args.size = evm_stack_pop_unsafe(stack);
  return args;
}

/// Pops 4 arguments for CREATE2.
static create2_stack_args_t pop_create2_args(evm_stack_t *const stack) {
  create2_stack_args_t args;
  args.value = evm_stack_pop_unsafe(stack);
  args.offset = evm_stack_pop_unsafe(stack);
  args.size = evm_stack_pop_unsafe(stack);
  args.salt = evm_stack_pop_unsafe(stack);
  return args;
}

/// Calculates CREATE/CREATE2 gas costs and child gas allocation.
/// Returns false on out-of-gas or overflow.
static bool calculate_create_gas(uint64_t *const parent_gas, evm_memory_t *const memory,
                                 const uint64_t init_offset, const uint64_t init_size,
                                 const bool is_create2, uint64_t *const out_child_gas) {
  uint64_t total_cost = GAS_CREATE; // 32000 base

  // Memory expansion cost for reading init code
  if (init_size > 0) {
    uint64_t mem_cost = 0;
    if (!evm_memory_expand(memory, init_offset, init_size, &mem_cost)) {
      return false;
    }
    if (total_cost > UINT64_MAX - mem_cost) {
      return false;
    }
    total_cost += mem_cost;
  }

  // EIP-3860: Init code word cost (Shanghai+)
  // CREATE: 2 gas per 32-byte word
  // CREATE2: 2 + 6 = 8 gas per 32-byte word (init code + keccak256 hashing)
  if (init_size > 0) {
    const uint64_t num_words = (init_size + 31) / 32;
    const uint64_t per_word_cost =
        is_create2 ? (GAS_INITCODE_WORD + GAS_KECCAK256_WORD) : GAS_INITCODE_WORD;

    // Overflow check for multiplication
    if (num_words > UINT64_MAX / per_word_cost) {
      return false;
    }
    const uint64_t initcode_cost = num_words * per_word_cost;

    // Overflow check for addition
    if (total_cost > UINT64_MAX - initcode_cost) {
      return false;
    }
    total_cost += initcode_cost;
  }

  // Check if parent has enough gas
  if (*parent_gas < total_cost) {
    return false;
  }
  *parent_gas -= total_cost;

  // Child gas: 63/64 of remaining (EIP-150)
  *out_child_gas = gas_cap_call(*parent_gas);
  *parent_gas -= *out_child_gas;

  return true;
}

// =============================================================================
// Public API
// =============================================================================

create_setup_t prepare_create(evm_stack_t *const stack, uint64_t *const gas,
                              evm_memory_t *const memory, state_access_t *const state,
                              const address_t *const sender, const bool is_static,
                              const uint16_t current_depth) {
  create_setup_t result = {.status = EVM_OK};

  // 1. Check stack underflow (need 3 items)
  if (!evm_stack_has_items(stack, 3)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  // 2. Check static context - CREATE is state-modifying
  if (is_static) {
    result.status = EVM_WRITE_PROTECTION;
    return result;
  }

  // 3. Check call depth limit
  if (current_depth >= MAX_CALL_DEPTH) {
    // Pop items but return depth exceeded (not fatal, push 0)
    for (size_t i = 0; i < 3; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  // Pop stack arguments
  const create_stack_args_t args = pop_create_args(stack);
  result.value = args.value;

  // 4. Validate offset/size fit in uint64
  if (!uint256_fits_u64(args.offset) || !uint256_fits_u64(args.size)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }
  result.init_offset = uint256_to_u64_unsafe(args.offset);
  result.init_size = uint256_to_u64_unsafe(args.size);

  // 5. Check offset + size overflow
  if (result.init_offset > UINT64_MAX - result.init_size) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // 6. EIP-3860: Check init code size limit
  if (result.init_size > MAX_INITCODE_SIZE) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // 7. Get sender nonce and check for overflow
  const uint64_t sender_nonce = state_get_nonce(state, sender);
  if (sender_nonce == UINT64_MAX) {
    result.status = EVM_CALL_DEPTH_EXCEEDED; // Nonce overflow (soft failure)
    return result;
  }

  // 8. Compute contract address (using current nonce, before increment)
  result.target = compute_create_address(sender, sender_nonce);

  // 9. Address collision check - target must have no code and zero nonce
  const size_t existing_code_size = state_get_code_size(state, &result.target);
  const uint64_t existing_nonce = state_get_nonce(state, &result.target);
  if (existing_code_size > 0 || existing_nonce > 0) {
    result.status = EVM_CALL_DEPTH_EXCEEDED; // Collision (soft failure)
    return result;
  }

  // 10. Calculate gas costs
  if (!calculate_create_gas(gas, memory, result.init_offset, result.init_size, false,
                            &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  return result;
}

create_setup_t prepare_create2(evm_stack_t *const stack, uint64_t *const gas,
                               evm_memory_t *const memory, state_access_t *const state,
                               const address_t *const sender, const bool is_static,
                               const uint16_t current_depth) {
  create_setup_t result = {.status = EVM_OK};

  // 1. Check stack underflow (need 4 items)
  if (!evm_stack_has_items(stack, 4)) {
    result.status = EVM_STACK_UNDERFLOW;
    return result;
  }

  // 2. Check static context - CREATE2 is state-modifying
  if (is_static) {
    result.status = EVM_WRITE_PROTECTION;
    return result;
  }

  // 3. Check call depth limit
  if (current_depth >= MAX_CALL_DEPTH) {
    // Pop items but return depth exceeded (not fatal, push 0)
    for (size_t i = 0; i < 4; ++i) {
      (void)evm_stack_pop_unsafe(stack);
    }
    result.status = EVM_CALL_DEPTH_EXCEEDED;
    return result;
  }

  // Pop stack arguments
  const create2_stack_args_t args = pop_create2_args(stack);
  result.value = args.value;

  // 4. Validate offset/size fit in uint64
  if (!uint256_fits_u64(args.offset) || !uint256_fits_u64(args.size)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }
  result.init_offset = uint256_to_u64_unsafe(args.offset);
  result.init_size = uint256_to_u64_unsafe(args.size);

  // 5. Check offset + size overflow
  if (result.init_offset > UINT64_MAX - result.init_size) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // 6. EIP-3860: Check init code size limit
  if (result.init_size > MAX_INITCODE_SIZE) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // 7. Calculate gas costs first (we need memory expansion for address computation)
  if (!calculate_create_gas(gas, memory, result.init_offset, result.init_size, true,
                            &result.child_gas)) {
    result.status = EVM_OUT_OF_GAS;
    return result;
  }

  // 8. Get init code pointer from memory (memory is now expanded) and hash it
  const uint8_t *init_code =
      result.init_size > 0 ? evm_memory_ptr_unsafe(memory, result.init_offset) : nullptr;
  const hash_t init_code_hash = keccak256(init_code, result.init_size);

  // 9. Convert uint256 salt to hash_t (both are 32 bytes, just different representation)
  hash_t salt_hash;
  uint256_to_bytes_be(args.salt, salt_hash.bytes);

  // 10. Compute contract address
  result.target = compute_create2_address(sender, &salt_hash, &init_code_hash);

  // 11. Address collision check - target must have no code and zero nonce
  const size_t existing_code_size = state_get_code_size(state, &result.target);
  const uint64_t existing_nonce = state_get_nonce(state, &result.target);
  if (existing_code_size > 0 || existing_nonce > 0) {
    result.status = EVM_CALL_DEPTH_EXCEEDED; // Collision (soft failure)
    return result;
  }

  return result;
}
