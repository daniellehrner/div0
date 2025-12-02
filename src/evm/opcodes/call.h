#ifndef DIV0_EVM_OPCODES_CALL_H
#define DIV0_EVM_OPCODES_CALL_H

#include <cstdint>

#include "div0/evm/execution_result.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/state/account_provider.h"
#include "div0/types/address.h"

namespace div0::evm::opcodes {

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

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CALL_H
