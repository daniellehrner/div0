#ifndef DIV0_TYPES_BYTES_H
#define DIV0_TYPES_BYTES_H

#include "div0/mem/arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Variable-length byte array.
/// Can be backed by malloc or arena allocation.
typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
  div0_arena_t *arena; // nullptr = malloc, non-nullptr = arena allocation
} bytes_t;

/// Initializes an empty bytes object (malloc-backed).
/// @param b Pointer to bytes_t to initialize
static inline void bytes_init(bytes_t *b) {
  b->data = nullptr;
  b->size = 0;
  b->capacity = 0;
  b->arena = nullptr;
}

/// Initializes an empty bytes object with arena backing.
/// @param b Pointer to bytes_t to initialize
/// @param arena Arena to use for allocations
static inline void bytes_init_arena(bytes_t *b, div0_arena_t *arena) {
  b->data = nullptr;
  b->size = 0;
  b->capacity = 0;
  b->arena = arena;
}

/// Checks if bytes is empty (size == 0).
static inline bool bytes_is_empty(const bytes_t *b) {
  return b->size == 0;
}

/// Clears the bytes (sets size to 0, keeps capacity).
static inline void bytes_clear(bytes_t *b) {
  b->size = 0;
}

/// Reserves capacity for at least `capacity` bytes.
/// For arena-backed bytes, this is a one-time allocation.
/// @param b Pointer to bytes_t
/// @param capacity Minimum capacity to reserve
/// @return true on success, false on allocation failure
bool bytes_reserve(bytes_t *b, size_t capacity);

/// Creates bytes from existing data (copies the data).
/// @param b Pointer to bytes_t (must be initialized)
/// @param data Source data to copy
/// @param len Number of bytes to copy
/// @return true on success, false on allocation failure
bool bytes_from_data(bytes_t *b, const uint8_t *data, size_t len);

/// Appends data to the end of bytes.
/// For arena-backed bytes, fails if capacity is exceeded.
/// @param b Pointer to bytes_t
/// @param data Data to append
/// @param len Number of bytes to append
/// @return true on success, false on allocation failure or capacity exceeded
bool bytes_append(bytes_t *b, const uint8_t *data, size_t len);

/// Appends a single byte.
/// @param b Pointer to bytes_t
/// @param byte Byte to append
/// @return true on success, false on allocation failure or capacity exceeded
bool bytes_append_byte(bytes_t *b, uint8_t byte);

/// Frees the bytes memory.
/// For arena-backed bytes, this is a no-op (arena owns the memory).
/// @param b Pointer to bytes_t
void bytes_free(bytes_t *b);

#endif // DIV0_TYPES_BYTES_H