#include "div0/trie/mpt.h"

#include "div0/trie/nibbles.h"

#include <string.h>

// =============================================================================
// Empty Value Sentinel
// =============================================================================

/// Static sentinel byte used to mark "empty value exists" (data != nullptr, size == 0).
/// This avoids allocating 1 byte per empty value.
static uint8_t empty_value_sentinel = 0;

// =============================================================================
// MPT Initialization
// =============================================================================

void mpt_init(mpt_t *mpt, mpt_backend_t *backend, div0_arena_t *work_arena) {
  mpt->backend = backend;
  mpt->work_arena = work_arena;
}

// =============================================================================
// Helper Functions
// =============================================================================

/// Find the position where two nibble sequences diverge.
static size_t find_divergence(const nibbles_t *path, const nibbles_t *key, size_t offset) {
  size_t i = 0;
  while (i < path->len && (offset + i) < key->len) {
    if (path->data[i] != key->data[offset + i]) {
      break;
    }
    i++;
  }
  return i;
}

/// Create a value copy in the backend's arena.
/// For empty values (value_len == 0), uses a static sentinel to distinguish
/// from "no value" (which has data == nullptr).
static bytes_t copy_value(mpt_backend_t *backend, const uint8_t *value, size_t value_len,
                          div0_arena_t *arena) {
  (void)backend; // May be used for backend-specific allocation
  bytes_t result;
  bytes_init_arena(&result, arena);
  if (value_len > 0) {
    bytes_from_data(&result, value, value_len);
  } else {
    // Empty value: use sentinel to mark "value exists but is empty"
    // This distinguishes from "no value" which has data == nullptr
    result.data = &empty_value_sentinel;
    result.size = 0;
  }
  return result;
}

// =============================================================================
// Delete Result Type
// =============================================================================

/// Result of a delete operation.
typedef enum {
  DELETE_NOT_FOUND, // Key was not found
  DELETE_UPDATED,   // Node was updated (still exists)
  DELETE_REMOVED    // Node was completely removed
} delete_result_t;

// =============================================================================
// Recursive Insert Implementation
// =============================================================================

/// Recursively insert a key-value pair into a subtrie.
/// Returns the new/modified node.
static mpt_node_t *insert_recursive(mpt_backend_t *backend, mpt_node_t *node, const nibbles_t *key,
                                    size_t offset, const uint8_t *value, size_t value_len,
                                    div0_arena_t *arena) {
  // Handle empty/null node - create a new leaf
  if (node == nullptr || node->type == MPT_NODE_EMPTY) {
    mpt_node_t *new_node = backend->vtable->alloc_node(backend);
    if (new_node == nullptr) {
      return nullptr;
    }
    if (offset >= key->len) {
      *new_node = mpt_node_leaf(NIBBLES_EMPTY, copy_value(backend, value, value_len, arena));
    } else {
      nibbles_t remaining = nibbles_slice(key, offset, SIZE_MAX, arena);
      *new_node = mpt_node_leaf(remaining, copy_value(backend, value, value_len, arena));
    }
    return new_node;
  }

  // If we've consumed the entire key but have an existing node, handle by type
  if (offset >= key->len) {
    switch (node->type) {
    case MPT_NODE_LEAF: {
      if (node->leaf.path.len == 0) {
        // Exact match with empty path - update value
        node->leaf.value = copy_value(backend, value, value_len, arena);
        mpt_node_invalidate_hash(node);
        return node;
      }
      // Need branch: new value at branch, old leaf as child
      mpt_node_t *branch = backend->vtable->alloc_node(backend);
      if (branch == nullptr) {
        return nullptr;
      }
      *branch = mpt_node_branch();
      branch->branch.value = copy_value(backend, value, value_len, arena);

      uint8_t old_nibble = node->leaf.path.data[0];
      mpt_node_t *old_leaf = backend->vtable->alloc_node(backend);
      if (old_leaf == nullptr) {
        return nullptr;
      }
      nibbles_t old_remaining = nibbles_slice(&node->leaf.path, 1, SIZE_MAX, arena);
      *old_leaf = mpt_node_leaf(old_remaining, node->leaf.value);
      branch->branch.children[old_nibble] = mpt_node_ref(old_leaf, arena);

      return branch;
    }
    case MPT_NODE_BRANCH:
      // Update value at branch point
      node->branch.value = copy_value(backend, value, value_len, arena);
      mpt_node_invalidate_hash(node);
      return node;
    case MPT_NODE_EXTENSION: {
      // Need to split extension
      mpt_node_t *branch = backend->vtable->alloc_node(backend);
      if (branch == nullptr) {
        return nullptr;
      }
      *branch = mpt_node_branch();
      branch->branch.value = copy_value(backend, value, value_len, arena);

      uint8_t ext_nibble = node->extension.path.data[0];
      if (node->extension.path.len > 1) {
        // Create shortened extension
        mpt_node_t *new_ext = backend->vtable->alloc_node(backend);
        if (new_ext == nullptr) {
          return nullptr;
        }
        nibbles_t new_ext_path = nibbles_slice(&node->extension.path, 1, SIZE_MAX, arena);
        *new_ext = mpt_node_extension(new_ext_path, node->extension.child);
        branch->branch.children[ext_nibble] = mpt_node_ref(new_ext, arena);
      } else {
        // Extension leads directly to child
        branch->branch.children[ext_nibble] = node->extension.child;
      }
      return branch;
    }
    default:
      break;
    }
  }

  // Handle different node types
  switch (node->type) {
  case MPT_NODE_LEAF: {
    // Need to split the leaf or replace it
    size_t match_len = find_divergence(&node->leaf.path, key, offset);

    if (match_len == node->leaf.path.len && (offset + match_len) == key->len) {
      // Exact match - update value
      node->leaf.value = copy_value(backend, value, value_len, arena);
      mpt_node_invalidate_hash(node);
      return node;
    }

    // Need to create a branch
    mpt_node_t *branch = backend->vtable->alloc_node(backend);
    if (branch == nullptr) {
      return nullptr;
    }
    *branch = mpt_node_branch();

    if (match_len > 0) {
      // Common prefix - create extension pointing to branch
      mpt_node_t *ext = backend->vtable->alloc_node(backend);
      if (ext == nullptr) {
        return nullptr;
      }
      nibbles_t common_path = nibbles_slice(&node->leaf.path, 0, match_len, arena);

      // Build the branch first
      if (match_len < node->leaf.path.len) {
        // Existing leaf becomes child of branch
        uint8_t old_nibble = node->leaf.path.data[match_len];
        mpt_node_t *old_leaf = backend->vtable->alloc_node(backend);
        if (old_leaf == nullptr) {
          return nullptr;
        }
        nibbles_t old_remaining = nibbles_slice(&node->leaf.path, match_len + 1, SIZE_MAX, arena);
        *old_leaf = mpt_node_leaf(old_remaining, node->leaf.value);
        branch->branch.children[old_nibble] = mpt_node_ref(old_leaf, arena);
      } else {
        // Existing leaf terminates at branch
        branch->branch.value = node->leaf.value;
      }

      if ((offset + match_len) < key->len) {
        // New key continues - add as child
        uint8_t new_nibble = key->data[offset + match_len];
        nibbles_t new_remaining = nibbles_slice(key, offset + match_len + 1, SIZE_MAX, arena);
        mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
        if (new_leaf == nullptr) {
          return nullptr;
        }
        *new_leaf = mpt_node_leaf(new_remaining, copy_value(backend, value, value_len, arena));
        branch->branch.children[new_nibble] = mpt_node_ref(new_leaf, arena);
      } else {
        // New key terminates at branch
        branch->branch.value = copy_value(backend, value, value_len, arena);
      }

      *ext = mpt_node_extension(common_path, mpt_node_ref(branch, arena));
      return ext;
    }

    // No common prefix - branch at current position
    if (node->leaf.path.len > 0) {
      // Existing leaf has remaining path - add as child
      uint8_t old_nibble = node->leaf.path.data[0];
      mpt_node_t *old_leaf = backend->vtable->alloc_node(backend);
      if (old_leaf == nullptr) {
        return nullptr;
      }
      nibbles_t old_remaining = nibbles_slice(&node->leaf.path, 1, SIZE_MAX, arena);
      *old_leaf = mpt_node_leaf(old_remaining, node->leaf.value);
      branch->branch.children[old_nibble] = mpt_node_ref(old_leaf, arena);
    } else {
      // Existing leaf has empty path - its value goes at branch
      branch->branch.value = node->leaf.value;
    }

    uint8_t new_nibble = key->data[offset];
    nibbles_t new_remaining = nibbles_slice(key, offset + 1, SIZE_MAX, arena);
    mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
    if (new_leaf == nullptr) {
      return nullptr;
    }
    *new_leaf = mpt_node_leaf(new_remaining, copy_value(backend, value, value_len, arena));
    branch->branch.children[new_nibble] = mpt_node_ref(new_leaf, arena);

    return branch;
  }

  case MPT_NODE_EXTENSION: {
    size_t match_len = find_divergence(&node->extension.path, key, offset);

    if (match_len == node->extension.path.len) {
      // Full path match - descend into child
      if (node->extension.child.node != nullptr) {
        // Use node pointer for in-memory traversal
        mpt_node_t *child = insert_recursive(backend, node->extension.child.node, key,
                                             offset + match_len, value, value_len, arena);
        if (child == nullptr) {
          return nullptr;
        }
        node->extension.child = mpt_node_ref(child, arena);
        mpt_node_invalidate_hash(node);
        return node;
      }
      // Fallback - shouldn't happen for in-memory backends
      mpt_node_invalidate_hash(node);
      return node;
    }

    // Partial match - need to split extension
    mpt_node_t *branch = backend->vtable->alloc_node(backend);
    if (branch == nullptr) {
      return nullptr;
    }
    *branch = mpt_node_branch();

    if (match_len < node->extension.path.len) {
      // Existing extension continues after branch
      uint8_t ext_nibble = node->extension.path.data[match_len];
      if (match_len + 1 < node->extension.path.len) {
        // Create shortened extension
        mpt_node_t *new_ext = backend->vtable->alloc_node(backend);
        if (new_ext == nullptr) {
          return nullptr;
        }
        nibbles_t new_ext_path =
            nibbles_slice(&node->extension.path, match_len + 1, SIZE_MAX, arena);
        *new_ext = mpt_node_extension(new_ext_path, node->extension.child);
        branch->branch.children[ext_nibble] = mpt_node_ref(new_ext, arena);
      } else {
        // Extension leads directly to child
        branch->branch.children[ext_nibble] = node->extension.child;
      }
    }

    // Add new key
    if ((offset + match_len) < key->len) {
      uint8_t new_nibble = key->data[offset + match_len];
      nibbles_t new_remaining = nibbles_slice(key, offset + match_len + 1, SIZE_MAX, arena);
      mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
      if (new_leaf == nullptr) {
        return nullptr;
      }
      *new_leaf = mpt_node_leaf(new_remaining, copy_value(backend, value, value_len, arena));
      branch->branch.children[new_nibble] = mpt_node_ref(new_leaf, arena);
    } else {
      branch->branch.value = copy_value(backend, value, value_len, arena);
    }

    if (match_len > 0) {
      // Create new extension for common prefix
      mpt_node_t *new_ext = backend->vtable->alloc_node(backend);
      if (new_ext == nullptr) {
        return nullptr;
      }
      nibbles_t common_path = nibbles_slice(&node->extension.path, 0, match_len, arena);
      *new_ext = mpt_node_extension(common_path, mpt_node_ref(branch, arena));
      return new_ext;
    }

    return branch;
  }

  case MPT_NODE_BRANCH: {
    if (offset >= key->len) {
      // Key terminates at this branch
      node->branch.value = copy_value(backend, value, value_len, arena);
      mpt_node_invalidate_hash(node);
      return node;
    }

    // Continue down the appropriate child
    uint8_t nibble = key->data[offset];

    if (node_ref_is_null(&node->branch.children[nibble])) {
      // No child - create new leaf
      mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
      if (new_leaf == nullptr) {
        return nullptr;
      }
      nibbles_t remaining = nibbles_slice(key, offset + 1, SIZE_MAX, arena);
      *new_leaf = mpt_node_leaf(remaining, copy_value(backend, value, value_len, arena));
      node->branch.children[nibble] = mpt_node_ref(new_leaf, arena);
    } else if (node->branch.children[nibble].node != nullptr) {
      // Existing child - descend recursively using node pointer
      mpt_node_t *child = insert_recursive(backend, node->branch.children[nibble].node, key,
                                           offset + 1, value, value_len, arena);
      if (child == nullptr) {
        return nullptr;
      }
      node->branch.children[nibble] = mpt_node_ref(child, arena);
    }

    mpt_node_invalidate_hash(node);
    return node;
  }

  default:
    break;
  }

  return node;
}

// =============================================================================
// Recursive Delete Implementation
// =============================================================================

/// Find the single non-null child in a branch node.
/// Returns the nibble index (0-15) or -1 if not exactly one child.
static int find_single_child(const mpt_branch_t *branch) {
  int found = -1;
  for (int i = 0; i < 16; i++) {
    if (!node_ref_is_null(&branch->children[i])) {
      if (found >= 0) {
        return -1; // More than one child
      }
      found = i;
    }
  }
  return found;
}

/// Collapse a branch node after one of its children was removed.
/// If the branch has only one remaining child and no value, convert to extension.
/// If the branch has only a value and no children, convert to leaf.
static mpt_node_t *collapse_branch(mpt_backend_t *backend, mpt_node_t *branch,
                                   div0_arena_t *arena) {
  bool has_value = branch->branch.value.data != nullptr;
  int single_child = find_single_child(&branch->branch);

  // Check for zero children first
  if (single_child < 0) {
    size_t child_count = mpt_branch_child_count(&branch->branch);
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (child_count == 0 && has_value) {
      // No children, only value - convert to leaf with empty path
      mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
      if (new_leaf == nullptr) {
        return branch;
      }
      *new_leaf = mpt_node_leaf(NIBBLES_EMPTY, branch->branch.value);
      return new_leaf;
    }
    // Multiple children - keep as branch
    mpt_node_invalidate_hash(branch);
    return branch;
  }

  if (has_value) {
    // Has value and one child - keep as branch
    mpt_node_invalidate_hash(branch);
    return branch;
  }

  // Only one child, no value - collapse
  mpt_node_t *child = branch->branch.children[single_child].node;
  if (child == nullptr) {
    // Shouldn't happen for in-memory, but be safe
    mpt_node_invalidate_hash(branch);
    return branch;
  }

  uint8_t nibble = (uint8_t)single_child;

  if (child->type == MPT_NODE_LEAF) {
    // Merge: branch -> leaf becomes leaf with extended path
    nibbles_t new_path = nibbles_concat3(&NIBBLES_EMPTY, nibble, &child->leaf.path, arena);
    mpt_node_t *new_leaf = backend->vtable->alloc_node(backend);
    if (new_leaf == nullptr) {
      return branch;
    }
    *new_leaf = mpt_node_leaf(new_path, child->leaf.value);
    return new_leaf;
  }

  if (child->type == MPT_NODE_EXTENSION) {
    // Merge: branch -> extension becomes extension with extended path
    nibbles_t new_path = nibbles_concat3(&NIBBLES_EMPTY, nibble, &child->extension.path, arena);
    mpt_node_t *new_ext = backend->vtable->alloc_node(backend);
    if (new_ext == nullptr) {
      return branch;
    }
    *new_ext = mpt_node_extension(new_path, child->extension.child);
    return new_ext;
  }

  // Child is a branch - create extension pointing to it
  nibbles_t path;
  path.len = 1;
  path.data = (uint8_t *)div0_arena_alloc(arena, 1);
  if (path.data == nullptr) {
    return branch;
  }
  path.data[0] = nibble;

  mpt_node_t *new_ext = backend->vtable->alloc_node(backend);
  if (new_ext == nullptr) {
    return branch;
  }
  *new_ext = mpt_node_extension(path, mpt_node_ref(child, arena));
  return new_ext;
}

/// Recursively delete a key from the trie.
/// @param out_node Output: the new/modified node (nullptr if removed)
/// @return DELETE_NOT_FOUND if key not found, DELETE_UPDATED if node updated, DELETE_REMOVED if
/// node removed
static delete_result_t delete_recursive(mpt_backend_t *backend, mpt_node_t *node,
                                        const nibbles_t *key, size_t offset, mpt_node_t **out_node,
                                        div0_arena_t *arena) {
  *out_node = node;

  if (node == nullptr || node->type == MPT_NODE_EMPTY) {
    return DELETE_NOT_FOUND;
  }

  switch (node->type) {
  case MPT_NODE_LEAF: {
    // Check if the remaining key matches the leaf path
    size_t remaining_len = key->len - offset;
    if (remaining_len != node->leaf.path.len) {
      return DELETE_NOT_FOUND;
    }
    for (size_t i = 0; i < node->leaf.path.len; i++) {
      if (node->leaf.path.data[i] != key->data[offset + i]) {
        return DELETE_NOT_FOUND;
      }
    }
    // Found it - remove this leaf
    *out_node = nullptr;
    return DELETE_REMOVED;
  }

  case MPT_NODE_EXTENSION: {
    // Check if the key prefix matches the extension path
    size_t match_len = find_divergence(&node->extension.path, key, offset);
    if (match_len != node->extension.path.len) {
      return DELETE_NOT_FOUND;
    }

    // Recurse into child
    mpt_node_t *new_child = nullptr;
    delete_result_t result = delete_recursive(backend, node->extension.child.node, key,
                                              offset + match_len, &new_child, arena);
    if (result == DELETE_NOT_FOUND) {
      return DELETE_NOT_FOUND;
    }

    if (result == DELETE_REMOVED) {
      // Child was removed - this extension should be removed too
      *out_node = nullptr;
      return DELETE_REMOVED;
    }

    // Child was updated - may need to merge
    if (new_child == nullptr) {
      *out_node = nullptr;
      return DELETE_REMOVED;
    }

    if (new_child->type == MPT_NODE_LEAF) {
      // Merge extension + leaf into new leaf
      nibbles_t new_path = nibbles_concat(&node->extension.path, &new_child->leaf.path, arena);
      mpt_node_t *merged = backend->vtable->alloc_node(backend);
      if (merged == nullptr) {
        return DELETE_NOT_FOUND;
      }
      *merged = mpt_node_leaf(new_path, new_child->leaf.value);
      *out_node = merged;
      return DELETE_UPDATED;
    }

    if (new_child->type == MPT_NODE_EXTENSION) {
      // Merge extension + extension into single extension
      nibbles_t new_path = nibbles_concat(&node->extension.path, &new_child->extension.path, arena);
      mpt_node_t *merged = backend->vtable->alloc_node(backend);
      if (merged == nullptr) {
        return DELETE_NOT_FOUND;
      }
      *merged = mpt_node_extension(new_path, new_child->extension.child);
      *out_node = merged;
      return DELETE_UPDATED;
    }

    // Child is a branch - just update reference
    node->extension.child = mpt_node_ref(new_child, arena);
    mpt_node_invalidate_hash(node);
    *out_node = node;
    return DELETE_UPDATED;
  }

  case MPT_NODE_BRANCH: {
    if (offset >= key->len) {
      // Key terminates at this branch - remove value
      if (node->branch.value.data == nullptr) {
        return DELETE_NOT_FOUND; // No value to remove
      }
      node->branch.value = (bytes_t){.data = nullptr, .size = 0};

      // Check if branch should collapse
      *out_node = collapse_branch(backend, node, arena);
      return DELETE_UPDATED;
    }

    uint8_t nibble = key->data[offset];
    if (node_ref_is_null(&node->branch.children[nibble])) {
      return DELETE_NOT_FOUND;
    }

    mpt_node_t *child = node->branch.children[nibble].node;
    if (child == nullptr) {
      return DELETE_NOT_FOUND; // Can't traverse (shouldn't happen for in-memory)
    }

    mpt_node_t *new_child = nullptr;
    delete_result_t result = delete_recursive(backend, child, key, offset + 1, &new_child, arena);
    if (result == DELETE_NOT_FOUND) {
      return DELETE_NOT_FOUND;
    }

    if (result == DELETE_REMOVED || new_child == nullptr) {
      // Child was removed
      node->branch.children[nibble] = node_ref_null();
    } else {
      // Child was updated
      node->branch.children[nibble] = mpt_node_ref(new_child, arena);
    }

    // Check if branch should collapse
    *out_node = collapse_branch(backend, node, arena);
    return DELETE_UPDATED;
  }

  default:
    return DELETE_NOT_FOUND;
  }
}

// =============================================================================
// Public API
// =============================================================================

bool mpt_insert(mpt_t *mpt, const uint8_t *key, size_t key_len, const uint8_t *value,
                size_t value_len) {
  if (mpt == nullptr || mpt->backend == nullptr) {
    return false;
  }

  nibbles_t key_nibbles = nibbles_from_bytes(key, key_len, mpt->work_arena);

  mpt_node_t *root = mpt->backend->vtable->get_root(mpt->backend);
  mpt_node_t *new_root =
      insert_recursive(mpt->backend, root, &key_nibbles, 0, value, value_len, mpt->work_arena);

  if (new_root == nullptr) {
    return false; // Allocation failed
  }

  mpt->backend->vtable->set_root(mpt->backend, new_root);
  return true;
}

bytes_t mpt_get(mpt_t *mpt, const uint8_t *key, size_t key_len) {
  bytes_t empty = {.data = nullptr, .size = 0};
  if (mpt == nullptr || mpt->backend == nullptr) {
    return empty;
  }

  nibbles_t key_nibbles = nibbles_from_bytes(key, key_len, mpt->work_arena);
  mpt_node_t *node = mpt->backend->vtable->get_root(mpt->backend);
  size_t offset = 0;

  while (node != nullptr && node->type != MPT_NODE_EMPTY) {
    switch (node->type) {
    case MPT_NODE_LEAF: {
      // Check if remaining key matches
      if (key_nibbles.len - offset == node->leaf.path.len) {
        bool match = true;
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        for (size_t i = 0; i < node->leaf.path.len && match; i++) {
          if (node->leaf.path.data[i] != key_nibbles.data[offset + i]) {
            match = false;
          }
        }
        if (match) {
          return node->leaf.value;
        }
      }
      return empty;
    }

    case MPT_NODE_EXTENSION: {
      // Check if path matches
      size_t match_len = find_divergence(&node->extension.path, &key_nibbles, offset);
      if (match_len != node->extension.path.len) {
        return empty;
      }
      offset += match_len;
      // Use node pointer for in-memory traversal
      if (node->extension.child.node != nullptr) {
        node = node->extension.child.node;
        continue;
      }
      return empty;
    }

    case MPT_NODE_BRANCH: {
      if (offset >= key_nibbles.len) {
        return node->branch.value;
      }
      uint8_t nibble = key_nibbles.data[offset];
      if (node_ref_is_null(&node->branch.children[nibble])) {
        return empty;
      }
      offset++;
      // Use node pointer for in-memory traversal
      if (node->branch.children[nibble].node != nullptr) {
        node = node->branch.children[nibble].node;
        continue;
      }
      return empty;
    }

    default:
      return empty;
    }
  }

  return empty;
}

bool mpt_contains(mpt_t *mpt, const uint8_t *key, size_t key_len) {
  if (mpt == nullptr || mpt->backend == nullptr) {
    return false;
  }

  nibbles_t key_nibbles = nibbles_from_bytes(key, key_len, mpt->work_arena);
  mpt_node_t *node = mpt->backend->vtable->get_root(mpt->backend);
  size_t offset = 0;

  while (node != nullptr && node->type != MPT_NODE_EMPTY) {
    switch (node->type) {
    case MPT_NODE_LEAF: {
      // Check if remaining key matches the leaf path
      size_t remaining = key_nibbles.len - offset;
      if (remaining != node->leaf.path.len) {
        return false;
      }
      for (size_t i = 0; i < node->leaf.path.len; i++) {
        if (node->leaf.path.data[i] != key_nibbles.data[offset + i]) {
          return false;
        }
      }
      return true; // Found - even if value is empty
    }

    case MPT_NODE_EXTENSION: {
      size_t match_len = find_divergence(&node->extension.path, &key_nibbles, offset);
      if (match_len != node->extension.path.len) {
        return false;
      }
      offset += match_len;
      if (node->extension.child.node != nullptr) {
        node = node->extension.child.node;
        continue;
      }
      return false;
    }

    case MPT_NODE_BRANCH: {
      if (offset >= key_nibbles.len) {
        // Key terminates at this branch - check if value exists
        // data != nullptr means a value was explicitly set (even if empty)
        return node->branch.value.data != nullptr;
      }
      uint8_t nibble = key_nibbles.data[offset];
      if (node_ref_is_null(&node->branch.children[nibble])) {
        return false;
      }
      offset++;
      if (node->branch.children[nibble].node != nullptr) {
        node = node->branch.children[nibble].node;
        continue;
      }
      return false;
    }

    default:
      return false;
    }
  }

  return false;
}

bool mpt_delete(mpt_t *mpt, const uint8_t *key, size_t key_len) {
  if (mpt == nullptr || mpt->backend == nullptr) {
    return false;
  }

  mpt_node_t *root = mpt->backend->vtable->get_root(mpt->backend);
  if (root == nullptr || root->type == MPT_NODE_EMPTY) {
    return false; // Empty trie, nothing to delete
  }

  nibbles_t key_nibbles = nibbles_from_bytes(key, key_len, mpt->work_arena);

  mpt_node_t *new_root = nullptr;
  delete_result_t result =
      delete_recursive(mpt->backend, root, &key_nibbles, 0, &new_root, mpt->work_arena);

  if (result == DELETE_NOT_FOUND) {
    return false;
  }

  mpt->backend->vtable->set_root(mpt->backend, new_root);
  return true;
}

hash_t mpt_root_hash(mpt_t *mpt) {
  if (mpt == nullptr || mpt->backend == nullptr) {
    return MPT_EMPTY_ROOT;
  }

  mpt_node_t *root = mpt->backend->vtable->get_root(mpt->backend);
  if (root == nullptr || root->type == MPT_NODE_EMPTY) {
    return MPT_EMPTY_ROOT;
  }

  return mpt_node_hash(root, mpt->work_arena);
}

bool mpt_is_empty(mpt_t *mpt) {
  if (mpt == nullptr || mpt->backend == nullptr) {
    return true;
  }

  mpt_node_t *root = mpt->backend->vtable->get_root(mpt->backend);
  // NOLINTNEXTLINE(readability-implicit-bool-conversion)
  return root == nullptr || root->type == MPT_NODE_EMPTY;
}

void mpt_clear(mpt_t *mpt) {
  if (mpt != nullptr && mpt->backend != nullptr) {
    mpt->backend->vtable->clear(mpt->backend);
  }
}

void mpt_destroy(mpt_t *mpt) {
  if (mpt != nullptr && mpt->backend != nullptr) {
    mpt->backend->vtable->destroy(mpt->backend);
    mpt->backend = nullptr;
  }
}
