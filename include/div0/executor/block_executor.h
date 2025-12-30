#ifndef DIV0_EXECUTOR_BLOCK_EXECUTOR_H
#define DIV0_EXECUTOR_BLOCK_EXECUTOR_H

#include "div0/ethereum/transaction/transaction.h"
#include "div0/evm/block_context.h"
#include "div0/evm/evm.h"
#include "div0/mem/arena.h"
#include "div0/state/state_access.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// =============================================================================
// Transaction Validation Errors
// =============================================================================

/// Transaction validation error codes.
typedef enum {
  TX_VALID = 0,
  TX_ERR_INVALID_SIGNATURE,
  TX_ERR_NONCE_TOO_LOW,
  TX_ERR_NONCE_TOO_HIGH,
  TX_ERR_INSUFFICIENT_BALANCE,
  TX_ERR_INTRINSIC_GAS,
  TX_ERR_GAS_LIMIT_EXCEEDED,
  TX_ERR_MAX_FEE_TOO_LOW,
  TX_ERR_CHAIN_ID_MISMATCH,
} tx_validation_error_t;

/// Returns a string description for a validation error.
const char *tx_validation_error_str(tx_validation_error_t err);

// =============================================================================
// Execution Log
// =============================================================================

/// Log entry emitted during EVM execution.
typedef struct {
  address_t address;   // Contract that emitted the log
  hash_t topics[4];    // Up to 4 indexed topics
  uint8_t topic_count; // Number of topics (0-4)
  uint8_t *data;       // Log data (arena-allocated)
  size_t data_size;    // Log data size
} exec_log_t;

// =============================================================================
// Execution Receipt
// =============================================================================

/// Transaction execution receipt.
typedef struct {
  hash_t tx_hash;             // Transaction hash
  uint8_t tx_type;            // Transaction type (0-4)
  bool success;               // True if execution succeeded
  uint64_t gas_used;          // Gas used by this transaction
  uint64_t cumulative_gas;    // Cumulative gas used in block
  exec_log_t *logs;           // Logs emitted (arena-allocated)
  size_t log_count;           // Number of logs
  address_t *created_address; // Contract address if CREATE (arena-allocated, nullptr otherwise)
  uint8_t *output;            // Return data (arena-allocated)
  size_t output_size;         // Return data size
} exec_receipt_t;

// =============================================================================
// Rejected Transaction
// =============================================================================

/// Rejected transaction entry.
typedef struct {
  size_t index;                // Original transaction index
  tx_validation_error_t error; // Validation error code
  const char *error_message;   // Human-readable error message
} exec_rejected_t;

// =============================================================================
// Block Execution Result
// =============================================================================

/// Result of executing all transactions in a block.
typedef struct {
  exec_receipt_t *receipts;  // Receipts for executed transactions
  size_t receipt_count;      // Number of receipts
  exec_rejected_t *rejected; // Rejected transactions
  size_t rejected_count;     // Number of rejected
  uint64_t gas_used;         // Total gas used
  uint64_t blob_gas_used;    // Total blob gas used (EIP-4844)
  hash_t state_root;         // Final state root
} block_exec_result_t;

// =============================================================================
// Transaction Input
// =============================================================================

/// Transaction with recovered sender for block execution.
typedef struct {
  const transaction_t *tx; // Transaction data
  address_t sender;        // Recovered sender address
  bool sender_recovered;   // True if sender already recovered (e.g., from t8n)
  size_t original_index;   // Index in input array (for rejection tracking)
} block_tx_t;

// =============================================================================
// Block Executor
// =============================================================================

/// Block executor instance.
/// Coordinates transaction execution across state and EVM.
typedef struct {
  state_access_t *state;          // State access interface (vtable)
  const block_context_t *block;   // Block context (shared across txs)
  evm_t *evm;                     // EVM instance (reused per tx)
  div0_arena_t *arena;            // Arena for allocations
  uint64_t chain_id;              // Chain ID for validation
  bool skip_signature_validation; // Skip signature recovery (for t8n)
} block_executor_t;

// =============================================================================
// Block Executor API
// =============================================================================

/// Initialize a block executor.
/// The executor does not own any resources - caller manages lifecycle.
/// @param exec Executor to initialize
/// @param state State access interface
/// @param block Block context
/// @param evm EVM instance
/// @param arena Arena for allocations
/// @param chain_id Chain ID for validation
void block_executor_init(block_executor_t *exec, state_access_t *state,
                         const block_context_t *block, evm_t *evm, div0_arena_t *arena,
                         uint64_t chain_id);

/// Execute all transactions in a block.
/// @param exec Initialized block executor
/// @param txs Transactions to execute
/// @param tx_count Number of transactions
/// @param result Output result (arena-allocated)
/// @return true on success, false on fatal error
[[nodiscard]] bool block_executor_run(block_executor_t *exec, const block_tx_t *txs,
                                      size_t tx_count, block_exec_result_t *result);

/// Validate a transaction before execution.
/// @param exec Initialized block executor
/// @param tx Transaction with sender
/// @param cumulative_gas Current cumulative gas in block
/// @return TX_VALID if valid, error code otherwise
[[nodiscard]] tx_validation_error_t block_executor_validate_tx(const block_executor_t *exec,
                                                               const block_tx_t *tx,
                                                               uint64_t cumulative_gas);

// =============================================================================
// Intrinsic Gas Calculation
// =============================================================================

/// Calculate intrinsic gas for a transaction.
/// Includes base cost, calldata cost, create cost, and access list cost.
/// @param tx Transaction to calculate intrinsic gas for
/// @return Intrinsic gas cost
[[nodiscard]] uint64_t tx_intrinsic_gas(const transaction_t *tx);

// =============================================================================
// Contract Address Calculation
// =============================================================================

/// Compute CREATE contract address.
/// address = keccak256(rlp([sender, nonce]))[12:]
/// @param sender Sender address
/// @param nonce Sender nonce at time of creation
/// @return Contract address
[[nodiscard]] address_t compute_create_address(const address_t *sender, uint64_t nonce);

/// Compute CREATE2 contract address.
/// address = keccak256(0xff ++ sender ++ salt ++ keccak256(init_code))[12:]
/// @param sender Sender address
/// @param salt 32-byte salt
/// @param init_code_hash Keccak256 hash of init code
/// @return Contract address
[[nodiscard]] address_t compute_create2_address(const address_t *sender, const hash_t *salt,
                                                const hash_t *init_code_hash);

#endif // DIV0_EXECUTOR_BLOCK_EXECUTOR_H
