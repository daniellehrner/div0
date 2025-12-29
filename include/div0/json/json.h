#ifndef DIV0_JSON_JSON_H
#define DIV0_JSON_JSON_H

/// @file json.h
/// @brief Core JSON types and error handling.
///
/// This module provides JSON parsing and serialization for the t8n transition
/// tool. JSON functionality is only available in hosted builds (not bare-metal)
/// as it is not needed for zk proofing.

// JSON functionality is not available in bare-metal builds
#ifndef DIV0_FREESTANDING

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Error Types
// ============================================================================

/// JSON parse/write error codes.
typedef enum {
  JSON_OK = 0,            ///< Success
  JSON_ERR_PARSE,         ///< Invalid JSON syntax
  JSON_ERR_MISSING_FIELD, ///< Required field not present
  JSON_ERR_INVALID_TYPE,  ///< Field has wrong JSON type
  JSON_ERR_INVALID_HEX,   ///< Malformed hex string
  JSON_ERR_OVERFLOW,      ///< Value too large for target type
  JSON_ERR_ALLOC,         ///< Memory allocation failed
  JSON_ERR_IO,            ///< File I/O error
} json_error_t;

/// Result type for JSON operations.
typedef struct {
  json_error_t error; ///< Error code (JSON_OK on success)
  const char *detail; ///< Optional error detail (static string, may be nullptr)
} json_result_t;

// ============================================================================
// Result Helpers
// ============================================================================

/// Create a success result.
static inline json_result_t json_ok(void) {
  return (json_result_t){.error = JSON_OK, .detail = nullptr};
}

/// Create an error result with detail.
static inline json_result_t json_err(json_error_t code, const char *detail) {
  return (json_result_t){.error = code, .detail = detail};
}

/// Check if result is success.
static inline bool json_is_ok(json_result_t r) {
  return r.error == JSON_OK;
}

/// Check if result is an error.
static inline bool json_is_err(json_result_t r) {
  return r.error != JSON_OK;
}

/// Get error name string.
static inline const char *json_error_name(json_error_t err) {
  switch (err) {
  case JSON_OK:
    return "OK";
  case JSON_ERR_PARSE:
    return "PARSE_ERROR";
  case JSON_ERR_MISSING_FIELD:
    return "MISSING_FIELD";
  case JSON_ERR_INVALID_TYPE:
    return "INVALID_TYPE";
  case JSON_ERR_INVALID_HEX:
    return "INVALID_HEX";
  case JSON_ERR_OVERFLOW:
    return "OVERFLOW";
  case JSON_ERR_ALLOC:
    return "ALLOC_ERROR";
  case JSON_ERR_IO:
    return "IO_ERROR";
  default:
    return "UNKNOWN";
  }
}

#endif // DIV0_FREESTANDING
#endif // DIV0_JSON_JSON_H
