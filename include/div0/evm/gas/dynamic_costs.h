#ifndef DIV0_EVM_GAS_DYNAMIC_COSTS_H
#define DIV0_EVM_GAS_DYNAMIC_COSTS_H

#include "div0/evm/gas.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// Gas Cost Function Types
// =============================================================================

typedef uint64_t (*sload_cost_fn_t)(bool is_cold);

typedef uint64_t (*sstore_cost_fn_t)(bool is_cold, uint256_t current_value,
                                     uint256_t original_value, uint256_t new_value);

typedef int64_t (*sstore_refund_fn_t)(uint256_t current_value, uint256_t original_value,
                                      uint256_t new_value);

typedef struct {
  sload_cost_fn_t sload;
  sstore_cost_fn_t sstore;
  sstore_refund_fn_t sstore_refund;
  uint64_t sstore_min_gas;
} gas_schedule_t;

// =============================================================================
// Shanghai Gas Functions (EIP-2929 + EIP-2200)
// =============================================================================

static inline uint64_t sload_cost_shanghai(bool is_cold) {
  return is_cold ? GAS_COLD_SLOAD : GAS_WARM_STORAGE_READ;
}

static inline uint64_t sstore_cost_shanghai(bool is_cold, uint256_t current_value,
                                            uint256_t original_value, uint256_t new_value) {
  // EIP-2929: Cold access adds 2100 on top of base cost
  uint64_t cold_cost = is_cold ? GAS_COLD_SLOAD : 0;

  // EIP-2200: Calculate base SSTORE cost
  // No-op (value unchanged) and dirty slot (current != original) both cost warm read.
  // Only clean slot transitions (current == original) have higher costs.
  uint64_t base_cost;
  bool is_clean_slot = uint256_eq(original_value, current_value);
  bool is_noop = uint256_eq(current_value, new_value);

  if (!is_noop && is_clean_slot) {
    // Clean slot transition
    if (uint256_is_zero(original_value)) {
      // 0 -> non-zero: SSTORE_SET
      base_cost = GAS_SSTORE_SET;
    } else {
      // non-zero -> different: SSTORE_RESET
      base_cost = GAS_SSTORE_RESET;
    }
  } else {
    // No-op or dirty slot: warm storage read cost
    base_cost = GAS_WARM_STORAGE_READ;
  }

  return cold_cost + base_cost;
}

static inline int64_t sstore_refund_shanghai(uint256_t current_value, uint256_t original_value,
                                             uint256_t new_value) {
  if (uint256_eq(current_value, new_value)) {
    return 0;
  }

  int64_t refund = 0;

  if (!uint256_is_zero(original_value)) {
    if (uint256_is_zero(current_value)) {
      refund -= (int64_t)GAS_SSTORE_CLEAR_REFUND;
    } else if (uint256_is_zero(new_value)) {
      refund += (int64_t)GAS_SSTORE_CLEAR_REFUND;
    }
  }

  if (uint256_eq(original_value, new_value)) {
    if (uint256_is_zero(original_value)) {
      refund += (int64_t)(GAS_SSTORE_SET - GAS_WARM_STORAGE_READ);
    } else {
      refund += (int64_t)(GAS_SSTORE_RESET - GAS_WARM_STORAGE_READ);
    }
  }

  return refund;
}

static inline gas_schedule_t gas_schedule_shanghai(void) {
  return (gas_schedule_t){
      .sload = sload_cost_shanghai,
      .sstore = sstore_cost_shanghai,
      .sstore_refund = sstore_refund_shanghai,
      .sstore_min_gas = GAS_CALL_STIPEND,
  };
}

// =============================================================================
// Cancun Gas Functions
// =============================================================================

static inline gas_schedule_t gas_schedule_cancun(void) {
  return gas_schedule_shanghai();
}

// =============================================================================
// Prague Gas Functions
// =============================================================================

static inline gas_schedule_t gas_schedule_prague(void) {
  return gas_schedule_cancun();
}

#endif // DIV0_EVM_GAS_DYNAMIC_COSTS_H
