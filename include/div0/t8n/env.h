#ifndef DIV0_T8N_ENV_H
#define DIV0_T8N_ENV_H

/// @file env.h
/// @brief t8n env.json schema types and parsing.
///
/// The env file contains block environment context for the transition tool,
/// including coinbase, gas limit, block number, timestamp, and fork-specific
/// fields like base fee and blob gas.

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/json/parse.h"
#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Types
// ============================================================================

/// Block hash entry for BLOCKHASH opcode.
typedef struct {
  uint64_t number; ///< Block number
  hash_t hash;     ///< Block hash
} t8n_block_hash_t;

/// Withdrawal entry (EIP-4895, Shanghai+).
typedef struct {
  uint64_t index;           ///< Withdrawal index
  uint64_t validator_index; ///< Validator index
  address_t address;        ///< Withdrawal recipient
  uint64_t amount;          ///< Amount in Gwei
} t8n_withdrawal_t;

/// Ommer (uncle) block entry.
typedef struct {
  address_t coinbase; ///< Ommer block coinbase
  uint64_t delta;     ///< Block number difference from current
} t8n_ommer_t;

/// Parsed env.json structure.
typedef struct {
  // ============================================================
  // Required fields
  // ============================================================
  address_t coinbase; ///< Block coinbase (currentCoinbase)
  uint64_t gas_limit; ///< Block gas limit (currentGasLimit)
  uint64_t number;    ///< Block number (currentNumber)
  uint64_t timestamp; ///< Block timestamp (currentTimestamp)

  // ============================================================
  // Optional fields (has_* indicates presence)
  // ============================================================

  // Pre-merge
  bool has_difficulty;
  uint256_t difficulty; ///< Block difficulty (currentDifficulty)

  // Post-merge (Paris+)
  bool has_prev_randao;
  uint256_t prev_randao; ///< Previous RANDAO (currentRandom/prevRandao)

  // EIP-1559 (London+)
  bool has_base_fee;
  uint256_t base_fee; ///< Block base fee (currentBaseFee)

  // EIP-4844 (Cancun+)
  bool has_excess_blob_gas;
  uint64_t excess_blob_gas; ///< Excess blob gas (currentExcessBlobGas)

  bool has_blob_gas_used;
  uint64_t blob_gas_used; ///< Blob gas used (currentBlobGasUsed)

  // EIP-4788 (Cancun+)
  bool has_parent_beacon_root;
  hash_t parent_beacon_root; ///< Parent beacon block root

  // Parent fields for base fee calculation
  bool has_parent_base_fee;
  uint256_t parent_base_fee;

  bool has_parent_gas_used;
  uint64_t parent_gas_used;

  bool has_parent_gas_limit;
  uint64_t parent_gas_limit;

  bool has_parent_excess_blob_gas;
  uint64_t parent_excess_blob_gas;

  bool has_parent_blob_gas_used;
  uint64_t parent_blob_gas_used;

  // ============================================================
  // Arrays (arena-allocated)
  // ============================================================
  t8n_block_hash_t *block_hashes; ///< Block hash lookup table
  size_t block_hash_count;

  t8n_withdrawal_t *withdrawals; ///< Withdrawals (Shanghai+)
  size_t withdrawal_count;

  t8n_ommer_t *ommers; ///< Ommer blocks
  size_t ommer_count;
} t8n_env_t;

// ============================================================================
// Initialization
// ============================================================================

/// Initialize env with default values.
///
/// Sets all optional fields to "not present" and clears arrays.
///
/// @param env Environment to initialize
void t8n_env_init(t8n_env_t *env);

// ============================================================================
// Parsing
// ============================================================================

/// Parse env.json from string.
///
/// @param json JSON string
/// @param len JSON length
/// @param arena Arena for allocations
/// @param out Output env structure
/// @return Parse result
json_result_t t8n_parse_env(const char *json, size_t len, div0_arena_t *arena, t8n_env_t *out);

/// Parse env.json from file.
///
/// @param path File path
/// @param arena Arena for allocations
/// @param out Output env structure
/// @return Parse result
json_result_t t8n_parse_env_file(const char *path, div0_arena_t *arena, t8n_env_t *out);

/// Parse env from a JSON value (already parsed document).
///
/// @param root JSON object (root of env document)
/// @param arena Arena for allocations
/// @param out Output env structure
/// @return Parse result
json_result_t t8n_parse_env_value(yyjson_val_t *root, div0_arena_t *arena, t8n_env_t *out);

#endif // DIV0_FREESTANDING
#endif // DIV0_T8N_ENV_H
