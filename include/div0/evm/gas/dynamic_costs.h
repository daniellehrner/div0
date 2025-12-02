#ifndef DIV0_EVM_GAS_DYNAMIC_COSTS_H
#define DIV0_EVM_GAS_DYNAMIC_COSTS_H

#include <algorithm>
#include <cstdint>

#include "div0/types/uint256.h"

namespace div0::evm::gas {

/// EIP-2929: Gas costs for warm/cold access (applies to storage and addresses)
namespace eip2929 {
constexpr uint64_t WARM_ACCESS = 100;    // Warm storage slot or address access
constexpr uint64_t COLD_SLOAD = 2100;    // Cold storage slot read
constexpr uint64_t COLD_ACCOUNT = 2600;  // Cold account/address access
}  // namespace eip2929

using SstoreCostFn = uint64_t (*)(bool is_warm, const types::Uint256& current_value,
                                  const types::Uint256& original_value,
                                  const types::Uint256& new_value);

using SloadCostFn = uint64_t (*)(bool is_warm);

using ExpCostFn = uint64_t (*)(const types::Uint256& exponent);

using MemoryAccessCostFn = uint64_t (*)(uint64_t current_size, uint64_t offset, uint64_t length);

namespace sload {

/// EIP-2929 access list gas costs for SLOAD
inline uint64_t eip2929(const bool is_warm) {
  return is_warm ? eip2929::WARM_ACCESS : eip2929::COLD_SLOAD;
}

}  // namespace sload

namespace sstore {

/// EIP-2929 + EIP-2200 gas costs for SSTORE
inline uint64_t eip2929(const bool is_warm, const types::Uint256& current,
                        const types::Uint256& original, const types::Uint256& new_val) {
  const uint64_t base_cost = is_warm ? eip2929::WARM_ACCESS : eip2929::COLD_SLOAD;

  if (current == new_val) {
    return base_cost;
  }

  if (original == current) {
    return (original.is_zero()) ? 20000 : 2900;
  }

  return base_cost;
}

}  // namespace sstore

namespace exp {

/// EIP-160 gas costs for EXP opcode
/// Cost = 10 (base) + 50 * byte_length(exponent)
inline uint64_t eip160(const types::Uint256& exponent) {
  constexpr uint64_t BASE_COST = 10;
  constexpr uint64_t COST_PER_BYTE = 50;
  return BASE_COST + (COST_PER_BYTE * exponent.byte_length());
}

}  // namespace exp

namespace memory {

/**
 * @brief Calculate memory cost for a given word count.
 *
 * EVM memory cost formula: 3 * words + words² / 512
 * Returns UINT64_MAX on overflow (signals OutOfGas to caller).
 *
 * @param words Number of 32-byte words
 * @return Total memory cost in gas, or UINT64_MAX on overflow
 */
[[nodiscard]] inline uint64_t cost_for_words(uint64_t words) noexcept {
  // Calculate 3 * words with overflow check
  uint64_t linear_cost = 0;
  if (__builtin_mul_overflow(3ULL, words, &linear_cost)) [[unlikely]] {
    return UINT64_MAX;
  }

  // Calculate words * words with overflow check
  uint64_t words_squared = 0;
  if (__builtin_mul_overflow(words, words, &words_squared)) [[unlikely]] {
    return UINT64_MAX;
  }

  // words² / 512 cannot overflow (division only decreases)
  const uint64_t quadratic_cost = words_squared / 512;

  // Calculate total with overflow check
  uint64_t total = 0;
  if (__builtin_add_overflow(linear_cost, quadratic_cost, &total)) [[unlikely]] {
    return UINT64_MAX;
  }

  return total;
}

/**
 * @brief Calculate gas cost for memory expansion.
 *
 * Returns the incremental cost to expand from current_words to new_words.
 * Returns 0 if no expansion is needed.
 * Returns UINT64_MAX on overflow (signals OutOfGas to caller).
 *
 * @param current_words Current memory size in 32-byte words
 * @param new_words Required memory size in 32-byte words
 * @return Expansion gas cost (0 if within bounds, UINT64_MAX on overflow)
 */
[[nodiscard]] inline uint64_t expansion_cost(uint64_t current_words, uint64_t new_words) noexcept {
  if (new_words <= current_words) [[likely]] {
    return 0;
  }

  const uint64_t new_cost = cost_for_words(new_words);
  if (new_cost == UINT64_MAX) [[unlikely]] {
    return UINT64_MAX;
  }

  // current_cost cannot overflow if new_cost didn't (current_words < new_words)
  const uint64_t current_cost = cost_for_words(current_words);

  return new_cost - current_cost;
}

/**
 * @brief Calculate gas cost for accessing memory at [offset, offset+length).
 *
 * Computes the expansion cost if the access would exceed current memory size.
 * Returns UINT64_MAX on overflow (signals OutOfGas to caller).
 *
 * @param current_size Current memory size in bytes
 * @param offset Access starting offset
 * @param length Number of bytes to access
 * @return Expansion gas cost (0 if within bounds, UINT64_MAX on overflow)
 */
[[nodiscard]] inline uint64_t access_cost(uint64_t current_size, uint64_t offset,
                                          uint64_t length) noexcept {
  if (length == 0) [[unlikely]] {
    return 0;
  }

  // Check for overflow: offset + length
  uint64_t end_offset = 0;
  if (__builtin_add_overflow(offset, length, &end_offset)) [[unlikely]] {
    return UINT64_MAX;  // Signal overflow → will cause OutOfGas
  }

  // Current words (memory is always word-aligned)
  const uint64_t current_words = current_size / 32;

  // Required words (round up): ceil(end_offset / 32)
  uint64_t rounded_end_offset = 0;
  if (__builtin_add_overflow(end_offset, 31, &rounded_end_offset)) [[unlikely]] {
    return UINT64_MAX;  // Signal overflow → will cause OutOfGas
  }
  const uint64_t new_words = rounded_end_offset / 32;

  return expansion_cost(current_words, new_words);
}

}  // namespace memory

namespace call {

/// Gas constants for CALL family opcodes
constexpr uint64_t VALUE_TRANSFER = 9000;  // Cost for non-zero value transfer
constexpr uint64_t NEW_ACCOUNT = 25000;    // Cost for creating new account (empty + value)
constexpr uint64_t STIPEND = 2300;         // Gas stipend added when value is non-zero

/**
 * @brief Calculate gas to be allocated to child call (EIP-150 63/64 rule).
 *
 * The caller can allocate at most 63/64 of remaining gas to the child.
 * If explicitly requested gas is less, use that.
 *
 * @param available_gas Gas remaining after deducting call overhead
 * @param requested_gas Gas explicitly requested by the CALL opcode
 * @return Gas to allocate to the child call
 */
[[nodiscard]] inline uint64_t child_gas(uint64_t available_gas, uint64_t requested_gas) noexcept {
  // EIP-150: max child gas = available - available/64 = available * 63/64
  const uint64_t max_child_gas = available_gas - (available_gas / 64);
  return std::min(requested_gas, max_child_gas);
}

}  // namespace call

}  // namespace div0::evm::gas

#endif  // DIV0_EVM_GAS_DYNAMIC_COSTS_H
