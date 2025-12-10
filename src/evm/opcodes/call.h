#ifndef DIV0_EVM_OPCODES_CONTROL_FLOW_H
#define DIV0_EVM_OPCODES_CONTROL_FLOW_H

#include <cstdint>

#include "div0/evm/execution_result.h"
#include "div0/evm/frame_result.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/state/account_provider.h"
#include "div0/types/address.h"

namespace div0::evm::opcodes {

// ============================================================================
// RETURN / REVERT opcodes
// ============================================================================

/// Common implementation for RETURN and REVERT opcodes
template <bool IsReturn>
[[gnu::always_inline]] inline FrameResult halt_with_data(Stack& stack, uint64_t& gas,
                                                         const gas::MemoryAccessCostFn& cost_fn,
                                                         Memory& memory) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::StackUnderflow);
  }

  uint64_t offset;
  uint64_t size;
  if (!stack[0].to_uint64(offset)) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::OutOfGas);
  }
  if (!stack[1].to_uint64(size)) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::OutOfGas);
  }

  (void)stack.pop();
  (void)stack.pop();

  if (size == 0) {
    if constexpr (IsReturn) {
      return FrameResult::return_data(0, 0);
    } else {
      return FrameResult::revert_data(0, 0);
    }
  }

  if (offset > UINT64_MAX - size) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::OutOfGas);
  }

  const uint64_t mem_cost = cost_fn(memory.size(), offset, size);
  if (mem_cost == UINT64_MAX || gas < mem_cost) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::OutOfGas);
  }

  gas -= mem_cost;
  memory.expand(offset + size);

  if constexpr (IsReturn) {
    return FrameResult::return_data(offset, size);
  } else {
    return FrameResult::revert_data(offset, size);
  }
}

/**
 * @brief RETURN - Halt execution returning output data.
 * Stack: [offset, size] => []
 */
[[gnu::always_inline]] inline FrameResult return_op(Stack& stack, uint64_t& gas,
                                                    const gas::MemoryAccessCostFn& cost_fn,
                                                    Memory& memory) noexcept {
  return halt_with_data<true>(stack, gas, cost_fn, memory);
}

/**
 * @brief REVERT - Halt execution reverting state changes.
 * Stack: [offset, size] => []
 */
[[gnu::always_inline]] inline FrameResult revert_op(Stack& stack, uint64_t& gas,
                                                    const gas::MemoryAccessCostFn& cost_fn,
                                                    Memory& memory) noexcept {
  return halt_with_data<false>(stack, gas, cost_fn, memory);
}

// ============================================================================
// CALL family opcodes
// ============================================================================

/// Result of preparing a CALL operation
struct CallSetup {
  ExecutionStatus status{ExecutionStatus::Success};
  types::Address target;
  types::Uint256 value;
  uint64_t child_gas{0};
  uint64_t args_offset{0};
  uint64_t args_size{0};
  uint64_t ret_offset{0};
  uint64_t ret_size{0};
};

/**
 * @brief Prepare a CALL operation - validates inputs, calculates gas, expands memory.
 *
 * This function handles everything except creating the child frame:
 * - Stack validation and popping
 * - Static context check
 * - Gas calculation (EIP-2929 access, value transfer, new account, memory)
 * - Memory expansion
 *
 * Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
 *
 * @param stack Current stack
 * @param gas Current gas (will be reduced by overhead costs)
 * @param memory Current memory (will be expanded if needed)
 * @param accounts Account provider for warm/cold and empty checks
 * @param is_static Whether we're in a static context
 * @param current_depth Current call depth
 * @param memory_cost_fn Function to calculate memory expansion cost
 * @return CallSetup with status and parameters for child frame creation
 */
[[nodiscard]] inline CallSetup prepare_call(
    Stack& stack, uint64_t& gas, Memory& memory, state::AccountProvider& accounts,
    const bool is_static, const uint16_t current_depth,
    const gas::MemoryAccessCostFn& memory_cost_fn) noexcept {
  CallSetup result;

  // Check stack underflow (need 7 items)
  if (!stack.has_items(7)) [[unlikely]] {
    result.status = ExecutionStatus::StackUnderflow;
    return result;
  }

  // Check call depth limit (EVM max is 1024)
  if (current_depth >= 1024) [[unlikely]] {
    // Pop all 7 items and push 0 (failure) - depth exceeded is not a fatal error
    for (size_t i = 0; i < 7; ++i) {
      (void)stack.pop();
    }
    (void)stack.push(types::Uint256::zero());
    result.status = ExecutionStatus::CallDepthExceeded;
    return result;
  }

  // Pop stack arguments
  const auto requested_gas_u256 = stack.pop_unsafe();
  const auto addr_u256 = stack.pop_unsafe();
  result.value = stack.pop_unsafe();
  const auto args_offset_u256 = stack.pop_unsafe();
  const auto args_size_u256 = stack.pop_unsafe();
  const auto ret_offset_u256 = stack.pop_unsafe();
  const auto ret_size_u256 = stack.pop_unsafe();

  // Memory offsets and sizes must fit in uint64_t. If they don't, the memory access
  // would require more than 2^64 bytes, which exceeds any possible gas limit.
  // The EVM spec handles this by making such operations cost effectively infinite gas.
  if (!args_offset_u256.fits_uint64() || !args_size_u256.fits_uint64() ||
      !ret_offset_u256.fits_uint64() || !ret_size_u256.fits_uint64()) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }

  result.args_offset = args_offset_u256.to_uint64_unsafe();
  result.args_size = args_size_u256.to_uint64_unsafe();
  result.ret_offset = ret_offset_u256.to_uint64_unsafe();
  result.ret_size = ret_size_u256.to_uint64_unsafe();

  // Check static context violation: CALL with value in static context
  const bool transfers_value = !result.value.is_zero();
  if (is_static && transfers_value) [[unlikely]] {
    result.status = ExecutionStatus::WriteProtection;
    return result;
  }

  // Convert target address
  result.target = types::Address(addr_u256);

  // === Gas Calculation ===
  uint64_t call_gas_cost = 0;

  // 1. EIP-2929: Cold/warm address access cost
  const bool was_cold = accounts.warm_address(result.target);
  call_gas_cost += was_cold ? gas::eip2929::COLD_ACCOUNT : gas::eip2929::WARM_ACCESS;

  // 2. Value transfer cost (if sending value)
  if (transfers_value) {
    call_gas_cost += gas::call::VALUE_TRANSFER;

    // 3. New account cost (if target is empty AND we're sending value)
    if (accounts.is_empty(result.target)) {
      call_gas_cost += gas::call::NEW_ACCOUNT;
    }
  }

  // 4. Memory expansion cost for both input and output regions.
  //    EVM charges expansion once to max(args_end, ret_end), not independently.
  //    Charging independently would overcharge due to quadratic cost formula.
  uint64_t max_mem_end = 0;
  if (result.args_size > 0) {
    uint64_t args_end = 0;
    if (__builtin_add_overflow(result.args_offset, result.args_size, &args_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, args_end);
  }
  if (result.ret_size > 0) {
    uint64_t ret_end = 0;
    if (__builtin_add_overflow(result.ret_offset, result.ret_size, &ret_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, ret_end);
  }

  if (max_mem_end > 0) {
    const uint64_t mem_cost = memory_cost_fn(memory.size(), 0, max_mem_end);
    if (mem_cost == UINT64_MAX) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    call_gas_cost += mem_cost;
  }

  // Deduct call overhead from current frame's gas
  if (gas < call_gas_cost) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }
  gas -= call_gas_cost;

  // 6. Calculate child gas using EIP-150 63/64 rule
  const uint64_t requested_gas =
      requested_gas_u256.fits_uint64() ? requested_gas_u256.to_uint64_unsafe() : UINT64_MAX;
  result.child_gas = gas::call::child_gas(gas, requested_gas);

  // Add gas stipend if transferring value
  if (transfers_value) {
    result.child_gas += gas::call::STIPEND;
  }

  // Deduct child gas from parent (excluding stipend which is "free")
  const uint64_t gas_to_deduct = result.child_gas - (transfers_value ? gas::call::STIPEND : 0);
  gas -= std::min(gas, gas_to_deduct);

  // Expand memory to cover both input and output regions (we already charged for this)
  if (max_mem_end > memory.size()) {
    memory.expand(max_mem_end);
  }

  return result;
}

/**
 * @brief Prepare a STATICCALL operation - like CALL but no value transfer and forced static.
 *
 * Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
 *
 * STATICCALL is always in static context (state modifications forbidden).
 * No value can be transferred (no value argument on stack).
 */
[[nodiscard]] inline CallSetup prepare_staticcall(
    Stack& stack, uint64_t& gas, Memory& memory, state::AccountProvider& accounts,
    const uint16_t current_depth, const gas::MemoryAccessCostFn& memory_cost_fn) noexcept {
  CallSetup result;

  // Check stack underflow (need 6 items - no value argument)
  if (!stack.has_items(6)) [[unlikely]] {
    result.status = ExecutionStatus::StackUnderflow;
    return result;
  }

  // Check call depth limit (EVM max is 1024)
  if (current_depth >= 1024) [[unlikely]] {
    for (size_t i = 0; i < 6; ++i) {
      (void)stack.pop();
    }
    (void)stack.push(types::Uint256::zero());
    result.status = ExecutionStatus::CallDepthExceeded;
    return result;
  }

  // Pop stack arguments (no value for STATICCALL)
  const auto requested_gas_u256 = stack.pop_unsafe();
  const auto addr_u256 = stack.pop_unsafe();
  const auto args_offset_u256 = stack.pop_unsafe();
  const auto args_size_u256 = stack.pop_unsafe();
  const auto ret_offset_u256 = stack.pop_unsafe();
  const auto ret_size_u256 = stack.pop_unsafe();

  // STATICCALL never transfers value
  result.value = types::Uint256::zero();

  if (!args_offset_u256.fits_uint64() || !args_size_u256.fits_uint64() ||
      !ret_offset_u256.fits_uint64() || !ret_size_u256.fits_uint64()) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }

  result.args_offset = args_offset_u256.to_uint64_unsafe();
  result.args_size = args_size_u256.to_uint64_unsafe();
  result.ret_offset = ret_offset_u256.to_uint64_unsafe();
  result.ret_size = ret_size_u256.to_uint64_unsafe();

  result.target = types::Address(addr_u256);

  // === Gas Calculation ===
  uint64_t call_gas_cost = 0;

  // EIP-2929: Cold/warm address access cost
  const bool was_cold = accounts.warm_address(result.target);
  call_gas_cost += was_cold ? gas::eip2929::COLD_ACCOUNT : gas::eip2929::WARM_ACCESS;

  // No value transfer cost for STATICCALL

  // Memory expansion cost
  uint64_t max_mem_end = 0;
  if (result.args_size > 0) {
    uint64_t args_end = 0;
    if (__builtin_add_overflow(result.args_offset, result.args_size, &args_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, args_end);
  }
  if (result.ret_size > 0) {
    uint64_t ret_end = 0;
    if (__builtin_add_overflow(result.ret_offset, result.ret_size, &ret_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, ret_end);
  }

  if (max_mem_end > 0) {
    const uint64_t mem_cost = memory_cost_fn(memory.size(), 0, max_mem_end);
    if (mem_cost == UINT64_MAX) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    call_gas_cost += mem_cost;
  }

  if (gas < call_gas_cost) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }
  gas -= call_gas_cost;

  // Calculate child gas using EIP-150 63/64 rule
  const uint64_t requested_gas =
      requested_gas_u256.fits_uint64() ? requested_gas_u256.to_uint64_unsafe() : UINT64_MAX;
  result.child_gas = gas::call::child_gas(gas, requested_gas);

  // No stipend for STATICCALL (no value transfer)

  gas -= std::min(gas, result.child_gas);

  if (max_mem_end > memory.size()) {
    memory.expand(max_mem_end);
  }

  return result;
}

/**
 * @brief Prepare a DELEGATECALL operation - executes code at target using caller's context.
 *
 * Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
 *
 * DELEGATECALL:
 * - Runs target's code but with caller's storage, caller, value
 * - msg.sender and msg.value are inherited from the current call
 * - No value argument on stack
 */
[[nodiscard]] inline CallSetup prepare_delegatecall(
    Stack& stack, uint64_t& gas, Memory& memory, state::AccountProvider& accounts,
    const uint16_t current_depth, const gas::MemoryAccessCostFn& memory_cost_fn) noexcept {
  CallSetup result;

  // Check stack underflow (need 6 items - no value argument)
  if (!stack.has_items(6)) [[unlikely]] {
    result.status = ExecutionStatus::StackUnderflow;
    return result;
  }

  // Check call depth limit (EVM max is 1024)
  if (current_depth >= 1024) [[unlikely]] {
    for (size_t i = 0; i < 6; ++i) {
      (void)stack.pop();
    }
    (void)stack.push(types::Uint256::zero());
    result.status = ExecutionStatus::CallDepthExceeded;
    return result;
  }

  // Pop stack arguments (no value for DELEGATECALL)
  const auto requested_gas_u256 = stack.pop_unsafe();
  const auto addr_u256 = stack.pop_unsafe();
  const auto args_offset_u256 = stack.pop_unsafe();
  const auto args_size_u256 = stack.pop_unsafe();
  const auto ret_offset_u256 = stack.pop_unsafe();
  const auto ret_size_u256 = stack.pop_unsafe();

  // Value is inherited from current frame (not set here, handled by EVM)
  result.value = types::Uint256::zero();

  if (!args_offset_u256.fits_uint64() || !args_size_u256.fits_uint64() ||
      !ret_offset_u256.fits_uint64() || !ret_size_u256.fits_uint64()) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }

  result.args_offset = args_offset_u256.to_uint64_unsafe();
  result.args_size = args_size_u256.to_uint64_unsafe();
  result.ret_offset = ret_offset_u256.to_uint64_unsafe();
  result.ret_size = ret_size_u256.to_uint64_unsafe();

  // Target is where to get the code from (storage access uses current address)
  result.target = types::Address(addr_u256);

  // === Gas Calculation ===
  uint64_t call_gas_cost = 0;

  // EIP-2929: Cold/warm address access cost
  const bool was_cold = accounts.warm_address(result.target);
  call_gas_cost += was_cold ? gas::eip2929::COLD_ACCOUNT : gas::eip2929::WARM_ACCESS;

  // No value transfer cost for DELEGATECALL

  // Memory expansion cost
  uint64_t max_mem_end = 0;
  if (result.args_size > 0) {
    uint64_t args_end = 0;
    if (__builtin_add_overflow(result.args_offset, result.args_size, &args_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, args_end);
  }
  if (result.ret_size > 0) {
    uint64_t ret_end = 0;
    if (__builtin_add_overflow(result.ret_offset, result.ret_size, &ret_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, ret_end);
  }

  if (max_mem_end > 0) {
    const uint64_t mem_cost = memory_cost_fn(memory.size(), 0, max_mem_end);
    if (mem_cost == UINT64_MAX) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    call_gas_cost += mem_cost;
  }

  if (gas < call_gas_cost) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }
  gas -= call_gas_cost;

  // Calculate child gas using EIP-150 63/64 rule
  const uint64_t requested_gas =
      requested_gas_u256.fits_uint64() ? requested_gas_u256.to_uint64_unsafe() : UINT64_MAX;
  result.child_gas = gas::call::child_gas(gas, requested_gas);

  // No stipend for DELEGATECALL (no value transfer)

  gas -= std::min(gas, result.child_gas);

  if (max_mem_end > memory.size()) {
    memory.expand(max_mem_end);
  }

  return result;
}

/**
 * @brief Prepare a CALLCODE operation - like CALL but runs at caller's address.
 *
 * Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
 *
 * CALLCODE (deprecated, use DELEGATECALL instead):
 * - Runs target's code at caller's address (storage)
 * - msg.sender is set to caller (unlike DELEGATECALL which preserves original sender)
 * - Can transfer value
 */
[[nodiscard]] inline CallSetup prepare_callcode(
    Stack& stack, uint64_t& gas, Memory& memory, state::AccountProvider& accounts,
    const bool is_static, const uint16_t current_depth,
    const gas::MemoryAccessCostFn& memory_cost_fn) noexcept {
  CallSetup result;

  // Check stack underflow (need 7 items)
  if (!stack.has_items(7)) [[unlikely]] {
    result.status = ExecutionStatus::StackUnderflow;
    return result;
  }

  // Check call depth limit (EVM max is 1024)
  if (current_depth >= 1024) [[unlikely]] {
    for (size_t i = 0; i < 7; ++i) {
      (void)stack.pop();
    }
    (void)stack.push(types::Uint256::zero());
    result.status = ExecutionStatus::CallDepthExceeded;
    return result;
  }

  // Pop stack arguments
  const auto requested_gas_u256 = stack.pop_unsafe();
  const auto addr_u256 = stack.pop_unsafe();
  result.value = stack.pop_unsafe();
  const auto args_offset_u256 = stack.pop_unsafe();
  const auto args_size_u256 = stack.pop_unsafe();
  const auto ret_offset_u256 = stack.pop_unsafe();
  const auto ret_size_u256 = stack.pop_unsafe();

  if (!args_offset_u256.fits_uint64() || !args_size_u256.fits_uint64() ||
      !ret_offset_u256.fits_uint64() || !ret_size_u256.fits_uint64()) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }

  result.args_offset = args_offset_u256.to_uint64_unsafe();
  result.args_size = args_size_u256.to_uint64_unsafe();
  result.ret_offset = ret_offset_u256.to_uint64_unsafe();
  result.ret_size = ret_size_u256.to_uint64_unsafe();

  const bool transfers_value = !result.value.is_zero();
  if (is_static && transfers_value) [[unlikely]] {
    result.status = ExecutionStatus::WriteProtection;
    return result;
  }

  // Target is where to get the code from (storage access uses current address)
  result.target = types::Address(addr_u256);

  // === Gas Calculation ===
  uint64_t call_gas_cost = 0;

  // EIP-2929: Cold/warm address access cost
  const bool was_cold = accounts.warm_address(result.target);
  call_gas_cost += was_cold ? gas::eip2929::COLD_ACCOUNT : gas::eip2929::WARM_ACCESS;

  // Value transfer cost (if sending value)
  // Note: CALLCODE does not create new accounts, so no NEW_ACCOUNT cost
  if (transfers_value) {
    call_gas_cost += gas::call::VALUE_TRANSFER;
  }

  // Memory expansion cost
  uint64_t max_mem_end = 0;
  if (result.args_size > 0) {
    uint64_t args_end = 0;
    if (__builtin_add_overflow(result.args_offset, result.args_size, &args_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, args_end);
  }
  if (result.ret_size > 0) {
    uint64_t ret_end = 0;
    if (__builtin_add_overflow(result.ret_offset, result.ret_size, &ret_end)) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    max_mem_end = std::max(max_mem_end, ret_end);
  }

  if (max_mem_end > 0) {
    const uint64_t mem_cost = memory_cost_fn(memory.size(), 0, max_mem_end);
    if (mem_cost == UINT64_MAX) [[unlikely]] {
      result.status = ExecutionStatus::OutOfGas;
      return result;
    }
    call_gas_cost += mem_cost;
  }

  if (gas < call_gas_cost) [[unlikely]] {
    result.status = ExecutionStatus::OutOfGas;
    return result;
  }
  gas -= call_gas_cost;

  // Calculate child gas using EIP-150 63/64 rule
  const uint64_t requested_gas =
      requested_gas_u256.fits_uint64() ? requested_gas_u256.to_uint64_unsafe() : UINT64_MAX;
  result.child_gas = gas::call::child_gas(gas, requested_gas);

  // Add gas stipend if transferring value
  if (transfers_value) {
    result.child_gas += gas::call::STIPEND;
  }

  const uint64_t gas_to_deduct = result.child_gas - (transfers_value ? gas::call::STIPEND : 0);
  gas -= std::min(gas, gas_to_deduct);

  if (max_mem_end > memory.size()) {
    memory.expand(max_mem_end);
  }

  return result;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CONTROL_FLOW_H
