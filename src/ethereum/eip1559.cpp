// eip1559.cpp - EIP-1559 base fee calculation

#include "div0/ethereum/eip1559.h"

namespace div0::ethereum {

types::Uint256 calc_base_fee(const types::Uint256& parent_base_fee, uint64_t parent_gas_used,
                             uint64_t parent_gas_limit) {
  // Gas target is half of gas limit (elasticity multiplier = 2)
  const uint64_t parent_gas_target = parent_gas_limit / eip1559::ELASTICITY_MULTIPLIER;

  // If gas used equals target, base fee unchanged
  if (parent_gas_used == parent_gas_target) {
    return parent_base_fee;
  }

  if (parent_gas_used > parent_gas_target) {
    // Parent block used more gas than target, increase base fee
    // delta = max(1, parentBaseFee * gasUsedDelta / parentGasTarget / baseFeeChangeDenominator)
    const uint64_t gas_used_delta = parent_gas_used - parent_gas_target;

    // Calculate: parentBaseFee * gasUsedDelta / parentGasTarget / BASE_FEE_CHANGE_DENOMINATOR
    const types::Uint256 numerator = parent_base_fee * types::Uint256(gas_used_delta);
    types::Uint256 delta = numerator / types::Uint256(parent_gas_target);
    delta = delta / types::Uint256(eip1559::BASE_FEE_CHANGE_DENOMINATOR);

    // Ensure minimum increase of 1
    if (delta.is_zero()) {
      delta = types::Uint256(1);
    }

    return parent_base_fee + delta;
  }

  // Parent block used less gas than target, decrease base fee
  // delta = parentBaseFee * gasUsedDelta / parentGasTarget / baseFeeChangeDenominator
  const uint64_t gas_used_delta = parent_gas_target - parent_gas_used;

  // Calculate: parentBaseFee * gasUsedDelta / parentGasTarget / BASE_FEE_CHANGE_DENOMINATOR
  const types::Uint256 numerator = parent_base_fee * types::Uint256(gas_used_delta);
  types::Uint256 delta = numerator / types::Uint256(parent_gas_target);
  delta = delta / types::Uint256(eip1559::BASE_FEE_CHANGE_DENOMINATOR);

  // Compute new base fee, ensuring it doesn't go below minimum (7 wei)
  // At 7 wei, 12.5% (1/8) rounds to 0, so it can't decrease further
  types::Uint256 new_base_fee;
  if (delta >= parent_base_fee) {
    new_base_fee = types::Uint256(eip1559::MIN_PROTOCOL_BASE_FEE);
  } else {
    new_base_fee = parent_base_fee - delta;
    if (new_base_fee < types::Uint256(eip1559::MIN_PROTOCOL_BASE_FEE)) {
      new_base_fee = types::Uint256(eip1559::MIN_PROTOCOL_BASE_FEE);
    }
  }

  return new_base_fee;
}

}  // namespace div0::ethereum
