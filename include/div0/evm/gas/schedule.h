#ifndef DIV0_EVM_GAS_SCHEDULE_H
#define DIV0_EVM_GAS_SCHEDULE_H

#include <array>
#include <cstdint>

#include "div0/evm/gas/dynamic_costs.h"

namespace div0::evm::gas {

/**
 * @brief Gas cost schedule for a specific EVM fork.
 *
 * The gas schedule defines the cost of every operation in the EVM. Costs vary
 * by fork version as new opcodes are introduced and optimizations are made.
 *
 * GAS COST CATEGORIES:
 * ====================
 * - **Static costs**: Fixed gas cost per opcode (e.g., ADD always costs 3 gas)
 * - **Dynamic costs**: Context-dependent costs (e.g., SSTORE depends on storage state)
 * - **Transaction costs**: Base costs for transaction execution
 *
 * ORGANIZATION:
 * =============
 * Static costs are stored in an array indexed by opcode byte value (0x00-0xFF).
 * Dynamic costs use function pointers that are called at execution time.
 *
 * @note Each fork has its own Schedule instance
 */
struct Schedule {
  /// Static gas costs indexed by opcode (0x00-0xFF)
  /// Example: static_costs[0x01] = 3 (ADD costs 3 gas)
  std::array<std::uint64_t, 256> static_costs;

  /// Dynamic cost function for SLOAD opcode
  /// Called at runtime with warm/cold access information
  SloadCostFn sload;

  /// Dynamic cost function for SSTORE opcode
  /// Called at runtime with storage state information
  SstoreCostFn sstore;

  /// Dynamic cost function for EXP opcode
  /// Called at runtime with exponent value to compute byte-based cost
  ExpCostFn exp;

  /// Dynamic cost function for memory access
  /// Called at runtime to compute memory expansion cost
  MemoryAccessCostFn memory_access;

  /// Per-word cost for KECCAK256 (SHA3) opcode
  /// Cost = static_cost + sha3_word_cost * ceil(length / 32) + memory_expansion
  uint64_t sha3_word_cost;

  /// Base cost for any transaction
  uint64_t tx_base_cost;
};

}  // namespace div0::evm::gas

#endif  // DIV0_EVM_GAS_SCHEDULE_H
