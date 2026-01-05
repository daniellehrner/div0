#include "div0/json/write.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

#include <stdlib.h>
#include <yyjson.h>

// ============================================================================
// Writer Handling
// ============================================================================

json_result_t json_writer_init(json_writer_t *w) {
  if (w == nullptr) {
    return json_err(JSON_ERR_ALLOC, "null writer");
  }

  w->doc = yyjson_mut_doc_new(nullptr);
  if (w->doc == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to create document");
  }

  return json_ok();
}

void json_writer_free(json_writer_t *w) {
  if (w != nullptr && w->doc != nullptr) {
    yyjson_mut_doc_free(w->doc);
    w->doc = nullptr;
  }
}

// ============================================================================
// Value Creation
// ============================================================================

yyjson_mut_val_t *json_write_null(json_writer_t *w) {
  return yyjson_mut_null(w->doc);
}

yyjson_mut_val_t *json_write_bool(json_writer_t *w, bool val) {
  return yyjson_mut_bool(w->doc, val);
}

yyjson_mut_val_t *json_write_u64(json_writer_t *w, uint64_t val) {
  return yyjson_mut_uint(w->doc, val);
}

yyjson_mut_val_t *json_write_i64(json_writer_t *w, int64_t val) {
  return yyjson_mut_sint(w->doc, val);
}

yyjson_mut_val_t *json_write_str(json_writer_t *w, const char *str) {
  return yyjson_mut_strcpy(w->doc, str);
}

yyjson_mut_val_t *json_write_strn(json_writer_t *w, const char *str, size_t len) {
  return yyjson_mut_strncpy(w->doc, str, len);
}

yyjson_mut_val_t *json_write_obj(json_writer_t *w) {
  return yyjson_mut_obj(w->doc);
}

yyjson_mut_val_t *json_write_arr(json_writer_t *w) {
  return yyjson_mut_arr(w->doc);
}

// ============================================================================
// Object Operations
// ============================================================================

bool json_obj_add(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, yyjson_mut_val_t *val) {
  if (obj == nullptr || val == nullptr) {
    return false;
  }
  // Create a copy of the key string and add it with the value
  yyjson_mut_val_t *key_val = yyjson_mut_strcpy(w->doc, key);
  if (key_val == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add(obj, key_val, val);
}

bool json_obj_add_null(json_writer_t *w, yyjson_mut_val_t *obj, const char *key) {
  return yyjson_mut_obj_add_null(w->doc, obj, key);
}

bool json_obj_add_bool(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, bool val) {
  return yyjson_mut_obj_add_bool(w->doc, obj, key, val);
}

bool json_obj_add_u64(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, uint64_t val) {
  return yyjson_mut_obj_add_uint(w->doc, obj, key, val);
}

bool json_obj_add_str(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, const char *val) {
  return yyjson_mut_obj_add_strcpy(w->doc, obj, key, val);
}

// ============================================================================
// Array Operations
// ============================================================================

bool json_arr_append(json_writer_t *w, yyjson_mut_val_t *arr, yyjson_mut_val_t *val) {
  (void)w; // Writer not needed for append
  if (arr == nullptr || val == nullptr) {
    return false;
  }
  return yyjson_mut_arr_append(arr, val);
}

// ============================================================================
// Hex-Encoded Value Creation
// ============================================================================

yyjson_mut_val_t *json_write_hex_u64(json_writer_t *w, uint64_t val) {
  char buf[19]; // "0x" + 16 digits + null
  hex_encode_u64(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_uint256(json_writer_t *w, const uint256_t *val) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode_uint256(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_uint256_padded(json_writer_t *w, const uint256_t *val) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode_uint256_padded(val, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_address(json_writer_t *w, const address_t *addr) {
  char buf[43]; // "0x" + 40 digits + null
  hex_encode(addr->bytes, ADDRESS_SIZE, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_hash(json_writer_t *w, const hash_t *hash) {
  char buf[67]; // "0x" + 64 digits + null
  hex_encode(hash->bytes, HASH_SIZE, buf);
  return yyjson_mut_strcpy(w->doc, buf);
}

yyjson_mut_val_t *json_write_hex_bytes(json_writer_t *w, const uint8_t *data, size_t len) {
  if (len == 0) {
    return yyjson_mut_strcpy(w->doc, "0x");
  }

  // Allocate buffer: "0x" + 2*len + null
  size_t buf_len = 2 + (len * 2) + 1;
  char *buf = malloc(buf_len);
  if (buf == nullptr) {
    return nullptr;
  }

  hex_encode(data, len, buf);
  yyjson_mut_val_t *val = yyjson_mut_strcpy(w->doc, buf);
  free(buf);
  return val;
}

// ============================================================================
// Object Helpers for Hex Values
// ============================================================================

bool json_obj_add_hex_u64(json_writer_t *w, yyjson_mut_val_t *obj, const char *key, uint64_t val) {
  yyjson_mut_val_t *v = json_write_hex_u64(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_uint256(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                              const uint256_t *val) {
  yyjson_mut_val_t *v = json_write_hex_uint256(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_uint256_padded(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                                     const uint256_t *val) {
  yyjson_mut_val_t *v = json_write_hex_uint256_padded(w, val);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_address(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                              const address_t *addr) {
  yyjson_mut_val_t *v = json_write_hex_address(w, addr);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_hash(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                           const hash_t *hash) {
  yyjson_mut_val_t *v = json_write_hex_hash(w, hash);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

bool json_obj_add_hex_bytes(json_writer_t *w, yyjson_mut_val_t *obj, const char *key,
                            const uint8_t *data, size_t len) {
  yyjson_mut_val_t *v = json_write_hex_bytes(w, data, len);
  if (v == nullptr) {
    return false;
  }
  return yyjson_mut_obj_add_val(w->doc, obj, key, v);
}

// ============================================================================
// Output
// ============================================================================

static yyjson_write_flag convert_flags(json_write_flags_t flags) {
  if (flags & JSON_WRITE_PRETTY) {
    return YYJSON_WRITE_PRETTY;
  }
  return 0;
}

char *json_write_string(json_writer_t *w, yyjson_mut_val_t *root, json_write_flags_t flags,
                        size_t *out_len) {
  if (w == nullptr || w->doc == nullptr || root == nullptr) {
    return nullptr;
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  char *str = yyjson_mut_write_opts(w->doc, convert_flags(flags), nullptr, out_len, &err);

  return str;
}

json_result_t json_write_file(json_writer_t *w, yyjson_mut_val_t *root, const char *path,
                              json_write_flags_t flags) {
  if (w == nullptr || w->doc == nullptr || root == nullptr || path == nullptr) {
    return json_err(JSON_ERR_IO, "null input");
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  bool ok = yyjson_mut_write_file(path, w->doc, convert_flags(flags), nullptr, &err);

  if (!ok) {
    return json_err(JSON_ERR_IO, err.msg);
  }

  return json_ok();
}

json_result_t json_write_fp(json_writer_t *w, yyjson_mut_val_t *root, FILE *fp,
                            json_write_flags_t flags) {
  if (w == nullptr || w->doc == nullptr || root == nullptr || fp == nullptr) {
    return json_err(JSON_ERR_IO, "null input");
  }

  yyjson_mut_doc_set_root(w->doc, root);

  yyjson_write_err err;
  bool ok = yyjson_mut_write_fp(fp, w->doc, convert_flags(flags), nullptr, &err);

  if (!ok) {
    return json_err(JSON_ERR_IO, err.msg);
  }

  return json_ok();
}

#endif // DIV0_FREESTANDING
