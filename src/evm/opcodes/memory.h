#ifndef DIV0_EVM_OPCODES_MEMORY_H
#define DIV0_EVM_OPCODES_MEMORY_H

#include <cassert>

#include "div0/evm/execution_result.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"

namespace div0::evm::opcodes {

/**
 * @brief MLOAD - Load 32-byte word from memory.
 *
 * Stack: [offset] => [value]
 * Gas: 3 (static) + memory expansion cost
 *
 * Reads 32 bytes starting at offset as big-endian Uint256.
 * Memory is zero-extended if offset + 32 exceeds current size.
 */
[[gnu::always_inline]] inline ExecutionStatus mload(Stack& stack, uint64_t& gas,
                                                    const uint64_t static_cost,
                                                    const gas::MemoryAccessCostFn& cost_fn,
                                                    Memory& memory) noexcept {
  if (!stack.has_items(1)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // EVM offsets are Uint256, but practical memory addressing uses uint64.
  // An offset > 2^64 would require gas far exceeding any block gas limit
  // (memory cost grows quadratically), so we treat conversion failure as OutOfGas.
  uint64_t offset;
  if (!stack[0].to_uint64(offset)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Prevent overflow when computing offset + 32 for bounds check.
  // If offset > UINT64_MAX - 32, then offset + 32 would wrap around.
  if (offset > UINT64_MAX - 32) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate memory expansion cost. access_cost returns UINT64_MAX on overflow
  // (e.g., when computing end_offset would overflow), signaling impossible gas.
  const uint64_t mem_cost = cost_fn(memory.size(), offset, 32);
  if (mem_cost == UINT64_MAX) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  uint64_t total_cost;
  if (__builtin_add_overflow(static_cost, mem_cost, &total_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (gas < total_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= total_cost;

  // Expand memory if needed (may throw std::bad_alloc on OOM)
  memory.expand(offset + 32);

  // Load word and replace stack top
  stack[0] = memory.load_word_unsafe(offset);

  return ExecutionStatus::Success;
}

/**
 * @brief MSTORE - Store 32-byte word to memory.
 *
 * Stack: [offset, value] => []
 * Gas: 3 (static) + memory expansion cost
 *
 * Stores value as 32 big-endian bytes at offset.
 */
[[gnu::always_inline]] inline ExecutionStatus mstore(Stack& stack, uint64_t& gas,
                                                     const uint64_t static_cost,
                                                     const gas::MemoryAccessCostFn& cost_fn,
                                                     Memory& memory) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // EVM offsets are Uint256, but practical memory addressing uses uint64.
  // An offset > 2^64 would require gas far exceeding any block gas limit
  // (memory cost grows quadratically), so we treat conversion failure as OutOfGas.
  uint64_t offset;
  if (!stack[0].to_uint64(offset)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Prevent overflow when computing offset + 32 for bounds check.
  // If offset > UINT64_MAX - 32, then offset + 32 would wrap around.
  if (offset > UINT64_MAX - 32) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate memory expansion cost. cost_fn returns UINT64_MAX on overflow
  // (e.g., when computing end_offset would overflow), signaling impossible gas.
  const uint64_t mem_cost = cost_fn(memory.size(), offset, 32);
  if (mem_cost == UINT64_MAX) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  uint64_t total_cost;
  if (__builtin_add_overflow(static_cost, mem_cost, &total_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (gas < total_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= total_cost;

  // Expand memory if needed (may throw std::bad_alloc on OOM)
  memory.expand(offset + 32);

  // Store word
  memory.store_word_unsafe(offset, stack[1]);

  // Pop both values
  [[maybe_unused]] const bool pop1 = stack.pop();
  [[maybe_unused]] const bool pop2 = stack.pop();
  assert(pop1 && pop2 && "Pop must never fail after has_items check");

  return ExecutionStatus::Success;
}

/**
 * @brief MSTORE8 - Store single byte to memory.
 *
 * Stack: [offset, value] => []
 * Gas: 3 (static) + memory expansion cost
 *
 * Stores least significant byte of value at offset.
 */
[[gnu::always_inline]] inline ExecutionStatus mstore8(Stack& stack, uint64_t& gas,
                                                      const uint64_t static_cost,
                                                      const gas::MemoryAccessCostFn& cost_fn,
                                                      Memory& memory) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // EVM offsets are Uint256, but practical memory addressing uses uint64.
  // An offset > 2^64 would require gas far exceeding any block gas limit
  // (memory cost grows quadratically), so we treat conversion failure as OutOfGas.
  uint64_t offset;
  if (!stack[0].to_uint64(offset)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate memory expansion cost for 1 byte.
  // cost_fn returns UINT64_MAX on overflow, signaling impossible gas.
  const uint64_t mem_cost = cost_fn(memory.size(), offset, 1);
  if (mem_cost == UINT64_MAX) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  uint64_t total_cost;
  if (__builtin_add_overflow(static_cost, mem_cost, &total_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (gas < total_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= total_cost;

  // Expand memory if needed (may throw std::bad_alloc on OOM)
  memory.expand(offset + 1);

  // Store least significant byte
  memory.store_byte_unsafe(offset, stack[1].lsb());

  // Pop both values
  [[maybe_unused]] const bool pop1 = stack.pop();
  [[maybe_unused]] const bool pop2 = stack.pop();
  assert(pop1 && pop2 && "Pop must never fail after has_items check");

  return ExecutionStatus::Success;
}

/**
 * @brief MSIZE - Get current memory size in bytes.
 *
 * Stack: [] => [size]
 * Gas: 2 (static only, no memory expansion)
 *
 * Returns size as multiple of 32 (word-aligned).
 */
[[gnu::always_inline]] inline ExecutionStatus msize(Stack& stack, uint64_t& gas,
                                                    const uint64_t static_cost,
                                                    const Memory& memory) noexcept {
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  if (gas < static_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= static_cost;

  [[maybe_unused]] const bool push_result = stack.push(types::Uint256(memory.size()));
  assert(push_result && "Push must never fail after has_space check");

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_MEMORY_H
