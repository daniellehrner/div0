#include "div0/json/write.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

#include <stdlib.h>
#include <yyjson.h>

// ============================================================================
// Writer Handling
// ============================================================================

json_result_t json_writer_init(json_writer_t *const w) {
  if (w == nullptr) {
    return json_err(JSON_ERR_ALLOC, "null writer");
  }

  w->doc = yyjson_mut_doc_new(nullptr);
  if (w->doc == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to create document");
  }

  return json_ok();
}

void json_writer_free(json_writer_t *const w) {
  if (w != nullptr && w->doc != nullptr) {
    yyjson_mut_doc_free(w->doc);
    w->doc = nullptr;
  }
}

// ============================================================================
// Value Creation
// ============================================================================

yyjson_mut_val_t *json_write_null(const json_writer_t *const w) {
  return yyjson_mut_null(w->doc);
}

yyjson_mut_val_t *json_write_bool(const json_writer_t *const w, const bool val) {
  return yyjson_mut_bool(w->doc, val);
}

yyjson_mut_val_t *json_write_u64(const json_writer_t *const w, const uint64_t val) {
  return yyjson_mut_uint(w->doc, val);
}

yyjson_mut_val_t *json_write_i64(const json_writer_t *const w, const int64_t val) {
  return yyjson_mut_sint(w->doc, val);
}

yyjson_mut_val_t *json_write_str(const json_writer_t *const w, const char *const str) {
  return yyjson_mut_strcpy(w->doc, str);
}

yyjson_mut_val_t *json_write_strn(const json_writer_t *const w, const char *str, const size_t len) {
  return yyjson_mut_strncpy(w->doc, str, len);
}

yyjson_mut_val_t *json_write_obj(const json_writer_t *const w) {
  return yyjson_mut_obj(w->doc);
}

yyjson_mut_val_t *json_write_arr(const json_writer_t *const w) {
  return yyjson_mut_arr(w->doc);
}

// ============================================================================
// Object Operations
// ============================================================================

bool json_obj_add(const json_writer_t *const w, yyjson_mut_val_t *const obj, const char *const key,
                  yyjson_mut_val_t *const val) {
  if (obj == nullptr || val == nullptr) {
    return false;
  }
  // Create a copy of the key string and add it with the value
  yyjson_mut_val_t *const key_val = yyjson_mut_strcpy(w->doc, key);
  if (key_val == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add(obj, key_val, val);
}

bool json_obj_add_null(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                       const char *const key) {
  return yyjson_mut_obj_add_null(w->doc, obj, key);
}

bool json_obj_add_bool(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                       const char *const key, const bool val) {
  return yyjson_mut_obj_add_bool(w->doc, obj, key, val);
}

bool json_obj_add_u64(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                      const char *const key, const uint64_t val) {
  return yyjson_mut_obj_add_uint(w->doc, obj, key, val);
}

bool json_obj_add_str(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                      const char *const key, const char *const val) {
  return yyjson_mut_obj_add_strcpy(w->doc, obj, key, val);
}

// ============================================================================
// Array Operations
// ============================================================================

bool json_arr_append(const json_writer_t *const w, yyjson_mut_val_t *const arr,
                     yyjson_mut_val_t *const val) {
  (void)w; // Writer not needed for append
  if (arr == nullptr || val == nullptr) {
    return false;
  }
  return yyjson_mut_arr_append(arr, val);
}

// ============================================================================
// Hex-Encoded Value Creation
// ============================================================================

yyjson_mut_val_t *json_write_hex_u64(const json_writer_t *const w, const uint64_t val) {
  char buf[19]; // "0x" + 16 digits + null
  hex_encode_u64(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_uint256(const json_writer_t *const w, const uint256_t *const val) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode_uint256(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_uint256_padded(const json_writer_t *const w,
                                                const uint256_t *const val) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode_uint256_padded(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_address(const json_writer_t *const w,
                                         const address_t *const addr) {
  char buf[43]; // "0x" + 40 digits + null
  hex_encode(addr->bytes, ADDRESS_SIZE, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_hash(const json_writer_t *const w, const hash_t *const hash) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode(hash->bytes, HASH_SIZE, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_bytes(const json_writer_t *const w, const uint8_t *const data,
                                       const size_t len) {
  if (len == 0) {
    return yyjson_mut_strcpy(w->doc, "0x");
  }

  // Allocate buffer: "0x" + 2*len + null
  const size_t buf_len = 2 + (len * 2) + 1;
  char *const buf = malloc(buf_len);
  if (buf == nullptr) {
    return nullptr;
  }

  hex_encode(data, len, buf);
  yyjson_mut_val_t *const val = yyjson_mut_strcpy(w->doc, buf);
  free(buf);
  return val;
}

// ============================================================================
// Object Helpers for Hex Values
// ============================================================================

bool json_obj_add_hex_u64(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                          const char *const key, const uint64_t val) {
  yyjson_mut_val_t *const v = json_write_hex_u64(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_uint256(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                              const char *const key, const uint256_t *const val) {
  yyjson_mut_val_t *const v = json_write_hex_uint256(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_uint256_padded(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                                     const char *const key, const uint256_t *const val) {
  yyjson_mut_val_t *const v = json_write_hex_uint256_padded(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_address(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                              const char *const key, const address_t *const addr) {
  yyjson_mut_val_t *const v = json_write_hex_address(w, addr);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_hash(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                           const char *const key, const hash_t *const hash) {
  yyjson_mut_val_t *const v = json_write_hex_hash(w, hash);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_bytes(const json_writer_t *const w, yyjson_mut_val_t *const obj,
                            const char *const key, const uint8_t *const data, const size_t len) {
  yyjson_mut_val_t *const v = json_write_hex_bytes(w, data, len);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

// ============================================================================
// Output
// ============================================================================

static yyjson_write_flag convert_flags(const json_write_flags_t flags) {
  if (flags & JSON_WRITE_PRETTY) {
    return YYJSON_WRITE_PRETTY;
  }
  return 0;
}

char *json_write_string(const json_writer_t *const w, yyjson_mut_val_t *const root,
                        const json_write_flags_t flags, size_t *const out_len) {
  if (w == nullptr || w->doc == nullptr || root == nullptr) {
    return nullptr;
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  char *const str = yyjson_mut_write_opts(w->doc, convert_flags(flags), nullptr, out_len, &err);

  return str;
}

json_result_t json_write_file(const json_writer_t *const w, yyjson_mut_val_t *const root,
                              const char *const path, const json_write_flags_t flags) {
  if (w == nullptr || w->doc == nullptr || root == nullptr || path == nullptr) {
    return json_err(JSON_ERR_IO, "null input");
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  const bool ok = yyjson_mut_write_file(path, w->doc, convert_flags(flags), nullptr, &err);

  if (!ok) {
    return json_err(JSON_ERR_IO, err.msg);
  }

  return json_ok();
}

json_result_t json_write_fp(const json_writer_t *const w, yyjson_mut_val_t *const root,
                            FILE *const fp, const json_write_flags_t flags) {
  if (w == nullptr || w->doc == nullptr || root == nullptr || fp == nullptr) {
    return json_err(JSON_ERR_IO, "null input");
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  const bool ok = yyjson_mut_write_fp(fp, w->doc, convert_flags(flags), nullptr, &err);

  if (!ok) {
    return json_err(JSON_ERR_IO, err.msg);
  }

  return json_ok();
}

#endif // DIV0_FREESTANDING
