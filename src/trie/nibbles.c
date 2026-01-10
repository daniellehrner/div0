#include "div0/trie/nibbles.h"

#include <string.h>

nibbles_t nibbles_from_bytes(const uint8_t *const bytes, const size_t len,
                             div0_arena_t *const arena) {
  if (len == 0 || bytes == nullptr) {
    return NIBBLES_EMPTY;
  }

  const size_t nibble_len = len * 2;
  uint8_t *const data = div0_arena_alloc(arena, nibble_len);
  if (data == nullptr) {
    return NIBBLES_EMPTY;
  }

  for (size_t i = 0; i < len; i++) {
    data[i * 2] = (bytes[i] >> 4) & 0x0F; // High nibble
    data[(i * 2) + 1] = bytes[i] & 0x0F;  // Low nibble
  }

  return (nibbles_t){.data = data, .len = nibble_len};
}

void nibbles_to_bytes(const nibbles_t *const nibbles, uint8_t *const out) {
  if (nibbles->len == 0 || out == nullptr) {
    return;
  }

  const size_t byte_len = nibbles->len / 2;
  for (size_t i = 0; i < byte_len; i++) {
    out[i] = (uint8_t)((nibbles->data[i * 2] << 4) | nibbles->data[(i * 2) + 1]);
  }
}

uint8_t *nibbles_to_bytes_alloc(const nibbles_t *const nibbles, div0_arena_t *const arena) {
  if (nibbles->len == 0) {
    return nullptr;
  }

  const size_t byte_len = nibbles->len / 2;
  uint8_t *const out = div0_arena_alloc(arena, byte_len);
  if (out == nullptr) {
    return nullptr;
  }

  nibbles_to_bytes(nibbles, out);
  return out;
}

size_t nibbles_common_prefix(const nibbles_t *const a, const nibbles_t *const b) {
  const size_t min_len = a->len < b->len ? a->len : b->len;
  size_t i = 0;

  while (i < min_len && a->data[i] == b->data[i]) {
    i++;
  }

  return i;
}

nibbles_t nibbles_slice(const nibbles_t *const src, const size_t start, size_t len,
                        div0_arena_t *const arena) {
  if (src->len == 0 || start >= src->len) {
    return NIBBLES_EMPTY;
  }

  // Clamp len to available nibbles
  const size_t remaining = src->len - start;
  if (len == SIZE_MAX || len > remaining) {
    len = remaining;
  }

  if (len == 0) {
    return NIBBLES_EMPTY;
  }

  // If no arena provided, return a view into the original data
  if (arena == nullptr) {
    return (nibbles_t){.data = src->data + start, .len = len};
  }

  // Allocate and copy
  uint8_t *const data = div0_arena_alloc(arena, len);
  if (data == nullptr) {
    return NIBBLES_EMPTY;
  }

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(data, src->data + start, len);
  return (nibbles_t){.data = data, .len = len};
}

nibbles_t nibbles_copy(const nibbles_t *const src, div0_arena_t *const arena) {
  if (src->len == 0) {
    return NIBBLES_EMPTY;
  }

  uint8_t *const data = div0_arena_alloc(arena, src->len);
  if (data == nullptr) {
    return NIBBLES_EMPTY;
  }

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(data, src->data, src->len);
  return (nibbles_t){.data = data, .len = src->len};
}

int nibbles_cmp(const nibbles_t *const a, const nibbles_t *const b) {
  const size_t min_len = a->len < b->len ? a->len : b->len;

  for (size_t i = 0; i < min_len; i++) {
    if (a->data[i] < b->data[i]) {
      return -1;
    }
    if (a->data[i] > b->data[i]) {
      return 1;
    }
  }

  // Prefixes match - shorter sequence comes first
  if (a->len < b->len) {
    return -1;
  }
  if (a->len > b->len) {
    return 1;
  }

  return 0;
}

nibbles_t nibbles_concat(const nibbles_t *const a, const nibbles_t *const b,
                         div0_arena_t *const arena) {
  if (a->len == 0) {
    return nibbles_copy(b, arena);
  }
  if (b->len == 0) {
    return nibbles_copy(a, arena);
  }
  const size_t total = a->len + b->len;
  uint8_t *const data = div0_arena_alloc(arena, total);
  if (data == nullptr) {
    return NIBBLES_EMPTY;
  }
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(data, a->data, a->len);
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(data + a->len, b->data, b->len);
  return (nibbles_t){.data = data, .len = total};
}

nibbles_t nibbles_concat3(const nibbles_t *const prefix, const uint8_t middle,
                          const nibbles_t *const suffix, div0_arena_t *const arena) {
  const size_t total = prefix->len + 1 + suffix->len;
  uint8_t *const data = div0_arena_alloc(arena, total);
  if (data == nullptr) {
    return NIBBLES_EMPTY;
  }
  size_t pos = 0;
  if (prefix->len > 0) {
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(data + pos, prefix->data, prefix->len);
    pos += prefix->len;
  }
  data[pos++] = middle;
  if (suffix->len > 0) {
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(data + pos, suffix->data, suffix->len);
  }
  return (nibbles_t){.data = data, .len = total};
}
