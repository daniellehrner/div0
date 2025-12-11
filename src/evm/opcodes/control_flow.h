#ifndef DIV0_EVM_OPCODES_CONTROL_FLOW_H
#define DIV0_EVM_OPCODES_CONTROL_FLOW_H

#include "op_helpers.h"

namespace div0::evm::opcodes {

// =============================================================================
// Stack Operations
// =============================================================================

/// POP opcode: Remove the top item from the stack
[[gnu::always_inline]] inline ExecutionStatus pop(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost) noexcept {
  if (!stack.has_items(1)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;
  [[maybe_unused]] const bool result = stack.pop();
  return ExecutionStatus::Success;
}

// =============================================================================
// Program Counter Operations
// =============================================================================

/// PC opcode: Push the program counter (before this instruction)
/// Note: The pc passed should be the value BEFORE incrementing for this instruction
[[gnu::always_inline]] inline ExecutionStatus pc(Stack& stack, uint64_t& gas,
                                                 const uint64_t gas_cost,
                                                 uint64_t pc_value) noexcept {
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;
  [[maybe_unused]] const bool result = stack.push(types::Uint256(pc_value));
  return ExecutionStatus::Success;
}

/// GAS opcode: Push remaining gas (after deducting for this instruction)
[[gnu::always_inline]] inline ExecutionStatus op_gas(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost) noexcept {
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;
  // Push remaining gas after deduction
  [[maybe_unused]] const bool result = stack.push(types::Uint256(gas));
  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CONTROL_FLOW_H