#ifndef DIV0_EVM_OPCODES_CONTEXT_H
#define DIV0_EVM_OPCODES_CONTEXT_H

#include "div0/evm/execution_result.h"
#include "div0/evm/stack.h"
#include "div0/types/address.h"

namespace div0::evm::opcodes {

/**
 * @brief CALLER - Get caller address (msg.sender).
 *
 * Stack: [] => [caller]
 * Gas: 2
 *
 * Pushes the address of the account that is directly responsible for this execution.
 * This is the immediate sender of the message (which may be different from the
 * transaction origin in the case of internal calls).
 *
 * @param stack EVM stack
 * @param gas Remaining gas (decremented by gas_cost)
 * @param gas_cost Static gas cost for this opcode
 * @param caller The caller address to push (frame.caller)
 * @return ExecutionStatus::Success or error status
 */
[[gnu::always_inline]] inline ExecutionStatus caller(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost,
                                                     const types::Address& caller) noexcept {
  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  gas -= gas_cost;

  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  (void)stack.push(caller.to_uint256());
  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CONTEXT_H
