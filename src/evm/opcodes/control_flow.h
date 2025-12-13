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

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CONTROL_FLOW_H
