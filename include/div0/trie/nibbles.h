#ifndef DIV0_TRIE_NIBBLES_H
#define DIV0_TRIE_NIBBLES_H

#include "div0/mem/arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Nibble sequence - each element is 0-15 (4 bits stored as uint8_t).
/// Arena-backed for efficient memory management.
typedef struct {
  uint8_t *data; // Each element is 0-15
  size_t len;
} nibbles_t;

/// Empty nibbles constant.
static const nibbles_t NIBBLES_EMPTY = {.data = nullptr, .len = 0};

/// Convert bytes to nibbles (2 nibbles per byte, high nibble first).
/// Example: 0xAB -> [0x0A, 0x0B]
/// @param bytes Source bytes to convert
/// @param len Number of bytes
/// @param arena Arena for allocation
/// @return Nibble sequence (len * 2 nibbles)
[[nodiscard]] nibbles_t nibbles_from_bytes(const uint8_t *bytes, size_t len, div0_arena_t *arena);

/// Convert nibbles to bytes (2 nibbles -> 1 byte).
/// Requires even number of nibbles.
/// Example: [0x0A, 0x0B] -> 0xAB
/// @param nibbles Source nibbles (must have even length)
/// @param out Output buffer (must be at least nibbles->len / 2 bytes)
void nibbles_to_bytes(const nibbles_t *nibbles, uint8_t *out);

/// Allocate bytes and convert nibbles to bytes.
/// @param nibbles Source nibbles (must have even length)
/// @param arena Arena for allocation
/// @return Allocated bytes (len = nibbles->len / 2)
[[nodiscard]] uint8_t *nibbles_to_bytes_alloc(const nibbles_t *nibbles, div0_arena_t *arena);

/// Find the length of the common prefix between two nibble sequences.
/// @param a First nibble sequence
/// @param b Second nibble sequence
/// @return Number of matching nibbles from the start
[[nodiscard]] size_t nibbles_common_prefix(const nibbles_t *a, const nibbles_t *b);

/// Create a slice of nibbles [start, start + len).
/// Returns a view into the original data (no allocation if arena is nullptr).
/// @param src Source nibbles
/// @param start Start index
/// @param len Number of nibbles (SIZE_MAX = to end)
/// @param arena Arena for allocation (can be nullptr for view)
/// @return Slice of nibbles
[[nodiscard]] nibbles_t nibbles_slice(const nibbles_t *src, size_t start, size_t len,
                                      div0_arena_t *arena);

/// Create a copy of nibbles.
/// @param src Source nibbles to copy
/// @param arena Arena for allocation
/// @return Copied nibbles
[[nodiscard]] nibbles_t nibbles_copy(const nibbles_t *src, div0_arena_t *arena);

/// Compare two nibble sequences lexicographically.
/// @param a First nibble sequence
/// @param b Second nibble sequence
/// @return <0 if a < b, 0 if a == b, >0 if a > b
[[nodiscard]] int nibbles_cmp(const nibbles_t *a, const nibbles_t *b);

/// Check if two nibble sequences are equal.
/// @param a First nibble sequence
/// @param b Second nibble sequence
/// @return true if equal, false otherwise
[[nodiscard]] static inline bool nibbles_equal(const nibbles_t *a, const nibbles_t *b) {
  return nibbles_cmp(a, b) == 0;
}

/// Check if nibbles is empty.
/// @param n Nibbles to check
/// @return true if empty (len == 0)
[[nodiscard]] static inline bool nibbles_is_empty(const nibbles_t *n) {
  return n->len == 0;
}

/// Concatenate two nibble sequences.
/// @param a First nibble sequence
/// @param b Second nibble sequence
/// @param arena Arena for allocation
/// @return Concatenated nibbles (a + b)
[[nodiscard]] nibbles_t nibbles_concat(const nibbles_t *a, const nibbles_t *b, div0_arena_t *arena);

/// Concatenate nibbles: prefix + single nibble + suffix.
/// @param prefix First nibble sequence
/// @param middle Single nibble value (0-15)
/// @param suffix Last nibble sequence
/// @param arena Arena for allocation
/// @return Concatenated nibbles (prefix + middle + suffix)
[[nodiscard]] nibbles_t nibbles_concat3(const nibbles_t *prefix, uint8_t middle,
                                        const nibbles_t *suffix, div0_arena_t *arena);

/// Get nibble at index (no bounds checking).
/// @param n Nibbles
/// @param index Index
/// @return Nibble value (0-15)
[[nodiscard]] static inline uint8_t nibbles_get(const nibbles_t *n, size_t index) {
  return n->data[index];
}

#endif // DIV0_TRIE_NIBBLES_H
