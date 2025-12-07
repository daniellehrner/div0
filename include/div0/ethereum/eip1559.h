#ifndef DIV0_ETHEREUM_EIP1559_H
#define DIV0_ETHEREUM_EIP1559_H

#include <cstdint>

#include "div0/types/uint256.h"

namespace div0::ethereum {

/// EIP-1559 protocol constants (mainnet)
namespace eip1559 {
/// Default base fee change denominator
constexpr uint64_t BASE_FEE_CHANGE_DENOMINATOR = 8;

/// Default elasticity multiplier (gas target = gas limit / 2)
constexpr uint64_t ELASTICITY_MULTIPLIER = 2;

/// Initial base fee for the first EIP-1559 block (1 gwei)
constexpr uint64_t INITIAL_BASE_FEE = 1'000'000'000;

/// Minimum protocol base fee (7 wei)
/// This is the lowest possible base fee under mainnet EIP-1559 parameters.
/// Since BASE_FEE_CHANGE_DENOMINATOR is 8 (12.5% change), 12.5% of 7 is < 1
/// and rounds down to 0, so the base fee cannot decrease below 7.
constexpr uint64_t MIN_PROTOCOL_BASE_FEE = 7;
}  // namespace eip1559

/**
 * @brief Calculate the base fee for a block given parent block parameters.
 *
 * Implements the EIP-1559 base fee calculation algorithm:
 * - If parent gas used equals target (parent_gas_limit / 2), base fee unchanged
 * - If parent gas used > target, base fee increases
 * - If parent gas used < target, base fee decreases
 *
 * The change is bounded by BASE_FEE_CHANGE_DENOMINATOR (max 12.5% change).
 *
 * @param parent_base_fee Base fee of the parent block
 * @param parent_gas_used Gas used by the parent block
 * @param parent_gas_limit Gas limit of the parent block
 * @return Calculated base fee for the current block
 */
[[nodiscard]] types::Uint256 calc_base_fee(const types::Uint256& parent_base_fee,
                                           uint64_t parent_gas_used, uint64_t parent_gas_limit);

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_EIP1559_H
