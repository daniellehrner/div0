#ifndef DIV0_EVM_OPCODES_STORAGE_H
#define DIV0_EVM_OPCODES_STORAGE_H

#include "div0/evm/execution_result.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/stack.h"
#include "div0/state/storage_provider.h"
#include "div0/types/address.h"
#include "div0/types/storage_slot.h"
#include "div0/types/storage_value.h"

namespace div0::evm::opcodes {

/**
 * @brief SLOAD - Load from storage.
 *
 * Stack: [key] => [value]
 */
[[gnu::always_inline]] inline ExecutionStatus sload(types::Address address, Stack& stack,
                                                    uint64_t& gas, state::StorageProvider& storage,
                                                    const gas::SloadCostFn& cost_fn) noexcept {
  if (!stack.has_items(1)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  const types::StorageSlot slot{stack[0]};
  const auto cost = cost_fn(storage.is_warm(address, slot));

  if (gas < cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= cost;

  stack[0] = storage.load(address, slot).get();

  return ExecutionStatus::Success;
}

/**
 * @brief SSTORE opcode (0x55) - Store to storage.
 *
 * Stack: [key, value] => []
 * Gas: 100 (simplified - should use dynamic gas based on storage state)
 */
[[gnu::always_inline]] inline ExecutionStatus sstore(const types::Address& address, Stack& stack,
                                                     uint64_t& gas, state::StorageProvider& storage,
                                                     const gas::SstoreCostFn& cost_fn) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  const auto cost =
      cost_fn(false, types::Uint256::zero(), types::Uint256::zero(), types::Uint256::zero());

  // Check gas and stack
  if (gas < cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Deduct gas
  gas -= cost;

  // Pop key and value, store
  const types::StorageSlot slot{stack[0]};
  const types::StorageValue value{stack[1]};

  storage.store(address, slot, value);

  // Pop both values
  [[maybe_unused]] const bool pop1_result = stack.pop();
  [[maybe_unused]] const bool pop2_result = stack.pop();
  assert(pop1_result && pop2_result && "Pop must never fail after has_items check");

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_STORAGE_H
