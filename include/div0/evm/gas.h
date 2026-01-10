#ifndef DIV0_EVM_GAS_H
#define DIV0_EVM_GAS_H

#include <stdint.h>

// =============================================================================
// EVM Gas Constants (EIP-150, EIP-2929, EIP-3529)
// =============================================================================

/// Zero gas cost.
static constexpr uint64_t GAS_ZERO = 0;

/// Base gas cost for most simple operations.
static constexpr uint64_t GAS_BASE = 2;

/// Very low gas cost.
static constexpr uint64_t GAS_VERY_LOW = 3;

/// Low gas cost.
static constexpr uint64_t GAS_LOW = 5;

/// Mid gas cost.
static constexpr uint64_t GAS_MID = 8;

/// High gas cost.
static constexpr uint64_t GAS_HIGH = 10;

/// Jump destination gas.
static constexpr uint64_t GAS_JUMPDEST = 1;

// =============================================================================
// Memory Gas
// =============================================================================

/// Gas per word for memory operations.
static constexpr uint64_t GAS_MEMORY = 3;

/// Gas for copying per word (CALLDATACOPY, CODECOPY, etc.).
static constexpr uint64_t GAS_COPY = 3;

// =============================================================================
// CALL Gas Constants (EIP-150, EIP-2929)
// =============================================================================

/// Cold account access (EIP-2929).
static constexpr uint64_t GAS_COLD_ACCOUNT_ACCESS = 2600;

/// Warm account access (EIP-2929).
static constexpr uint64_t GAS_WARM_ACCESS = 100;

/// Gas stipend given to callee when value is transferred.
static constexpr uint64_t GAS_CALL_STIPEND = 2300;

/// Extra gas for CALL with non-zero value.
static constexpr uint64_t GAS_CALL_VALUE = 9000;

/// Extra gas for CALL to non-existent account with non-zero value.
static constexpr uint64_t GAS_NEW_ACCOUNT = 25000;

// =============================================================================
// Storage Gas (EIP-2200, EIP-3529)
// =============================================================================

/// Cold SLOAD (EIP-2929).
static constexpr uint64_t GAS_COLD_SLOAD = 2100;

/// Warm SLOAD (EIP-2929).
static constexpr uint64_t GAS_WARM_STORAGE_READ = 100;

/// SSTORE set (zero to non-zero).
static constexpr uint64_t GAS_SSTORE_SET = 20000;

/// SSTORE reset (non-zero to non-zero or non-zero to zero).
static constexpr uint64_t GAS_SSTORE_RESET = 2900;

/// SSTORE clear refund (non-zero to zero).
static constexpr uint64_t GAS_SSTORE_CLEAR_REFUND = 4800;

// =============================================================================
// CREATE Gas
// =============================================================================

/// CREATE gas.
static constexpr uint64_t GAS_CREATE = 32000;

/// Gas per byte for contract code storage.
static constexpr uint64_t GAS_CODE_DEPOSIT_PER_BYTE = 200;

/// Keccak256 per word gas.
static constexpr uint64_t GAS_KECCAK256_WORD = 6;

// =============================================================================
// Transaction Gas
// =============================================================================

/// Transaction base gas.
static constexpr uint64_t GAS_TX = 21000;

/// Gas per zero byte in transaction data.
static constexpr uint64_t GAS_TX_DATA_ZERO = 4;

/// Gas per non-zero byte in transaction data.
static constexpr uint64_t GAS_TX_DATA_NON_ZERO = 16;

/// Gas per access list address.
static constexpr uint64_t GAS_ACCESS_LIST_ADDRESS = 2400;

/// Gas per access list storage key.
static constexpr uint64_t GAS_ACCESS_LIST_STORAGE_KEY = 1900;

// =============================================================================
// Limits
// =============================================================================

/// Maximum call depth.
static constexpr uint16_t MAX_CALL_DEPTH = 1024;

// =============================================================================
// LOG Gas Constants
// =============================================================================

/// Base gas for LOG opcode.
static constexpr uint64_t GAS_LOG = 375;

/// Gas per topic.
static constexpr uint64_t GAS_LOG_TOPIC = 375;

/// Gas per byte of log data.
static constexpr uint64_t GAS_LOG_DATA = 8;

/// Maximum code size (EIP-170).
static constexpr uint64_t MAX_CODE_SIZE = 24576;

/// Maximum initcode size (EIP-3860).
static constexpr uint64_t MAX_INITCODE_SIZE = 49152;

// =============================================================================
// Gas Calculation Helpers
// =============================================================================

/// Calculate 63/64 of gas (EIP-150 rule for CALL gas).
static inline uint64_t gas_cap_call(uint64_t gas_left) {
  return gas_left - (gas_left / 64);
}

#endif // DIV0_EVM_GAS_H
