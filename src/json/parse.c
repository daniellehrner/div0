#include "div0/json/parse.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

#include <yyjson.h>

// ============================================================================
// Document Handling
// ============================================================================

json_result_t json_parse(const char *const json, const size_t len, json_doc_t *const doc) {
  if (json == nullptr || doc == nullptr) {
    return json_err(JSON_ERR_PARSE, "null input");
  }

  yyjson_read_err err;
  doc->doc = yyjson_read_opts((char *)json, len, 0, nullptr, &err);

  if (doc->doc == nullptr) {
    return json_err(JSON_ERR_PARSE, err.msg);
  }

  return json_ok();
}

json_result_t json_parse_file(const char *path, json_doc_t *doc) {
  if (path == nullptr || doc == nullptr) {
    return json_err(JSON_ERR_IO, "null input");
  }

  yyjson_read_err err;
  doc->doc = yyjson_read_file(path, 0, nullptr, &err);

  if (doc->doc == nullptr) {
    if (err.code == YYJSON_READ_ERROR_FILE_OPEN) {
      return json_err(JSON_ERR_IO, err.msg);
    }
    return json_err(JSON_ERR_PARSE, err.msg);
  }

  return json_ok();
}

void json_doc_free(json_doc_t *doc) {
  if (doc != nullptr && doc->doc != nullptr) {
    yyjson_doc_free(doc->doc);
    doc->doc = nullptr;
  }
}

yyjson_val_t *json_doc_root(const json_doc_t *doc) {
  if (doc == nullptr || doc->doc == nullptr) {
    return nullptr;
  }
  return yyjson_doc_get_root(doc->doc);
}

// ============================================================================
// Type Checking
// ============================================================================

bool json_is_null(yyjson_val_t *val) {
  return yyjson_is_null(val);
}

bool json_is_bool(yyjson_val_t *val) {
  return yyjson_is_bool(val);
}

bool json_is_num(yyjson_val_t *val) {
  return yyjson_is_num(val);
}

bool json_is_str(yyjson_val_t *val) {
  return yyjson_is_str(val);
}

bool json_is_obj(yyjson_val_t *val) {
  return yyjson_is_obj(val);
}

bool json_is_arr(yyjson_val_t *val) {
  return yyjson_is_arr(val);
}

// ============================================================================
// Value Accessors
// ============================================================================

yyjson_val_t *json_obj_get(yyjson_val_t *obj, const char *key) {
  return yyjson_obj_get(obj, key);
}

yyjson_val_t *json_arr_get(yyjson_val_t *const arr, const size_t idx) {
  return yyjson_arr_get(arr, idx);
}

size_t json_arr_len(yyjson_val_t *arr) {
  return yyjson_arr_size(arr);
}

size_t json_obj_size(yyjson_val_t *obj) {
  return yyjson_obj_size(obj);
}

const char *json_get_str(yyjson_val_t *val) {
  return yyjson_get_str(val);
}

size_t json_get_str_len(yyjson_val_t *val) {
  return yyjson_get_len(val);
}

bool json_get_bool(yyjson_val_t *val) {
  return yyjson_get_bool(val);
}

uint64_t json_get_u64(yyjson_val_t *val) {
  return yyjson_get_uint(val);
}

int64_t json_get_i64(yyjson_val_t *val) {
  return yyjson_get_sint(val);
}

// ============================================================================
// Hex-Encoded Value Parsing (from fields)
// ============================================================================

bool json_get_hex_u64(yyjson_val_t *obj, const char *key, uint64_t *out) {
  yyjson_val_t *const val = yyjson_obj_get(obj, key);
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  return hex_decode_u64(str, out);
}

bool json_get_hex_uint256(yyjson_val_t *obj, const char *key, uint256_t *out) {
  yyjson_val_t *const val = yyjson_obj_get(obj, key);
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  return hex_decode_uint256(str, out);
}

bool json_get_hex_address(yyjson_val_t *obj, const char *key, address_t *out) {
  yyjson_val_t *const val = yyjson_obj_get(obj, key);
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  return hex_decode(str, out->bytes, ADDRESS_SIZE);
}

bool json_get_hex_hash(yyjson_val_t *obj, const char *key, hash_t *out) {
  yyjson_val_t *const val = yyjson_obj_get(obj, key);
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  return hex_decode(str, out->bytes, HASH_SIZE);
}

bool json_get_hex_bytes(yyjson_val_t *obj, const char *key, div0_arena_t *arena, bytes_t *out) {
  yyjson_val_t *const val = yyjson_obj_get(obj, key);
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  const size_t hex_len = hex_strlen(str);

  // Empty bytes
  if (hex_len == 0) {
    out->data = nullptr;
    out->size = 0;
    return true;
  }

  // Must have even number of hex chars
  if (hex_len % 2 != 0) {
    return false;
  }

  const size_t byte_len = hex_len / 2;
  uint8_t *const data = div0_arena_alloc(arena, byte_len);
  if (data == nullptr) {
    return false;
  }

  if (!hex_decode(str, data, byte_len)) {
    return false;
  }

  out->data = data;
  out->size = byte_len;
  return true;
}

// ============================================================================
// Hex-Encoded Value Parsing (from values)
// ============================================================================

bool json_val_hex_u64(yyjson_val_t *val, uint64_t *out) {
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *str = yyjson_get_str(val);
  return hex_decode_u64(str, out);
}

bool json_val_hex_uint256(yyjson_val_t *val, uint256_t *out) {
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *str = yyjson_get_str(val);
  return hex_decode_uint256(str, out);
}

bool json_val_hex_address(yyjson_val_t *val, address_t *out) {
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *str = yyjson_get_str(val);
  return hex_decode(str, out->bytes, ADDRESS_SIZE);
}

bool json_val_hex_hash(yyjson_val_t *val, hash_t *out) {
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *str = yyjson_get_str(val);
  return hex_decode(str, out->bytes, HASH_SIZE);
}

bool json_val_hex_bytes(yyjson_val_t *val, div0_arena_t *arena, bytes_t *out) {
  if (val == nullptr || !yyjson_is_str(val)) {
    return false;
  }
  const char *const str = yyjson_get_str(val);
  const size_t hex_len = hex_strlen(str);

  // Empty bytes
  if (hex_len == 0) {
    out->data = nullptr;
    out->size = 0;
    return true;
  }

  // Must have even number of hex chars
  if (hex_len % 2 != 0) {
    return false;
  }

  const size_t byte_len = hex_len / 2;
  uint8_t *const data = div0_arena_alloc(arena, byte_len);
  if (data == nullptr) {
    return false;
  }

  if (!hex_decode(str, data, byte_len)) {
    return false;
  }

  out->data = data;
  out->size = byte_len;
  return true;
}

// ============================================================================
// Object Iteration
// ============================================================================

json_obj_iter_t json_obj_iter(yyjson_val_t *obj) {
  json_obj_iter_t iter = {.cur = nullptr, .idx = 0, .max = 0};
  if (obj == nullptr || !yyjson_is_obj(obj)) {
    return iter;
  }
  iter.max = yyjson_obj_size(obj);
  if (iter.max > 0) {
    // First key is right after the object header
    iter.cur = unsafe_yyjson_get_first(obj);
  }
  return iter;
}

bool json_obj_iter_next(json_obj_iter_t *iter, const char **key, yyjson_val_t **val) {
  if (iter == nullptr || iter->idx >= iter->max) {
    return false;
  }

  *key = yyjson_get_str(iter->cur);
  *val = iter->cur + 1; // Value follows key

  iter->cur = unsafe_yyjson_get_next(iter->cur + 1); // Next key after value
  iter->idx++;

  return true;
}

// ============================================================================
// Array Iteration
// ============================================================================

json_arr_iter_t json_arr_iter(yyjson_val_t *arr) {
  json_arr_iter_t iter = {.cur = nullptr, .idx = 0, .max = 0};
  if (arr == nullptr || !yyjson_is_arr(arr)) {
    return iter;
  }
  iter.max = yyjson_arr_size(arr);
  if (iter.max > 0) {
    iter.cur = unsafe_yyjson_get_first(arr);
  }
  return iter;
}

bool json_arr_iter_next(json_arr_iter_t *iter, yyjson_val_t **val) {
  if (iter == nullptr || iter->idx >= iter->max) {
    return false;
  }

  *val = iter->cur;
  iter->cur = unsafe_yyjson_get_next(iter->cur);
  iter->idx++;

  return true;
}

#endif // DIV0_FREESTANDING
