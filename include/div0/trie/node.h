#ifndef DIV0_TRIE_NODE_H
#define DIV0_TRIE_NODE_H

#include "div0/mem/arena.h"
#include "div0/trie/nibbles.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Node types in the Merkle Patricia Trie.
typedef enum {
  MPT_NODE_EMPTY,     // Empty node (null)
  MPT_NODE_LEAF,      // Leaf node: terminates path with value
  MPT_NODE_EXTENSION, // Extension node: shared path prefix
  MPT_NODE_BRANCH     // Branch node: 16-way branch + optional value
} mpt_node_type_t;

/// Forward declaration
typedef struct mpt_node mpt_node_t;

/// Reference to a child node.
/// Can be either:
/// - Embedded bytes (RLP < 32 bytes) - stored inline
/// - Hash (RLP >= 32 bytes) - stored as keccak256 hash
/// For in-memory backends, the node pointer allows direct traversal.
/// Fields ordered to minimize padding.
typedef struct {
  union {
    bytes_t embedded; // RLP-encoded node (when < 32 bytes)
    hash_t hash;      // keccak256 of RLP-encoded node
  };
  mpt_node_t *node; // Direct pointer (for in-memory backends, nullptr for disk)
  bool is_hash;     // True if this is a hash reference
} node_ref_t;

/// Check if a node reference is null (empty).
[[nodiscard]] static inline bool node_ref_is_null(const node_ref_t *ref) {
  // For in-memory backends, check the node pointer first
  if (ref->node != nullptr) {
    return false;
  }
  if (ref->is_hash) {
    return hash_is_zero(&ref->hash);
  }
  return bytes_is_empty(&ref->embedded);
}

/// Create a null (empty) node reference.
[[nodiscard]] static inline node_ref_t node_ref_null(void) {
  node_ref_t ref = {.embedded = {.data = nullptr, .size = 0}, .node = nullptr, .is_hash = false};
  return ref;
}

/// Leaf node: terminates a path with a value.
/// RLP encoding: [hex_prefix(path, is_leaf=true), value]
typedef struct {
  nibbles_t path; // Remaining nibbles of key
  bytes_t value;  // Stored value
} mpt_leaf_t;

/// Extension node: shared path prefix optimization.
/// RLP encoding: [hex_prefix(path, is_leaf=false), child_ref]
typedef struct {
  nibbles_t path;   // Shared nibble prefix
  node_ref_t child; // Reference to child (must be branch)
} mpt_extension_t;

/// Branch node: 16-way branch point.
/// RLP encoding: [child0, ..., child15, value]
typedef struct {
  node_ref_t children[16]; // One slot per nibble (0-15)
  bytes_t value;           // Optional value if key terminates here
} mpt_branch_t;

/// Generic MPT node.
/// Fields ordered to minimize padding.
struct mpt_node {
  // Cached hash (invalidated when node is modified)
  hash_t cached_hash;
  union {
    mpt_leaf_t leaf;
    mpt_extension_t extension;
    mpt_branch_t branch;
  };
  mpt_node_type_t type;
  bool hash_valid;
};

/// Empty root hash constant (keccak256 of RLP-encoded empty string: 0x80).
/// = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
extern const hash_t MPT_EMPTY_ROOT;

/// Create an empty node.
[[nodiscard]] mpt_node_t mpt_node_empty(void);

/// Create a leaf node.
[[nodiscard]] mpt_node_t mpt_node_leaf(nibbles_t path, bytes_t value);

/// Create an extension node.
[[nodiscard]] mpt_node_t mpt_node_extension(nibbles_t path, node_ref_t child);

/// Create a branch node (all children null, no value).
[[nodiscard]] mpt_node_t mpt_node_branch(void);

/// RLP-encode a node.
/// @param node The node to encode
/// @param arena Arena for allocation
/// @return RLP-encoded bytes
[[nodiscard]] bytes_t mpt_node_encode(const mpt_node_t *node, div0_arena_t *arena);

/// Compute or return cached hash of a node.
/// @param node The node (hash_valid may be updated)
/// @param arena Arena for temporary allocations
/// @return keccak256 hash of RLP-encoded node
[[nodiscard]] hash_t mpt_node_hash(mpt_node_t *node, div0_arena_t *arena);

/// Compute node reference (embed if RLP < 32 bytes, else hash).
/// @param node The node
/// @param arena Arena for allocation
/// @return Node reference (embedded or hash)
[[nodiscard]] node_ref_t mpt_node_ref(mpt_node_t *node, div0_arena_t *arena);

/// Count non-null children in a branch node.
/// @param branch The branch node
/// @return Number of non-null children (0-16)
[[nodiscard]] size_t mpt_branch_child_count(const mpt_branch_t *branch);

/// Check if a node is a leaf.
[[nodiscard]] static inline bool mpt_node_is_leaf(const mpt_node_t *node) {
  return node->type == MPT_NODE_LEAF;
}

/// Check if a node is an extension.
[[nodiscard]] static inline bool mpt_node_is_extension(const mpt_node_t *node) {
  return node->type == MPT_NODE_EXTENSION;
}

/// Check if a node is a branch.
[[nodiscard]] static inline bool mpt_node_is_branch(const mpt_node_t *node) {
  return node->type == MPT_NODE_BRANCH;
}

/// Check if a node is empty.
[[nodiscard]] static inline bool mpt_node_is_empty(const mpt_node_t *node) {
  return node->type == MPT_NODE_EMPTY;
}

/// Invalidate cached hash (call when node is modified).
static inline void mpt_node_invalidate_hash(mpt_node_t *node) {
  node->hash_valid = false;
}

#endif // DIV0_TRIE_NODE_H
