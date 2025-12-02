#ifndef DIV0_EVM_OPCODES_DUP_H
#define DIV0_EVM_OPCODES_DUP_H

#include <cassert>

#include "div0/evm/execution_result.h"
#include "div0/evm/stack.h"

namespace div0::evm::opcodes {

[[gnu::always_inline]] inline ExecutionStatus dup_n(Stack& stack, uint64_t& gas,
                                                    const uint64_t gas_cost, const size_t n) {
  assert(n >= 1 && n <= 16 && "DUP size must be 1-16");

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  if (!stack.has_items(n)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  gas -= gas_cost;

  stack.dup_unsafe(n);

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_DUP_H
