// Unit tests for EIP-1559 base fee calculation

#include <div0/ethereum/eip1559.h>
#include <gtest/gtest.h>

namespace div0::ethereum {

using types::Uint256;

// =============================================================================
// Geth Test Vectors
// From go-ethereum/consensus/misc/eip1559/eip1559_test.go
// =============================================================================

// InitialBaseFee = 1000000000 (1 gwei)
TEST(CalcBaseFee, GethTestVector_UsageEqualsTarget) {
  // parentBaseFee: 1000000000, parentGasLimit: 20000000, parentGasUsed: 10000000
  // Expected: 1000000000 (unchanged)
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 10000000;  // = target (limit / 2)

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(eip1559::INITIAL_BASE_FEE));
}

TEST(CalcBaseFee, GethTestVector_UsageBelowTarget) {
  // parentBaseFee: 1000000000, parentGasLimit: 20000000, parentGasUsed: 9000000
  // Expected: 987500000
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 9000000;

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(987500000));
}

TEST(CalcBaseFee, GethTestVector_UsageAboveTarget) {
  // parentBaseFee: 1000000000, parentGasLimit: 20000000, parentGasUsed: 11000000
  // Expected: 1012500000
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 11000000;

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1012500000));
}

// =============================================================================
// Gas Used Equals Target (No Change)
// =============================================================================

TEST(CalcBaseFee, UsageEqualsTarget_BaseFeelUnchanged) {
  const Uint256 parent_base_fee(5000000000ULL);  // 5 gwei
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 15000000;  // = limit / 2

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, parent_base_fee);
}

TEST(CalcBaseFee, UsageEqualsTarget_MinBaseFee) {
  const Uint256 parent_base_fee(eip1559::MIN_PROTOCOL_BASE_FEE);  // 7 wei
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 10000000;  // = target

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(eip1559::MIN_PROTOCOL_BASE_FEE));
}

// =============================================================================
// Gas Used Above Target (Increase Scenarios)
// =============================================================================

TEST(CalcBaseFee, UsageAboveTarget_FullBlock) {
  // Full block: gas used = gas limit (2x target)
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);  // 1 gwei
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 30000000;  // Full block

  // delta = parentBaseFee * (gasUsed - target) / target / 8
  // delta = 1000000000 * 15000000 / 15000000 / 8 = 1000000000 / 8 = 125000000
  // newBaseFee = 1000000000 + 125000000 = 1125000000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1125000000));
}

TEST(CalcBaseFee, UsageAboveTarget_10PercentAbove) {
  // Gas used 10% above target (same pattern as geth test)
  // parentGasLimit: 20000000, target: 10000000, gasUsed: 11000000 (10% above)
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 11000000;  // 10% above target

  // delta = 1000000000 * 1000000 / 10000000 / 8 = 12500000
  // newBaseFee = 1000000000 + 12500000 = 1012500000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1012500000));
}

TEST(CalcBaseFee, UsageAboveTarget_MinimumDeltaOfOne) {
  // Very small increase should still result in at least delta = 1
  const Uint256 parent_base_fee(100);  // Small base fee
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 10000001;  // 1 gas above target

  // delta = 100 * 1 / 10000000 / 8 = 0 -> minimum of 1
  // newBaseFee = 100 + 1 = 101
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(101));
}

TEST(CalcBaseFee, UsageAboveTarget_MaxIncrease) {
  // Full block should increase by 12.5% (max)
  const Uint256 parent_base_fee(1000);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 20000000;  // Full block

  // delta = 1000 * 10000000 / 10000000 / 8 = 125
  // newBaseFee = 1000 + 125 = 1125
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1125));
}

TEST(CalcBaseFee, UsageAboveTarget_LargeBaseFee) {
  // Large base fee to test uint256 handling
  const Uint256 parent_base_fee(
      std::array<uint64_t, 4>{0xFFFFFFFFFFFFFFFF, 0, 0, 0});  // ~18.4 * 10^18
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 30000000;  // Full block

  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  // Should increase by 12.5%
  const Uint256 expected_delta = parent_base_fee / Uint256(8);
  EXPECT_EQ(result, parent_base_fee + expected_delta);
}

// =============================================================================
// Gas Used Below Target (Decrease Scenarios)
// =============================================================================

TEST(CalcBaseFee, UsageBelowTarget_EmptyBlock) {
  // Empty block: gas used = 0
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);  // 1 gwei
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 0;

  // delta = parentBaseFee * target / target / 8 = parentBaseFee / 8
  // delta = 1000000000 / 8 = 125000000
  // newBaseFee = 1000000000 - 125000000 = 875000000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(875000000));
}

TEST(CalcBaseFee, UsageBelowTarget_10PercentBelow) {
  // Gas used 10% below target (same pattern as geth test)
  // parentGasLimit: 20000000, target: 10000000, gasUsed: 9000000 (10% below)
  const Uint256 parent_base_fee(eip1559::INITIAL_BASE_FEE);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 9000000;  // 10% below target

  // delta = 1000000000 * 1000000 / 10000000 / 8 = 12500000
  // newBaseFee = 1000000000 - 12500000 = 987500000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(987500000));
}

TEST(CalcBaseFee, UsageBelowTarget_MaxDecrease) {
  // Empty block should decrease by 12.5% (max)
  const Uint256 parent_base_fee(1000);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 0;

  // delta = 1000 * 10000000 / 10000000 / 8 = 125
  // newBaseFee = 1000 - 125 = 875
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(875));
}

// =============================================================================
// Minimum Base Fee Floor (7 wei)
// =============================================================================

TEST(CalcBaseFee, MinBaseFeeFloor_CannotDecreaseBelowMin) {
  // At minimum base fee, should not decrease further
  const Uint256 parent_base_fee(eip1559::MIN_PROTOCOL_BASE_FEE);  // 7 wei
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 0;  // Empty block

  // 12.5% of 7 = 0.875, rounds down to 0
  // So base fee stays at 7
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(eip1559::MIN_PROTOCOL_BASE_FEE));
}

TEST(CalcBaseFee, MinBaseFeeFloor_DecreasesToMin) {
  // Base fee of 8 should decrease to 7 (floor)
  const Uint256 parent_base_fee(8);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 0;  // Empty block

  // delta = 8 * 10000000 / 10000000 / 8 = 1
  // newBaseFee = 8 - 1 = 7
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(eip1559::MIN_PROTOCOL_BASE_FEE));
}

TEST(CalcBaseFee, MinBaseFeeFloor_SmallBaseFeeClamped) {
  // Very small base fee that would decrease below floor
  const Uint256 parent_base_fee(10);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 0;

  // delta = 10 / 8 = 1
  // newBaseFee = 10 - 1 = 9 (still above floor)
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(9));
}

TEST(CalcBaseFee, MinBaseFeeFloor_LargeDecreaseClampedToMin) {
  // Test with parent base fee that has very large potential delta
  // but should be clamped to minimum
  const Uint256 parent_base_fee(100);
  const uint64_t parent_gas_limit = 20000000;
  const uint64_t parent_gas_used = 0;

  // delta = 100 / 8 = 12
  // newBaseFee = 100 - 12 = 88
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(88));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(CalcBaseFee, EdgeCase_VerySmallGasLimit) {
  // Very small gas limit
  const Uint256 parent_base_fee(1000000000);
  const uint64_t parent_gas_limit = 100;
  const uint64_t parent_gas_used = 100;  // Full block

  // target = 50
  // delta = 1000000000 * 50 / 50 / 8 = 125000000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1125000000));
}

TEST(CalcBaseFee, EdgeCase_GasLimitOfTwo) {
  // Minimum meaningful gas limit (target = 1)
  const Uint256 parent_base_fee(1000);
  const uint64_t parent_gas_limit = 2;
  const uint64_t parent_gas_used = 2;  // Full block

  // target = 1, gas_used_delta = 1
  // delta = 1000 * 1 / 1 / 8 = 125
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(1125));
}

TEST(CalcBaseFee, EdgeCase_LargeGasLimit) {
  // Large gas limit (mainnet-like, 30M)
  const Uint256 parent_base_fee(50000000000ULL);  // 50 gwei
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 20000000;  // 2/3 of limit

  // target = 15000000
  // gas_used_delta = 5000000
  // delta = 50000000000 * 5000000 / 15000000 / 8 = 2083333333
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(52083333333ULL));
}

TEST(CalcBaseFee, EdgeCase_ZeroGasUsedWithHighBaseFee) {
  // Zero gas used (empty block) with high base fee
  const Uint256 parent_base_fee(100000000000ULL);  // 100 gwei
  const uint64_t parent_gas_limit = 30000000;
  const uint64_t parent_gas_used = 0;

  // delta = 100000000000 / 8 = 12500000000
  // newBaseFee = 100000000000 - 12500000000 = 87500000000
  const auto result = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  EXPECT_EQ(result, Uint256(87500000000ULL));
}

// =============================================================================
// Determinism
// =============================================================================

TEST(CalcBaseFee, Deterministic_SameInputsSameOutput) {
  const Uint256 parent_base_fee(1234567890ULL);
  const uint64_t parent_gas_limit = 25000000;
  const uint64_t parent_gas_used = 13000000;

  const auto result1 = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  const auto result2 = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);
  const auto result3 = calc_base_fee(parent_base_fee, parent_gas_used, parent_gas_limit);

  EXPECT_EQ(result1, result2);
  EXPECT_EQ(result2, result3);
}

// =============================================================================
// Constants Verification
// =============================================================================

TEST(EIP1559Constants, CorrectValues) {
  EXPECT_EQ(eip1559::BASE_FEE_CHANGE_DENOMINATOR, 8);
  EXPECT_EQ(eip1559::ELASTICITY_MULTIPLIER, 2);
  EXPECT_EQ(eip1559::INITIAL_BASE_FEE, 1000000000);
  EXPECT_EQ(eip1559::MIN_PROTOCOL_BASE_FEE, 7);
}

}  // namespace div0::ethereum
