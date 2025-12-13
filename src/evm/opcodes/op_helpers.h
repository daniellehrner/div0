#ifndef DIV0_EVM_OPCODES_OP_HELPERS_H
#define DIV0_EVM_OPCODES_OP_HELPERS_H

#include <cassert>

#include "div0/evm/execution_result.h"
#include "div0/evm/stack.h"

namespace div0::evm::opcodes {
/**
 * @brief Generic handler for unary operations.
 *
 * Handles common validation (stack underflow, gas), performs the operation,
 * and updates the stack. Fully inlines to zero-overhead.
 *
 * @tparam UnaryOp Callable type (lambda, function pointer, functor)
 * @param stack EVM stack
 * @param gas Available gas (will be decremented)
 * @param gas_cost Gas cost for this operation
 * @param op Unary operation: (a) -> result
 * @return ExecutionStatus indicating success or failure
 */
template <typename UnaryOp>
[[gnu::always_inline]] ExecutionStatus unary_static_gas_op(Stack& stack, uint64_t& gas,
                                                           const uint64_t gas_cost,
                                                           const UnaryOp& op) noexcept {
  if (!stack.has_items(1)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  // Apply operation to top value and store result in place
  stack[0] = op(stack[0]);

  return ExecutionStatus::Success;
}

/**
 * @brief Generic handler for binary arithmetic operations.
 *
 * Handles common validation (stack underflow, gas), performs the operation,
 * and updates the stack. Fully inlines to zero-overhead.
 *
 * @tparam BinaryOp Callable type (lambda, function pointer, functor)
 * @param stack EVM stack
 * @param gas Available gas (will be decremented)
 * @param gas_cost Gas cost for this operation
 * @param op Binary operation: (a, b) -> result
 * @return ExecutionStatus indicating success or failure
 */
template <typename BinaryOp>
[[gnu::always_inline]] ExecutionStatus binary_static_gas_op(Stack& stack, uint64_t& gas,
                                                            const uint64_t gas_cost,
                                                            const BinaryOp& op) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  // Apply binary operation: result = op(stack[0], stack[1])
  // For non-commutative ops: this computes (top op second_from_top)
  // Result stored in stack[1], then stack[0] is popped
  stack[1] = op(stack[0], stack[1]);

  // Pop must succeed since we validated HasItems(2) above
  [[maybe_unused]] const bool pop_result = stack.pop();
  assert(pop_result && "Pop should never fail after HasItems check");

  return ExecutionStatus::Success;
}

/**
 * @brief Generic handler for ternary operations (3 operands → 1 result).
 *
 * Used for ADDMOD, MULMOD. Pops 3 values, computes result, pushes 1.
 *
 * @tparam TernaryOp Callable type: (a, b, c) -> result
 * @param stack EVM stack
 * @param gas Available gas (will be decremented)
 * @param gas_cost Gas cost for this operation
 * @param op Ternary operation: (a, b, N) -> result
 * @return ExecutionStatus indicating success or failure
 */
template <typename TernaryOp>
[[gnu::always_inline]] ExecutionStatus ternary_static_gas_op(Stack& stack, uint64_t& gas,
                                                             const uint64_t gas_cost,
                                                             const TernaryOp& op) noexcept {
  if (!stack.has_items(3)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  // Apply ternary operation: result = op(stack[0], stack[1], stack[2])
  // Result stored in stack[2], then pop 2 elements
  stack[2] = op(stack[0], stack[1], stack[2]);

  // Pop must succeed since we validated has_items(3) above
  [[maybe_unused]] const bool pop1 = stack.pop();
  [[maybe_unused]] const bool pop2 = stack.pop();
  assert(pop1 && pop2 && "Pop should never fail after has_items check");

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_OP_HELPERS_H
