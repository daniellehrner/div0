#include "div0/trie/node.h"

#include "div0/crypto/keccak256.h"
#include "div0/rlp/encode.h"
#include "div0/trie/hex_prefix.h"

#include <string.h>

// Empty root hash: keccak256(0x80) where 0x80 is RLP of empty string
// Pre-computed value
const hash_t MPT_EMPTY_ROOT = {.bytes = {0x56, 0xe8, 0x1f, 0x17, 0x1b, 0xcc, 0x55, 0xa6,
                                         0xff, 0x83, 0x45, 0xe6, 0x92, 0xc0, 0xf8, 0x6e,
                                         0x5b, 0x48, 0xe0, 0x1b, 0x99, 0x6c, 0xad, 0xc0,
                                         0x01, 0x62, 0x2f, 0xb5, 0xe3, 0x63, 0xb4, 0x21}};

mpt_node_t mpt_node_empty(void) {
  mpt_node_t node = {
      .type = MPT_NODE_EMPTY,
      .hash_valid = true,
  };
  node.cached_hash = MPT_EMPTY_ROOT;
  return node;
}

mpt_node_t mpt_node_leaf(nibbles_t path, bytes_t value) {
  mpt_node_t node = {
      .type = MPT_NODE_LEAF,
      .leaf =
          {
              .path = path,
              .value = value,
          },
      .hash_valid = false,
  };
  return node;
}

mpt_node_t mpt_node_extension(nibbles_t path, node_ref_t child) {
  mpt_node_t node = {
      .type = MPT_NODE_EXTENSION,
      .extension =
          {
              .path = path,
              .child = child,
          },
      .hash_valid = false,
  };
  return node;
}

mpt_node_t mpt_node_branch(void) {
  mpt_node_t node = {
      .type = MPT_NODE_BRANCH,
      .branch = {.value = {.data = nullptr, .size = 0}},
      .hash_valid = false,
  };
  // Initialize all children to null
  for (int i = 0; i < 16; i++) {
    node.branch.children[i] = node_ref_null();
  }
  return node;
}

/// Helper: Encode a node reference for RLP.
/// If hash: encode the 32-byte hash as bytes
/// If embedded: return the already-encoded bytes directly (no re-encoding)
static bytes_t encode_node_ref(const node_ref_t *ref, div0_arena_t *arena) {
  if (node_ref_is_null(ref)) {
    // Empty reference: encode as empty string (0x80)
    return rlp_encode_bytes(arena, nullptr, 0);
  }

  if (ref->is_hash) {
    // Hash: encode 32-byte hash as bytes
    return rlp_encode_bytes(arena, ref->hash.bytes, 32);
  }

  // Embedded: already RLP-encoded, just copy
  bytes_t result;
  bytes_init_arena(&result, arena);
  bytes_from_data(&result, ref->embedded.data, ref->embedded.size);
  return result;
}

bytes_t mpt_node_encode(const mpt_node_t *node, div0_arena_t *arena) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  switch (node->type) {
  case MPT_NODE_EMPTY: {
    // Empty node: RLP empty string (0x80)
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, 0x80);
    return result;
  }

  case MPT_NODE_LEAF: {
    // Leaf: [hex_prefix(path, is_leaf=true), value]
    bytes_t hp_path = hex_prefix_encode(&node->leaf.path, true, arena);
    bytes_t rlp_path = rlp_encode_bytes(arena, hp_path.data, hp_path.size);
    bytes_t rlp_value = rlp_encode_bytes(arena, node->leaf.value.data, node->leaf.value.size);

    // Estimate size and build list
    size_t est_size = 9 + rlp_path.size + rlp_value.size;
    bytes_reserve(&result, est_size);

    rlp_list_builder_t builder;
    rlp_list_start(&builder, &result);
    rlp_list_append(&result, &rlp_path);
    rlp_list_append(&result, &rlp_value);
    rlp_list_end(&builder);
    return result;
  }

  case MPT_NODE_EXTENSION: {
    // Extension: [hex_prefix(path, is_leaf=false), child_ref]
    bytes_t hp_path = hex_prefix_encode(&node->extension.path, false, arena);
    bytes_t rlp_path = rlp_encode_bytes(arena, hp_path.data, hp_path.size);
    bytes_t rlp_child = encode_node_ref(&node->extension.child, arena);

    size_t est_size = 9 + rlp_path.size + rlp_child.size;
    bytes_reserve(&result, est_size);

    rlp_list_builder_t builder;
    rlp_list_start(&builder, &result);
    rlp_list_append(&result, &rlp_path);
    rlp_list_append(&result, &rlp_child);
    rlp_list_end(&builder);
    return result;
  }

  case MPT_NODE_BRANCH: {
    // Branch: [child0, ..., child15, value]
    // Pre-encode all children
    bytes_t child_encodings[16];
    size_t total_size = 9; // List header reserve

    for (int i = 0; i < 16; i++) {
      child_encodings[i] = encode_node_ref(&node->branch.children[i], arena);
      total_size += child_encodings[i].size;
    }

    bytes_t rlp_value = rlp_encode_bytes(arena, node->branch.value.data, node->branch.value.size);
    total_size += rlp_value.size;

    bytes_reserve(&result, total_size);

    rlp_list_builder_t builder;
    rlp_list_start(&builder, &result);

    for (int i = 0; i < 16; i++) {
      rlp_list_append(&result, &child_encodings[i]);
    }
    rlp_list_append(&result, &rlp_value);

    rlp_list_end(&builder);
    return result;
  }
  }

  // Should not reach here
  return result;
}

hash_t mpt_node_hash(mpt_node_t *node, div0_arena_t *arena) {
  // Return cached hash if valid
  if (node->hash_valid) {
    return node->cached_hash;
  }

  // Special case: empty node has pre-computed hash
  if (node->type == MPT_NODE_EMPTY) {
    node->cached_hash = MPT_EMPTY_ROOT;
    node->hash_valid = true;
    return node->cached_hash;
  }

  // Encode and hash
  bytes_t encoded = mpt_node_encode(node, arena);
  node->cached_hash = keccak256(encoded.data, encoded.size);
  node->hash_valid = true;

  return node->cached_hash;
}

node_ref_t mpt_node_ref(mpt_node_t *node, div0_arena_t *arena) {
  node_ref_t ref;

  // Empty nodes produce empty reference
  if (node->type == MPT_NODE_EMPTY) {
    return node_ref_null();
  }

  bytes_t encoded = mpt_node_encode(node, arena);

  if (encoded.size < 32) {
    // Small enough to embed directly
    ref.is_hash = false;
    ref.embedded = encoded;
  } else {
    // Too large, store as hash
    ref.is_hash = true;
    ref.hash = keccak256(encoded.data, encoded.size);
  }

  // Set node pointer for in-memory traversal
  ref.node = node;

  return ref;
}

size_t mpt_branch_child_count(const mpt_branch_t *branch) {
  size_t count = 0;
  for (int i = 0; i < 16; i++) {
    if (!node_ref_is_null(&branch->children[i])) {
      count++;
    }
  }
  return count;
}
