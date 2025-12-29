#ifndef DIV0_JSON_PARSE_H
#define DIV0_JSON_PARSE_H

/// @file parse.h
/// @brief JSON parsing API using yyjson.
///
/// Provides functions for parsing JSON from strings and files, with helpers
/// for extracting hex-encoded Ethereum types.

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stddef.h>

// Forward declarations for yyjson types (opaque to users)
typedef struct yyjson_val yyjson_val_t;
typedef struct yyjson_doc yyjson_doc_t;

// ============================================================================
// Document Handle
// ============================================================================

/// Opaque JSON document handle.
///
/// A document must be freed with json_doc_free() when no longer needed.
/// All yyjson_val pointers from the document become invalid after freeing.
typedef struct {
  yyjson_doc_t *doc;
} json_doc_t;

/// Parse JSON from string.
///
/// @param json JSON string (not required to be null-terminated)
/// @param len Length of JSON string
/// @param doc Output document handle (must be freed with json_doc_free)
/// @return Parse result
json_result_t json_parse(const char *json, size_t len, json_doc_t *doc);

/// Parse JSON from file.
///
/// @param path File path
/// @param doc Output document handle
/// @return Parse result
json_result_t json_parse_file(const char *path, json_doc_t *doc);

/// Free a parsed document.
///
/// @param doc Document to free (may be nullptr)
void json_doc_free(json_doc_t *doc);

/// Get root value from document.
///
/// @param doc Parsed document
/// @return Root JSON value, or nullptr if doc is invalid
yyjson_val_t *json_doc_root(const json_doc_t *doc);

// ============================================================================
// Type Checking
// ============================================================================

/// Check if value is null.
bool json_is_null(yyjson_val_t *val);

/// Check if value is a boolean.
bool json_is_bool(yyjson_val_t *val);

/// Check if value is a number.
bool json_is_num(yyjson_val_t *val);

/// Check if value is a string.
bool json_is_str(yyjson_val_t *val);

/// Check if value is an object.
bool json_is_obj(yyjson_val_t *val);

/// Check if value is an array.
bool json_is_arr(yyjson_val_t *val);

// ============================================================================
// Value Accessors
// ============================================================================

/// Get object field by key.
///
/// @param obj JSON object
/// @param key Field name
/// @return Field value, or nullptr if not found or obj is not an object
yyjson_val_t *json_obj_get(yyjson_val_t *obj, const char *key);

/// Get array element by index.
///
/// @param arr JSON array
/// @param idx Zero-based index
/// @return Element value, or nullptr if out of bounds or arr is not an array
yyjson_val_t *json_arr_get(yyjson_val_t *arr, size_t idx);

/// Get array length.
///
/// @param arr JSON array
/// @return Number of elements, or 0 if arr is not an array
size_t json_arr_len(yyjson_val_t *arr);

/// Get object size (number of key-value pairs).
///
/// @param obj JSON object
/// @return Number of fields, or 0 if obj is not an object
size_t json_obj_size(yyjson_val_t *obj);

/// Get string value.
///
/// @param val JSON value
/// @return String pointer (owned by document), or nullptr if not a string
const char *json_get_str(yyjson_val_t *val);

/// Get string length.
///
/// @param val JSON value
/// @return String length, or 0 if not a string
size_t json_get_str_len(yyjson_val_t *val);

/// Get boolean value.
///
/// @param val JSON value
/// @return Boolean value, or false if not a boolean
bool json_get_bool(yyjson_val_t *val);

/// Get uint64 value.
///
/// @param val JSON value
/// @return Uint64 value, or 0 if not a number
uint64_t json_get_u64(yyjson_val_t *val);

/// Get int64 value.
///
/// @param val JSON value
/// @return Int64 value, or 0 if not a number
int64_t json_get_i64(yyjson_val_t *val);

// ============================================================================
// Hex-Encoded Value Parsing
// ============================================================================

/// Parse hex-encoded uint64 from JSON string field.
///
/// @param obj JSON object
/// @param key Field name
/// @param out Output value
/// @return true on success, false if missing or invalid
bool json_get_hex_u64(yyjson_val_t *obj, const char *key, uint64_t *out);

/// Parse hex-encoded uint256 from JSON string field.
///
/// @param obj JSON object
/// @param key Field name
/// @param out Output value
/// @return true on success, false if missing or invalid
bool json_get_hex_uint256(yyjson_val_t *obj, const char *key, uint256_t *out);

/// Parse hex-encoded address from JSON string field.
///
/// @param obj JSON object
/// @param key Field name
/// @param out Output value
/// @return true on success, false if missing or invalid
bool json_get_hex_address(yyjson_val_t *obj, const char *key, address_t *out);

/// Parse hex-encoded hash from JSON string field.
///
/// @param obj JSON object
/// @param key Field name
/// @param out Output value
/// @return true on success, false if missing or invalid
bool json_get_hex_hash(yyjson_val_t *obj, const char *key, hash_t *out);

/// Parse hex-encoded bytes from JSON string field.
///
/// Allocates the output bytes using the provided arena.
///
/// @param obj JSON object
/// @param key Field name
/// @param arena Arena for allocation
/// @param out Output bytes structure
/// @return true on success, false if missing or invalid
bool json_get_hex_bytes(yyjson_val_t *obj, const char *key, div0_arena_t *arena, bytes_t *out);

/// Parse hex-encoded uint64 from JSON string value.
///
/// @param val JSON string value
/// @param out Output value
/// @return true on success, false if not a string or invalid hex
bool json_val_hex_u64(yyjson_val_t *val, uint64_t *out);

/// Parse hex-encoded uint256 from JSON string value.
///
/// @param val JSON string value
/// @param out Output value
/// @return true on success, false if not a string or invalid hex
bool json_val_hex_uint256(yyjson_val_t *val, uint256_t *out);

/// Parse hex-encoded address from JSON string value.
///
/// @param val JSON string value
/// @param out Output value
/// @return true on success, false if not a string or invalid hex
bool json_val_hex_address(yyjson_val_t *val, address_t *out);

/// Parse hex-encoded hash from JSON string value.
///
/// @param val JSON string value
/// @param out Output value
/// @return true on success, false if not a string or invalid hex
bool json_val_hex_hash(yyjson_val_t *val, hash_t *out);

/// Parse hex-encoded bytes from JSON string value.
///
/// @param val JSON string value
/// @param arena Arena for allocation
/// @param out Output bytes structure
/// @return true on success, false if not a string or invalid hex
bool json_val_hex_bytes(yyjson_val_t *val, div0_arena_t *arena, bytes_t *out);

// ============================================================================
// Object Iteration
// ============================================================================

/// Iterator for JSON objects.
typedef struct {
  yyjson_val_t *cur; ///< Current key pointer
  size_t idx;        ///< Current index
  size_t max;        ///< Total count
} json_obj_iter_t;

/// Initialize object iterator.
///
/// @param obj JSON object
/// @return Iterator (use json_obj_iter_next to iterate)
json_obj_iter_t json_obj_iter(yyjson_val_t *obj);

/// Get next key-value pair.
///
/// @param iter Iterator
/// @param key Output key string (owned by document)
/// @param val Output value
/// @return true if a pair was returned, false when exhausted
bool json_obj_iter_next(json_obj_iter_t *iter, const char **key, yyjson_val_t **val);

// ============================================================================
// Array Iteration
// ============================================================================

/// Iterator for JSON arrays.
typedef struct {
  yyjson_val_t *cur; ///< Current element pointer
  size_t idx;        ///< Current index
  size_t max;        ///< Total count
} json_arr_iter_t;

/// Initialize array iterator.
///
/// @param arr JSON array
/// @return Iterator (use json_arr_iter_next to iterate)
json_arr_iter_t json_arr_iter(yyjson_val_t *arr);

/// Get next array element.
///
/// @param iter Iterator
/// @param val Output value
/// @return true if an element was returned, false when exhausted
bool json_arr_iter_next(json_arr_iter_t *iter, yyjson_val_t **val);

#endif // DIV0_FREESTANDING
#endif // DIV0_JSON_PARSE_H
