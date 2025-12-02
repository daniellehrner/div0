#ifndef DIV0_EVM_OPCODES_SWAP_N_H
#define DIV0_EVM_OPCODES_SWAP_N_H

#include <cassert>

#include "div0/evm/execution_result.h"
#include "div0/evm/stack.h"

namespace div0::evm::opcodes {

[[gnu::always_inline]] inline ExecutionStatus swap_n(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost, const size_t n) {
  assert(n >= 1 && n <= 16 && "SWAP size must be 1-16");

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  if (!stack.swap(n)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  gas -= gas_cost;

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_SWAP_N_H
