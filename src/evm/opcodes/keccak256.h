#ifndef DIV0_EVM_OPCODES_KECCAK256_H
#define DIV0_EVM_OPCODES_KECCAK256_H

#include <cassert>

#include "div0/crypto/keccak256.h"
#include "div0/evm/execution_result.h"
#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"

namespace div0::evm::opcodes {

/**
 * @brief SHA3 (KECCAK256) - Compute Keccak-256 hash of memory region.
 *
 * Stack: [offset, length] => [hash]
 * Gas: 30 (static) + 6 * ceil(length / 32) (word cost) + memory expansion cost
 *
 * Reads `length` bytes starting at `offset` from memory, computes the
 * Keccak-256 hash, and pushes the 256-bit result onto the stack.
 *
 * Special case: length=0 returns hash of empty input (known constant).
 */
[[gnu::always_inline]] inline ExecutionStatus keccak256(Stack& stack, uint64_t& gas,
                                                        const uint64_t static_cost,
                                                        const uint64_t word_cost,
                                                        const gas::MemoryAccessCostFn& mem_cost_fn,
                                                        Memory& memory) noexcept {
  if (!stack.has_items(2)) [[unlikely]] {
    return ExecutionStatus::StackUnderflow;
  }

  // EVM uses Uint256 for offset/length, but practical memory addressing uses uint64.
  // Values > 2^64 would require gas far exceeding any block gas limit:
  // - Memory cost grows quadratically: 3*words + words²/512
  // - At 2^64 bytes (~18 exabytes), cost would be astronomically high
  // We treat conversion failure as OutOfGas since the operation is economically impossible.
  uint64_t offset;
  uint64_t length;
  if (!stack[0].to_uint64(offset)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (!stack[1].to_uint64(length)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate word cost: word_cost * ceil(length / 32)
  uint64_t words_numerator;
  if (__builtin_add_overflow(length, 31, &words_numerator)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  const uint64_t words = words_numerator / 32;
  uint64_t dynamic_cost;
  if (__builtin_mul_overflow(word_cost, words, &dynamic_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  // Calculate memory expansion cost (0 if length == 0)
  uint64_t mem_cost = 0;
  if (length > 0) {
    // Check offset + length overflow
    if (offset > UINT64_MAX - length) [[unlikely]] {
      return ExecutionStatus::OutOfGas;
    }
    mem_cost = mem_cost_fn(memory.size(), offset, length);
    if (mem_cost == UINT64_MAX) [[unlikely]] {
      return ExecutionStatus::OutOfGas;
    }
  }

  // Total cost = static + word_cost * words + memory_expansion
  uint64_t total_cost;
  if (__builtin_add_overflow(static_cost, dynamic_cost, &total_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (__builtin_add_overflow(total_cost, mem_cost, &total_cost)) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  if (gas < total_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  gas -= total_cost;

  // Compute hash
  types::Uint256 hash;
  if (length == 0) {
    // Hash of empty input is a known constant
    // keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
    hash = crypto::keccak256(std::span<const uint8_t>{});
  } else {
    // Expand memory if needed
    memory.expand(offset + length);

    // Get span of memory to hash
    const auto data = memory.read_span_unsafe(offset, length);
    hash = crypto::keccak256(data);
  }

  // Pop offset and length, push hash
  // Replace stack[1] with hash, then pop stack[0]
  stack[1] = hash;
  [[maybe_unused]] const bool pop_result = stack.pop();
  assert(pop_result && "Pop must never fail after has_items check");

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_KECCAK256_H
