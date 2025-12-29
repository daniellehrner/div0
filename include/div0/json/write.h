#ifndef DIV0_JSON_WRITE_H
#define DIV0_JSON_WRITE_H

/// @file write.h
/// @brief JSON writing/serialization API using yyjson.
///
/// Provides functions for building and serializing JSON documents.

#ifndef DIV0_FREESTANDING

#include "div0/json/json.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdio.h>

// Forward declarations for yyjson mutable types
typedef struct yyjson_mut_doc yyjson_mut_doc_t;
typedef struct yyjson_mut_val yyjson_mut_val_t;

// ============================================================================
// Writer Handle
// ============================================================================

/// Mutable JSON document for building output.
typedef struct {
  yyjson_mut_doc_t *doc;
} json_writer_t;

/// Initialize a JSON writer.
///
/// @param w Writer to initialize
/// @return Result (JSON_OK on success)
json_result_t json_writer_init(json_writer_t *w);

/// Free a JSON writer.
///
/// @param w Writer to free (may be nullptr)
void json_writer_free(json_writer_t *w);

// ============================================================================
// Value Creation
// ============================================================================

/// Create a null value.
yyjson_mut_val_t *json_write_null(json_writer_t *w);

/// Create a boolean value.
yyjson_mut_val_t *json_write_bool(json_writer_t *w, bool val);

/// Create a uint64 value.
yyjson_mut_val_t *json_write_u64(json_writer_t *w, uint64_t val);

/// Create an int64 value.
yyjson_mut_val_t *json_write_i64(json_writer_t *w, int64_t val);

/// Create a string value (copies the string).
yyjson_mut_val_t *json_write_str(json_writer_t *w, const char *str);

/// Create a string value with length (copies the string).
yyjson_mut_val_t *json_write_strn(json_writer_t *w, const char *str, size_t len);

/// Create an empty object.
yyjson_mut_val_t *json_write_obj(json_writer_t *w);

/// Create an empty array.
yyjson_mut_val_t *json_write_arr(json_writer_t *w);

// ============================================================================
// Object Operations
// ============================================================================

/// Add a key-value pair to an object.
///
/// @param w Writer
/// @param obj Object to modify
/// @param key Field name (copied)
/// @param val Value to add
/// @return true on success
bool json_obj_add(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, yyjson_mut_val_t *val);

/// Add a null field to an object.
bool json_obj_add_null(json_writer_t *w, yyjson_mut_val_t *obj, const char *key);

/// Add a boolean field to an object.
bool json_obj_add_bool(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, bool val);

/// Add a uint64 field to an object.
bool json_obj_add_u64(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, uint64_t val);

/// Add a string field to an object.
bool json_obj_add_str(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, const char *val);

// ============================================================================
// Array Operations
// ============================================================================

/// Append a value to an array.
///
/// @param w Writer
/// @param arr Array to modify
/// @param val Value to append
/// @return true on success
bool json_arr_append(json_writer_t *w, yyjson_mut_val_t *arr, yyjson_mut_val_t *val);

// ============================================================================
// Hex-Encoded Value Creation
// ============================================================================

/// Create hex-encoded uint64 string ("0x...").
///
/// Uses minimal encoding (no leading zeros).
yyjson_mut_val_t *json_write_hex_u64(json_writer_t *w, uint64_t val);

/// Create hex-encoded uint256 string ("0x...").
///
/// Uses minimal encoding (no leading zeros).
yyjson_mut_val_t *json_write_hex_uint256(json_writer_t *w, const uint256_t *val);

/// Create zero-padded hex uint256 (64 chars, "0x...").
yyjson_mut_val_t *json_write_hex_uint256_padded(json_writer_t *w, const uint256_t *val);

/// Create hex-encoded address string ("0x...").
yyjson_mut_val_t *json_write_hex_address(json_writer_t *w, const address_t *addr);

/// Create hex-encoded hash string ("0x...").
yyjson_mut_val_t *json_write_hex_hash(json_writer_t *w, const hash_t *hash);

/// Create hex-encoded bytes string ("0x...").
yyjson_mut_val_t *json_write_hex_bytes(json_writer_t *w, const uint8_t *data, size_t len);

// ============================================================================
// Object Helpers for Hex Values
// ============================================================================

/// Add a hex-encoded uint64 field to an object.
bool json_obj_add_hex_u64(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, uint64_t val);

/// Add a hex-encoded uint256 field to an object.
bool json_obj_add_hex_uint256(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                              const uint256_t *val);

/// Add a zero-padded hex uint256 field to an object.
bool json_obj_add_hex_uint256_padded(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                                     const uint256_t *val);

/// Add a hex-encoded address field to an object.
bool json_obj_add_hex_address(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                              const address_t *addr);

/// Add a hex-encoded hash field to an object.
bool json_obj_add_hex_hash(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                           const hash_t *hash);

/// Add a hex-encoded bytes field to an object.
bool json_obj_add_hex_bytes(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                            const uint8_t *data, size_t len);

// ============================================================================
// Output
// ============================================================================

/// Write flags for output formatting.
typedef enum {
  JSON_WRITE_COMPACT = 0, ///< Compact output (no whitespace)
  JSON_WRITE_PRETTY = 1,  ///< Pretty-printed output
} json_write_flags_t;

/// Write JSON to string.
///
/// @param w Writer with document
/// @param root Root value to write
/// @param flags Output formatting flags
/// @param out_len Output string length (optional, may be nullptr)
/// @return Allocated string (caller must free with free()), or nullptr on error
char *json_write_string(json_writer_t *w, yyjson_mut_val_t *root, json_write_flags_t flags,
                        size_t *out_len);

/// Write JSON to file.
///
/// @param w Writer
/// @param root Root value
/// @param path File path
/// @param flags Output formatting flags
/// @return Result
json_result_t json_write_file(json_writer_t *w, yyjson_mut_val_t *root, const char *path,
                              json_write_flags_t flags);

/// Write JSON to FILE stream.
///
/// @param w Writer
/// @param root Root value
/// @param fp File pointer
/// @param flags Output formatting flags
/// @return Result
json_result_t json_write_fp(json_writer_t *w, yyjson_mut_val_t *root, FILE *fp,
                            json_write_flags_t flags);

#endif // DIV0_FREESTANDING
#endif // DIV0_JSON_WRITE_H
