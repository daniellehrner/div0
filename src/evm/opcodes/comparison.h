#ifndef DIV0_EVM_OPCODES_COMPARISON_H
#define DIV0_EVM_OPCODES_COMPARISON_H

#include "op_helpers.h"

namespace div0::evm::opcodes {

[[gnu::always_inline]] inline ExecutionStatus lt(Stack& stack, uint64_t& gas,
                                                 const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& a, const types::Uint256& b) {
                                return (a < b) ? types::Uint256(1) : types::Uint256(0);
                              });
}

[[gnu::always_inline]] inline ExecutionStatus gt(Stack& stack, uint64_t& gas,
                                                 const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& a, const types::Uint256& b) {
                                return (a > b) ? types::Uint256(1) : types::Uint256(0);
                              });
}

[[gnu::always_inline]] inline ExecutionStatus eq(Stack& stack, uint64_t& gas,
                                                 const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& a, const types::Uint256& b) {
                                return (a == b) ? types::Uint256(1) : types::Uint256(0);
                              });
}

[[gnu::always_inline]] inline ExecutionStatus is_zero(Stack& stack, uint64_t& gas,
                                                      const uint64_t gas_cost) noexcept {
  return unary_static_gas_op(stack, gas, gas_cost, [](const types::Uint256& a) {
    return a.is_zero() ? types::Uint256(1) : types::Uint256(0);
  });
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_COMPARISON_H
