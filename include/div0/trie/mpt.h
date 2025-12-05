#ifndef DIV0_TRIE_MPT_H
#define DIV0_TRIE_MPT_H

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "div0/trie/nibbles.h"
#include "div0/trie/node.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

namespace div0::trie {

/**
 * @brief Merkle Patricia Trie implementation.
 *
 * A pure trie data structure with no Ethereum-specific semantics.
 * Maps arbitrary byte keys to byte values and computes the root hash.
 *
 * Design:
 * - Build-once: Optimized for building from a set of key-value pairs
 * - No node storage: Nodes are computed during root hash calculation
 * - Stateless hashing: Hash is recomputed each time (no caching)
 *
 * Usage:
 *   MerklePatriciaTrie trie;
 *   trie.insert(key1, value1);
 *   trie.insert(key2, value2);
 *   auto root = trie.root_hash();
 */
class MerklePatriciaTrie {
 public:
  /// Empty trie root hash (keccak256 of RLP-encoded empty string: 0x80)
  /// = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
  static const types::Hash EMPTY_ROOT;

  /// Default constructor: creates empty trie
  MerklePatriciaTrie() = default;

  /// Construct from a list of key-value pairs
  explicit MerklePatriciaTrie(std::span<const std::pair<types::Bytes, types::Bytes>> items);

  /// Insert a key-value pair
  /// @param key Raw key bytes (will be converted to nibbles internally)
  /// @param value Value bytes to store
  void insert(std::span<const uint8_t> key, std::span<const uint8_t> value);

  /// Insert a key-value pair (convenience overload)
  void insert(const types::Bytes& key, const types::Bytes& value) {
    insert(std::span<const uint8_t>(key), std::span<const uint8_t>(value));
  }

  /// Get value for a key
  /// @return Value if found, nullopt otherwise
  [[nodiscard]] std::optional<types::Bytes> get(std::span<const uint8_t> key) const;

  /// Compute the root hash
  /// @return Root hash of the trie (EMPTY_ROOT if empty)
  [[nodiscard]] types::Hash root_hash() const;

  /// Check if the trie is empty
  [[nodiscard]] bool empty() const noexcept { return items_.empty(); }

  /// Get number of items in the trie
  [[nodiscard]] size_t size() const noexcept { return items_.size(); }

  /// Clear all items
  void clear() noexcept { items_.clear(); }

 private:
  /// Internal storage: nibble path -> value
  /// Using vector for now; can optimize to sorted vector for cache locality
  std::vector<std::pair<Nibbles, types::Bytes>> items_;

  /// Build a node for a subset of items with a common prefix
  /// @param items Items sharing the common prefix
  /// @param prefix_len Length of the common prefix (already consumed)
  /// @return The constructed node
  [[nodiscard]] Node build_node(std::span<const std::pair<Nibbles, types::Bytes>> items,
                                size_t prefix_len) const;

  /// Compute hash of a node, handling embedding vs hashing
  [[nodiscard]] NodeRef hash_subtree(std::span<const std::pair<Nibbles, types::Bytes>> items,
                                     size_t prefix_len) const;
};

}  // namespace div0::trie

#endif  // DIV0_TRIE_MPT_H
