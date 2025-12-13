#ifndef DIV0_EVM_OPCODES_CONTEXT_H
#define DIV0_EVM_OPCODES_CONTEXT_H

#include <algorithm>
#include <cstring>
#include <span>

#include "div0/evm/execution_result.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/types/address.h"
#include "div0/types/bytes32.h"

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

// =============================================================================
// Call Data Operations
// =============================================================================

/**
 * @brief CALLDATALOAD - Load 32 bytes of call data onto stack.
 *
 * Stack: [offset] => [data]
 * Gas: 3
 *
 * Reads 32 bytes from call data starting at offset. Bytes beyond the call data
 * size are treated as zeros (right-padded).
 *
 * @param stack EVM stack
 * @param gas Remaining gas
 * @param gas_cost Static gas cost
 * @param calldata Call data span
 * @return ExecutionStatus
 */
[[gnu::always_inline]] inline ExecutionStatus calldataload(
    Stack& stack, uint64_t& gas, const uint64_t gas_cost,
    std::span<const uint8_t> calldata) noexcept {
  if (!stack.has_items(1)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  const types::Uint256& offset_val = stack[0];

  // If offset doesn't fit in uint64 or is beyond calldata, result is zero
  if (!offset_val.fits_uint64()) {
    stack[0] = types::Uint256::zero();
    return ExecutionStatus::Success;
  }

  const uint64_t offset = offset_val.to_uint64_unsafe();

  // If completely beyond calldata bounds, result is zero
  if (offset >= calldata.size()) {
    stack[0] = types::Uint256::zero();
    return ExecutionStatus::Success;
  }

  // Get subspan from offset (may be less than 32 bytes)
  const auto remaining = calldata.subspan(offset);
  // Use Bytes32::from_span_padded which handles the zero-padding
  const types::Bytes32 word = types::Bytes32::from_span_padded(remaining);

  stack[0] = word.to_uint256();
  return ExecutionStatus::Success;
}

/**
 * @brief CALLDATASIZE - Get size of call data.
 *
 * Stack: [] => [size]
 * Gas: 2
 *
 * Pushes the byte size of the call data.
 *
 * @param stack EVM stack
 * @param gas Remaining gas
 * @param gas_cost Static gas cost
 * @param calldata_size Size of call data
 * @return ExecutionStatus
 */
[[gnu::always_inline]] inline ExecutionStatus calldatasize(Stack& stack, uint64_t& gas,
                                                           const uint64_t gas_cost,
                                                           size_t calldata_size) noexcept {
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;
  (void)stack.push(types::Uint256(calldata_size));
  return ExecutionStatus::Success;
}

/**
 * @brief CALLDATACOPY - Copy call data to memory.
 *
 * Stack: [destOffset, offset, size] => []
 * Gas: 3 + 3 * ceil(size/32) + memory_expansion_cost
 *
 * Copies size bytes from call data at offset to memory at destOffset.
 * Bytes beyond call data are padded with zeros.
 *
 * @param stack EVM stack
 * @param gas Remaining gas
 * @param gas_cost Base gas cost (3)
 * @param memory_cost_fn Memory expansion cost calculator
 * @param memory EVM memory
 * @param calldata Call data span
 * @return ExecutionStatus
 */
template <typename MemoryCostFn>
[[gnu::always_inline]] inline ExecutionStatus calldatacopy(
    Stack& stack, uint64_t& gas, const uint64_t gas_cost, MemoryCostFn memory_cost_fn,
    Memory& memory, std::span<const uint8_t> calldata) noexcept {
  if (!stack.has_items(3)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // Get stack values: destOffset (top), offset (2nd), size (3rd)
  const types::Uint256& dest_offset_val = stack[0];
  const types::Uint256& src_offset_val = stack[1];
  const types::Uint256& size_val = stack[2];

  // Check for zero-size copy (no memory expansion needed)
  if (size_val.is_zero()) {
    if (gas < gas_cost) [[unlikely]] {
      return ExecutionStatus::OutOfGas;
    }
    gas -= gas_cost;
    [[maybe_unused]] const bool p1 = stack.pop();
    [[maybe_unused]] const bool p2 = stack.pop();
    [[maybe_unused]] const bool p3 = stack.pop();
    return ExecutionStatus::Success;
  }

  // Size must fit in uint64 for practical purposes
  if (!size_val.fits_uint64()) [[unlikely]] {
    return ExecutionStatus::OutOfGas;  // Would require impossibly large memory
  }
  const uint64_t size = size_val.to_uint64_unsafe();

  // Destination offset must fit in uint64
  if (!dest_offset_val.fits_uint64()) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  const uint64_t dest_offset = dest_offset_val.to_uint64_unsafe();

  // Check for overflow in dest_offset + size
  if (dest_offset > UINT64_MAX - size) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate memory expansion cost
  const uint64_t memory_expansion = memory_cost_fn(memory.size(), dest_offset, size);

  // Calculate word cost: 3 * ceil(size/32)
  const uint64_t word_cost = 3 * ((size + 31) / 32);

  // Check for overflow in total cost
  const uint64_t total_cost = gas_cost + word_cost + memory_expansion;
  if (total_cost < gas_cost || total_cost < word_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  if (gas < total_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= total_cost;

  // Pop stack values
  [[maybe_unused]] const bool p1 = stack.pop();
  [[maybe_unused]] const bool p2 = stack.pop();
  [[maybe_unused]] const bool p3 = stack.pop();

  // Expand memory if needed
  const uint64_t new_size = dest_offset + size;
  memory.expand(new_size);

  // Get destination span
  auto dest = memory.write_span_unsafe(dest_offset, size);

  // Source offset handling
  uint64_t src_offset = 0;
  bool src_in_bounds = false;
  if (src_offset_val.fits_uint64()) {
    src_offset = src_offset_val.to_uint64_unsafe();
    src_in_bounds = src_offset < calldata.size();
  }

  if (src_in_bounds) {
    // Copy what we can from calldata
    const size_t bytes_available = calldata.size() - src_offset;
    const size_t bytes_to_copy = std::min(bytes_available, static_cast<size_t>(size));
    std::memcpy(dest.data(), calldata.data() + src_offset, bytes_to_copy);

    // Zero-fill the rest
    if (bytes_to_copy < size) {
      std::memset(dest.data() + bytes_to_copy, 0, size - bytes_to_copy);
    }
  } else {
    // Source is entirely out of bounds, fill with zeros
    std::memset(dest.data(), 0, size);
  }

  return ExecutionStatus::Success;
}

// =============================================================================
// Program Counter and Gas Operations
// =============================================================================

/**
 * @brief PC - Get the value of the program counter.
 *
 * Stack: [] => [pc]
 * Gas: 2
 *
 * Pushes the value of the program counter prior to the increment corresponding
 * to this instruction. This is the offset of this PC instruction in the code.
 *
 * @param stack EVM stack
 * @param gas Remaining gas
 * @param gas_cost Static gas cost
 * @param pc Current program counter value (before this instruction)
 * @return ExecutionStatus
 */
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

  (void)stack.push(types::Uint256(pc_value));
  return ExecutionStatus::Success;
}

/**
 * @brief GAS - Get the amount of remaining gas.
 *
 * Stack: [] => [gas]
 * Gas: 2
 *
 * Pushes the amount of gas remaining after this operation (gas after deducting
 * the cost of this instruction).
 *
 * @param stack EVM stack
 * @param gas Remaining gas (will be decremented)
 * @param gas_cost Static gas cost
 * @return ExecutionStatus
 */
[[gnu::always_inline]] inline ExecutionStatus gas_op(Stack& stack, uint64_t& gas,
                                                     const uint64_t gas_cost) noexcept {
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= gas_cost;

  // Push the remaining gas AFTER deducting the cost of this instruction
  (void)stack.push(types::Uint256(gas));
  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_CONTEXT_H
