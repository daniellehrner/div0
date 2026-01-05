#ifndef DIV0_T8N_ALLOC_H
#define DIV0_T8N_ALLOC_H

/// @file alloc.h
/// @brief t8n alloc.json parsing and serialization.
///
/// This file provides JSON parsing/serialization for the alloc format used
/// by go-ethereum's transition tool (t8n). The underlying types are defined
/// in div0/state/snapshot.h and are available in all builds.
///
/// JSON functions are hosted-only (require filesystem and JSON library).

#include "div0/state/snapshot.h"

// ============================================================================
// JSON Parsing/Serialization (hosted only)
// ============================================================================

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/json/parse.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"

// ============================================================================
// Parsing
// ============================================================================

/// Parse alloc.json from string.
///
/// @param json JSON string
/// @param len JSON length
/// @param arena Arena for allocations
/// @param out Output snapshot structure
/// @return Parse result
json_result_t t8n_parse_alloc(const char *json, size_t len, div0_arena_t *arena,
                              state_snapshot_t *out);

/// Parse alloc.json from file.
///
/// @param path File path
/// @param arena Arena for allocations
/// @param out Output snapshot structure
/// @return Parse result
json_result_t t8n_parse_alloc_file(const char *path, div0_arena_t *arena, state_snapshot_t *out);

/// Parse alloc from a JSON value (already parsed document).
///
/// @param root JSON object (root of alloc document)
/// @param arena Arena for allocations
/// @param out Output snapshot structure
/// @return Parse result
json_result_t t8n_parse_alloc_value(yyjson_val_t *root, div0_arena_t *arena, state_snapshot_t *out);

// ============================================================================
// Serialization
// ============================================================================

/// Serialize state snapshot to alloc JSON format.
///
/// @param snapshot State snapshot to serialize
/// @param w JSON writer
/// @return Root JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_alloc(const state_snapshot_t *snapshot, json_writer_t *w);

/// Serialize a single account snapshot to JSON object.
///
/// @param account Account snapshot to serialize
/// @param w JSON writer
/// @return JSON object value, or nullptr on error
yyjson_mut_val_t *t8n_write_alloc_account(const account_snapshot_t *account, json_writer_t *w);

#endif // DIV0_FREESTANDING

#endif // DIV0_T8N_ALLOC_H
