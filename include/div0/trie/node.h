#ifndef DIV0_TRIE_NODE_H
#define DIV0_TRIE_NODE_H

#include <array>
#include <cassert>
#include <span>
#include <variant>

#include "div0/trie/nibbles.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

namespace div0::trie {

/**
 * @brief Reference to a child node in the trie.
 *
 * A child can be either:
 * - Embedded: Raw RLP-encoded bytes (when encoded size < 32 bytes)
 * - Hashed: Keccak-256 hash of the RLP-encoded node
 *
 * The empty state (both Bytes and Hash are "zero") represents null/absent children.
 */
using NodeRef = std::variant<types::Bytes, types::Hash>;

/**
 * @brief Check if a NodeRef is null (represents absence of child).
 */
[[nodiscard]] inline bool is_null_ref(const NodeRef& ref) noexcept {
  if (const auto* bytes = std::get_if<types::Bytes>(&ref)) {
    return bytes->empty();
  }
  const auto* hash = std::get_if<types::Hash>(&ref);
  assert(hash != nullptr && "NodeRef must be Bytes or Hash");
  return hash->is_zero();
}

/**
 * @brief Create a null NodeRef (empty Bytes).
 */
[[nodiscard]] inline NodeRef null_ref() noexcept { return types::Bytes{}; }

// =============================================================================
// Node Types
// =============================================================================

/**
 * @brief Leaf node: terminates a path with a value.
 *
 * Contains:
 * - path: Remaining nibbles of the key (hex-prefix encoded with leaf flag)
 * - value: The stored value bytes
 */
struct LeafNode {
  Nibbles path;
  types::Bytes value;
};

/**
 * @brief Extension node: shared path prefix optimization.
 *
 * Contains:
 * - path: Shared nibbles (hex-prefix encoded without leaf flag)
 * - child: Reference to the next node (must be a branch node)
 */
struct ExtensionNode {
  Nibbles path;
  NodeRef child;
};

/**
 * @brief Branch node: 16-way branch point plus optional value.
 *
 * Contains:
 * - children[16]: One slot per nibble (0-15), each a NodeRef
 * - value: Optional value if a key terminates at this branch
 */
struct BranchNode {
  std::array<NodeRef, 16> children;
  types::Bytes value;

  BranchNode() noexcept {
    // Initialize all children to null
    for (auto& child : children) {
      child = null_ref();
    }
  }
};

/**
 * @brief A trie node: one of Leaf, Extension, or Branch.
 */
using Node = std::variant<LeafNode, ExtensionNode, BranchNode>;

// =============================================================================
// Node Encoding (RLP)
// =============================================================================

/**
 * @brief RLP-encode a node to bytes.
 *
 * Leaf: [hex_prefix(path, is_leaf=true), value]
 * Extension: [hex_prefix(path, is_leaf=false), child_ref]
 * Branch: [child0, ..., child15, value]
 */
[[nodiscard]] types::Bytes encode_node(const Node& node);

/**
 * @brief Compute the hash or embedded encoding for a node.
 *
 * If the RLP-encoded node is < 32 bytes, returns the raw bytes (embedded).
 * Otherwise, returns the Keccak-256 hash.
 */
[[nodiscard]] NodeRef hash_node(const Node& node);

/**
 * @brief Get the hash of a node (always computes hash, even if embeddable).
 *
 * Used for computing the root hash.
 */
[[nodiscard]] types::Hash compute_node_hash(const Node& node);

// =============================================================================
// Node Helpers
// =============================================================================

/**
 * @brief Check if a node is a leaf.
 */
[[nodiscard]] inline bool is_leaf(const Node& node) noexcept {
  return std::holds_alternative<LeafNode>(node);
}

/**
 * @brief Check if a node is an extension.
 */
[[nodiscard]] inline bool is_extension(const Node& node) noexcept {
  return std::holds_alternative<ExtensionNode>(node);
}

/**
 * @brief Check if a node is a branch.
 */
[[nodiscard]] inline bool is_branch(const Node& node) noexcept {
  return std::holds_alternative<BranchNode>(node);
}

/**
 * @brief Count non-null children in a branch node.
 */
[[nodiscard]] size_t count_children(const BranchNode& branch) noexcept;

}  // namespace div0::trie

#endif  // DIV0_TRIE_NODE_H
