#ifndef DIV0_TRIE_HEX_PREFIX_H
#define DIV0_TRIE_HEX_PREFIX_H

#include "div0/mem/arena.h"
#include "div0/trie/nibbles.h"
#include "div0/types/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Hex-prefix (compact) encoding for MPT node paths.
///
/// Encoding rules:
/// - First nibble contains flags: bit 1 (0x20) = is_leaf, bit 0 (0x10) = odd_length
/// - If odd length: first byte = (flags | first_nibble), then remaining nibble pairs
/// - If even length: first byte = (flags << 4), then all nibble pairs
///
/// Examples:
///   [1, 2, 3, 4, 5], leaf=false -> [0x11, 0x23, 0x45]  (odd extension)
///   [0, 1, 2, 3, 4, 5], leaf=false -> [0x00, 0x01, 0x23, 0x45]  (even extension)
///   [1, 2, 3, 4, 5], leaf=true -> [0x31, 0x23, 0x45]  (odd leaf)
///   [0, 1, 2, 3, 4, 5], leaf=true -> [0x20, 0x01, 0x23, 0x45]  (even leaf)

/// Hex-prefix decode result.
typedef struct {
  nibbles_t nibbles; // Decoded nibbles
  bool is_leaf;      // True if this is a leaf node path
  bool success;      // True if decoding succeeded
} hex_prefix_result_t;

/// Encode nibbles using hex-prefix (compact) encoding.
/// @param nibbles Source nibble sequence
/// @param is_leaf True for leaf nodes, false for extension nodes
/// @param arena Arena for allocation
/// @return Encoded bytes
[[nodiscard]] bytes_t hex_prefix_encode(const nibbles_t *nibbles, bool is_leaf,
                                        div0_arena_t *arena);

/// Decode hex-prefix encoded data back to nibbles.
/// @param data Encoded data
/// @param len Length of encoded data
/// @param arena Arena for allocation
/// @return Decode result containing nibbles and is_leaf flag
[[nodiscard]] hex_prefix_result_t hex_prefix_decode(const uint8_t *data, size_t len,
                                                    div0_arena_t *arena);

#endif // DIV0_TRIE_HEX_PREFIX_H
