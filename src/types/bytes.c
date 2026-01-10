#include "div0/types/bytes.h"

#include "div0/mem/arena.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Minimum capacity when growing
static constexpr size_t MIN_CAPACITY = 32;

bool bytes_reserve(bytes_t *const b, const size_t capacity) {
  if (capacity <= b->capacity) {
    return true; // Already have enough capacity
  }

  if (b->arena != nullptr) {
    // Arena-backed: allocate from arena (one-time, no realloc)
    if (b->data != nullptr) {
      // Arena doesn't support realloc - cannot grow
      return false;
    }
    b->data = (uint8_t *)div0_arena_alloc(b->arena, capacity);
    if (b->data == nullptr) {
      return false;
    }
    b->capacity = capacity;
    return true;
  }

  // Malloc-backed: use realloc
  size_t new_capacity = b->capacity == 0 ? MIN_CAPACITY : b->capacity;
  while (new_capacity < capacity) {
    new_capacity *= 2;
  }

  const auto new_data = (uint8_t *)realloc(b->data, new_capacity);
  if (new_data == nullptr) {
    return false;
  }

  b->data = new_data;
  b->capacity = new_capacity;
  return true;
}

bool bytes_from_data(bytes_t *const b, const uint8_t *const data, const size_t len) {
  if (data == nullptr || len == 0) {
    b->size = 0;
    return true;
  }

  if (!bytes_reserve(b, len)) {
    return false;
  }

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(b->data, data, len);
  b->size = len;
  return true;
}

bool bytes_append(bytes_t *const b, const uint8_t *const data, const size_t len) {
  if (data == nullptr || len == 0) {
    return true;
  }

  const size_t new_size = b->size + len;
  if (new_size < b->size) {
    // Overflow
    return false;
  }

  if (!bytes_reserve(b, new_size)) {
    return false;
  }

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(b->data + b->size, data, len);
  b->size = new_size;
  return true;
}

bool bytes_append_byte(bytes_t *const b, const uint8_t byte) {
  return bytes_append(b, &byte, 1);
}

void bytes_free(bytes_t *b) {
  if (b->arena != nullptr) {
    // Arena-backed: no-op (arena owns the memory)
    b->data = nullptr;
    b->size = 0;
    b->capacity = 0;
    return;
  }

  // Malloc-backed: free the memory
  free(b->data);
  b->data = nullptr;
  b->size = 0;
  b->capacity = 0;
}