#ifndef DIV0_TRIE_NIBBLES_H
#define DIV0_TRIE_NIBBLES_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "div0/types/bytes.h"

namespace div0::trie {

/// Nibble sequence (each element is 0-15)
using Nibbles = std::vector<uint8_t>;

/**
 * @brief Convert bytes to nibbles.
 *
 * Each byte becomes two nibbles (high nibble first).
 * Example: 0xAB -> [0x0A, 0x0B]
 *
 * @param bytes Input bytes
 * @return Nibble sequence (2 nibbles per byte)
 */
[[nodiscard]] Nibbles bytes_to_nibbles(std::span<const uint8_t> bytes);

/**
 * @brief Convert nibbles to bytes.
 *
 * Pairs of nibbles become bytes (high nibble first).
 * Requires even number of nibbles.
 * Example: [0x0A, 0x0B] -> 0xAB
 *
 * @param nibbles Input nibbles (must have even length)
 * @return Byte sequence
 */
[[nodiscard]] types::Bytes nibbles_to_bytes(const Nibbles& nibbles);

/**
 * @brief Find common prefix length of two nibble sequences.
 *
 * @param a First nibble sequence
 * @param b Second nibble sequence
 * @return Number of matching nibbles from the start
 */
[[nodiscard]] size_t common_prefix_length(const Nibbles& a, const Nibbles& b);

/**
 * @brief Extract a slice of nibbles.
 *
 * @param nibbles Source nibble sequence
 * @param start Start index
 * @param len Number of nibbles (default: to end)
 * @return Nibble subsequence
 */
[[nodiscard]] Nibbles nibbles_slice(const Nibbles& nibbles, size_t start,
                                    size_t len = static_cast<size_t>(-1));

}  // namespace div0::trie

#endif  // DIV0_TRIE_NIBBLES_H
