#ifndef DIV0_TRIE_HEX_PREFIX_H
#define DIV0_TRIE_HEX_PREFIX_H

#include <span>
#include <utility>

#include "div0/trie/nibbles.h"
#include "div0/types/bytes.h"

namespace div0::trie {

/**
 * @brief Encode nibble path using hex-prefix (compact) encoding.
 *
 * Hex-prefix encoding compresses a nibble path into bytes while
 * encoding whether the path belongs to a leaf or extension node.
 *
 * Encoding rules:
 * - First nibble contains flags: bit 5 = is_leaf, bit 4 = odd_length
 * - If odd length: first byte = (flags << 4) | first_nibble
 * - If even length: first byte = (flags << 4), then nibble pairs
 *
 * @param nibbles Nibble path (each element 0-15)
 * @param is_leaf True for leaf nodes, false for extension nodes
 * @return Compact encoded bytes
 */
[[nodiscard]] types::Bytes hex_prefix_encode(const Nibbles& nibbles, bool is_leaf);

/**
 * @brief Decode hex-prefix encoded path.
 *
 * @param encoded Compact encoded bytes
 * @return Pair of (nibbles, is_leaf)
 */
[[nodiscard]] std::pair<Nibbles, bool> hex_prefix_decode(std::span<const uint8_t> encoded);

}  // namespace div0::trie

#endif  // DIV0_TRIE_HEX_PREFIX_H
