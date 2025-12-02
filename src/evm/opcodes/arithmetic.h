#ifndef DIV0_EVM_OPCODES_ARITHMETIC_H
#define DIV0_EVM_OPCODES_ARITHMETIC_H

#include "div0/evm/gas/dynamic_costs.h"
#include "div0/types/uint256.h"
#include "op_helpers.h"

namespace div0::evm::opcodes {

[[gnu::always_inline]] inline ExecutionStatus add(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a + b; });
}

[[gnu::always_inline]] inline ExecutionStatus mul(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a * b; });
}

[[gnu::always_inline]] inline ExecutionStatus sub(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a - b; });
}

[[gnu::always_inline]] inline ExecutionStatus div(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a / b; });
}

[[gnu::always_inline]] inline ExecutionStatus mod(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a % b; });
}

[[gnu::always_inline]] inline ExecutionStatus sdiv(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& a, const types::Uint256& b) { return types::Uint256::sdiv(a, b); });
}

[[gnu::always_inline]] inline ExecutionStatus smod(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& a, const types::Uint256& b) { return types::Uint256::smod(a, b); });
}

[[gnu::always_inline]] inline ExecutionStatus addmod(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost) noexcept {
  return ternary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& a, const types::Uint256& b, const types::Uint256& n) {
        return types::Uint256::addmod(a, b, n);
      });
}

[[gnu::always_inline]] inline ExecutionStatus mulmod(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost) noexcept {
  return ternary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& a, const types::Uint256& b, const types::Uint256& n) {
        return types::Uint256::mulmod(a, b, n);
      });
}

[[gnu::always_inline]] inline ExecutionStatus signextend(Stack& stack, uint64_t& gas,
                                                         const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& byte_pos, const types::Uint256& x) {
                                return types::Uint256::signextend(byte_pos, x);
                              });
}

/// EXP opcode with dynamic gas cost based on exponent byte length
[[gnu::always_inline]] inline ExecutionStatus exp(Stack& stack, uint64_t& gas,
                                                  div0::evm::gas::ExpCostFn gas_fn) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // Compute dynamic gas based on exponent (stack[1])
  const uint64_t gas_cost = gas_fn(stack[1]);
  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  stack[1] = types::Uint256::exp(stack[0], stack[1]);

  [[maybe_unused]] const bool pop_result = stack.pop();
  assert(pop_result && "Pop should never fail after has_items check");

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_ARITHMETIC_H
