#ifndef DIV0_T8N_ALLOC_H
#define DIV0_T8N_ALLOC_H

/// @file alloc.h
/// @brief t8n alloc.json schema types and parsing.
///
/// The alloc file contains pre-state account allocations for the transition
/// tool. Each account has an address, balance, optional nonce, code, and
/// storage slots.

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/json/parse.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Types
// ============================================================================

/// Storage slot-value pair.
typedef struct {
  uint256_t slot;  ///< Storage slot key
  uint256_t value; ///< Storage value
} t8n_storage_entry_t;

/// Pre-state account from alloc.json.
typedef struct {
  address_t address;            ///< Account address
  uint256_t balance;            ///< Account balance
  uint64_t nonce;               ///< Account nonce
  bytes_t code;                 ///< Contract bytecode (arena-allocated)
  t8n_storage_entry_t *storage; ///< Storage entries (arena-allocated array)
  size_t storage_count;         ///< Number of storage entries
} t8n_alloc_account_t;

/// Parsed alloc.json result.
typedef struct {
  t8n_alloc_account_t *accounts; ///< Account array (arena-allocated)
  size_t account_count;          ///< Number of accounts
} t8n_alloc_t;

// ============================================================================
// Parsing
// ============================================================================

/// Parse alloc.json from string.
///
/// @param json JSON string
/// @param len JSON length
/// @param arena Arena for allocations
/// @param out Output alloc structure
/// @return Parse result
json_result_t t8n_parse_alloc(const char *json, size_t len, div0_arena_t *arena, t8n_alloc_t *out);

/// Parse alloc.json from file.
///
/// @param path File path
/// @param arena Arena for allocations
/// @param out Output alloc structure
/// @return Parse result
json_result_t t8n_parse_alloc_file(const char *path, div0_arena_t *arena, t8n_alloc_t *out);

/// Parse alloc from a JSON value (already parsed document).
///
/// @param root JSON object (root of alloc document)
/// @param arena Arena for allocations
/// @param out Output alloc structure
/// @return Parse result
json_result_t t8n_parse_alloc_value(yyjson_val_t *root, div0_arena_t *arena, t8n_alloc_t *out);

// ============================================================================
// Serialization
// ============================================================================

/// Serialize alloc to JSON object.
///
/// @param alloc Alloc data to serialize
/// @param w JSON writer
/// @return Root JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_alloc(const t8n_alloc_t *alloc, json_writer_t *w);

/// Serialize a single account to JSON object.
///
/// @param account Account to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_alloc_account(const t8n_alloc_account_t *account, json_writer_t *w);

#endif // DIV0_FREESTANDING
#endif // DIV0_T8N_ALLOC_H
