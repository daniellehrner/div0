#ifndef DIV0_T8N_TXS_H
#define DIV0_T8N_TXS_H

/// @file txs.h
/// @brief t8n txs.json schema types and parsing.
///
/// The txs file contains an array of transactions to execute in the transition
/// tool. Transactions can be any of the supported types (Legacy through
/// EIP-7702).

#ifndef DIV0_FREESTANDING

#include "div0/ethereum/transaction/transaction.h"
#include "div0/json/json.h"
#include "div0/json/parse.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"

#include <stddef.h>

// ============================================================================
// Types
// ============================================================================

/// Parsed transactions array.
typedef struct {
  transaction_t *txs; ///< Transaction array (arena-allocated)
  size_t tx_count;    ///< Number of transactions
} t8n_txs_t;

// ============================================================================
// Parsing
// ============================================================================

/// Parse transactions from JSON array string.
///
/// Supports all transaction types (0-4): Legacy, EIP-2930, EIP-1559,
/// EIP-4844, and EIP-7702.
///
/// @param json JSON array string (e.g., "[{...}, {...}]")
/// @param len JSON length
/// @param arena Arena for allocations
/// @param out Output transactions
/// @return Parse result
json_result_t t8n_parse_txs(const char *json, size_t len, div0_arena_t *arena, t8n_txs_t *out);

/// Parse transactions from file.
///
/// @param path File path
/// @param arena Arena for allocations
/// @param out Output transactions
/// @return Parse result
json_result_t t8n_parse_txs_file(const char *path, div0_arena_t *arena, t8n_txs_t *out);

/// Parse transactions from a JSON array value.
///
/// @param arr JSON array
/// @param arena Arena for allocations
/// @param out Output transactions
/// @return Parse result
json_result_t t8n_parse_txs_value(yyjson_val_t *arr, div0_arena_t *arena, t8n_txs_t *out);

/// Parse a single transaction from JSON object.
///
/// @param obj JSON object representing a transaction
/// @param arena Arena for allocations
/// @param out Output transaction
/// @return Parse result
json_result_t t8n_parse_tx(yyjson_val_t *obj, div0_arena_t *arena, transaction_t *out);

// ============================================================================
// Serialization
// ============================================================================

/// Serialize transactions to JSON array.
///
/// @param txs Transactions to serialize
/// @param w JSON writer
/// @return JSON array value, or nullptr on error
yyjson_mut_val_t *t8n_write_txs(const t8n_txs_t *txs, const json_writer_t *w);

/// Serialize a single transaction to JSON object.
///
/// @param tx Transaction to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_tx(const transaction_t *tx, const json_writer_t *w);

#endif // DIV0_FREESTANDING
#endif // DIV0_T8N_TXS_H
