#ifndef DIV0_T8N_RESULT_H
#define DIV0_T8N_RESULT_H

/// @file result.h
/// @brief t8n result.json schema types and serialization.
///
/// The result file contains the output of the transition tool execution,
/// including state root, transaction/receipts roots, and execution receipts.

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/json/write.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Log Type
// ============================================================================

/// Log entry emitted during execution.
typedef struct {
  address_t address;  ///< Contract that emitted the log
  hash_t *topics;     ///< Log topics (array, up to 4)
  size_t topic_count; ///< Number of topics
  bytes_t data;       ///< Log data
} t8n_log_t;

// ============================================================================
// Receipt Type
// ============================================================================

/// Transaction receipt.
typedef struct {
  uint8_t type;                ///< Transaction type (0-4)
  hash_t tx_hash;              ///< Transaction hash
  uint64_t transaction_index;  ///< Index in block
  uint64_t gas_used;           ///< Gas used by this transaction
  uint64_t cumulative_gas;     ///< Cumulative gas used
  bool status;                 ///< Execution status (true = success)
  uint8_t bloom[256];          ///< Bloom filter
  t8n_log_t *logs;             ///< Logs emitted
  size_t log_count;            ///< Number of logs
  address_t *contract_address; ///< Created contract address (or nullptr)
} t8n_receipt_t;

// ============================================================================
// Rejected Transaction Type
// ============================================================================

/// Rejected transaction with error reason.
typedef struct {
  size_t index;      ///< Original transaction index
  const char *error; ///< Error message
} t8n_rejected_tx_t;

// ============================================================================
// Result Type
// ============================================================================

/// t8n execution result.
typedef struct {
  // Roots
  hash_t state_root;       ///< Post-state root
  hash_t tx_root;          ///< Transactions trie root
  hash_t receipts_root;    ///< Receipts trie root
  hash_t logs_hash;        ///< keccak256(rlp(logs))
  uint8_t logs_bloom[256]; ///< Combined bloom filter

  // Gas
  uint64_t gas_used; ///< Total gas used

  // Blob gas (EIP-4844, Cancun+)
  bool has_blob_gas_used;
  uint64_t blob_gas_used; ///< Total blob gas used

  // Receipts
  t8n_receipt_t *receipts; ///< Transaction receipts
  size_t receipt_count;

  // Transaction hashes (in order of execution)
  hash_t *tx_hashes;
  size_t tx_hash_count;

  // Rejected transactions
  t8n_rejected_tx_t *rejected;
  size_t rejected_count;

  // Optional fork-specific fields
  bool has_current_difficulty;
  uint256_t current_difficulty;

  bool has_current_base_fee;
  uint256_t current_base_fee;

  bool has_withdrawals_root;
  hash_t withdrawals_root;

  bool has_current_excess_blob_gas;
  uint64_t current_excess_blob_gas;

  bool has_requests_hash;
  hash_t requests_hash;
} t8n_result_t;

// ============================================================================
// Initialization
// ============================================================================

/// Initialize result with default values.
///
/// @param result Result to initialize
void t8n_result_init(t8n_result_t *result);

// ============================================================================
// Serialization
// ============================================================================

/// Serialize result to JSON object.
///
/// @param result Result to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_result(const t8n_result_t *result, const json_writer_t *w);

/// Serialize a receipt to JSON object.
///
/// @param receipt Receipt to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_receipt(const t8n_receipt_t *receipt, const json_writer_t *w);

/// Serialize a log to JSON object.
///
/// @param log Log to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_log(const t8n_log_t *log, const json_writer_t *w);

#endif // DIV0_FREESTANDING
#endif // DIV0_T8N_RESULT_H
