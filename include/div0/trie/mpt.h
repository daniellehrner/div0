#ifndef DIV0_TRIE_MPT_H
#define DIV0_TRIE_MPT_H

#include "div0/mem/arena.h"
#include "div0/trie/node.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Forward declarations
typedef struct mpt_backend mpt_backend_t;
typedef struct mpt mpt_t;

// =============================================================================
// Backend Interface (vtable pattern)
// =============================================================================

/// Backend operations for node storage.
/// This abstraction allows different storage backends (memory, disk, etc.)
/// while keeping the core MPT logic unchanged.
typedef struct {
  /// Get the root node (nullptr if trie is empty).
  mpt_node_t *(*get_root)(mpt_backend_t *backend);

  /// Set the root node.
  void (*set_root)(mpt_backend_t *backend, mpt_node_t *root);

  /// Allocate a new node.
  mpt_node_t *(*alloc_node)(mpt_backend_t *backend);

  /// Get node by hash (for loading from persistent storage).
  /// In-memory backend may return nullptr (nodes are inline).
  mpt_node_t *(*get_node_by_hash)(const mpt_backend_t *backend, const hash_t *hash);

  /// Store node (for persistent storage).
  /// Returns the hash of the stored node.
  hash_t (*store_node)(mpt_backend_t *backend, mpt_node_t *node);

  /// Begin a batch operation (for atomic updates).
  void (*begin_batch)(mpt_backend_t *backend);

  /// Commit a batch operation.
  void (*commit_batch)(mpt_backend_t *backend);

  /// Rollback a batch operation.
  void (*rollback_batch)(mpt_backend_t *backend);

  /// Clear all nodes (reset to empty trie).
  void (*clear)(mpt_backend_t *backend);

  /// Destroy the backend and free resources.
  void (*destroy)(mpt_backend_t *backend);
} mpt_backend_vtable_t;

/// Base structure for all backends.
/// Concrete backends embed this as first member.
struct mpt_backend {
  const mpt_backend_vtable_t *vtable;
};

// =============================================================================
// MPT Handle
// =============================================================================

/// Merkle Patricia Trie handle.
/// Provides the high-level API for trie operations.
struct mpt {
  mpt_backend_t *backend;   // Storage backend
  div0_arena_t *work_arena; // Working arena for temporary allocations
};

// =============================================================================
// MPT Operations
// =============================================================================

/// Initialize an MPT with a backend.
/// @param mpt MPT to initialize
/// @param backend Storage backend to use
/// @param work_arena Arena for temporary allocations during operations
void mpt_init(mpt_t *mpt, mpt_backend_t *backend, div0_arena_t *work_arena);

/// Insert or update a key-value pair.
/// @param mpt The trie
/// @param key Key bytes
/// @param key_len Length of key
/// @param value Value bytes
/// @param value_len Length of value
/// @return true on success, false on error
bool mpt_insert(mpt_t *mpt, const uint8_t *key, size_t key_len, const uint8_t *value,
                size_t value_len);

/// Get value for a key.
/// @param mpt The trie
/// @param key Key bytes
/// @param key_len Length of key
/// @return Value bytes (empty if not found), arena-backed
[[nodiscard]] bytes_t mpt_get(const mpt_t *mpt, const uint8_t *key, size_t key_len);

/// Check if a key exists.
/// @param mpt The trie
/// @param key Key bytes
/// @param key_len Length of key
/// @return true if key exists
[[nodiscard]] bool mpt_contains(const mpt_t *mpt, const uint8_t *key, size_t key_len);

/// Delete a key.
/// @param mpt The trie
/// @param key Key bytes
/// @param key_len Length of key
/// @return true if key was deleted, false if not found
bool mpt_delete(mpt_t *mpt, const uint8_t *key, size_t key_len);

/// Get the root hash.
/// Computes incrementally if any nodes are dirty.
/// @param mpt The trie
/// @return Root hash (MPT_EMPTY_ROOT if empty)
[[nodiscard]] hash_t mpt_root_hash(const mpt_t *mpt);

/// Check if the trie is empty.
/// @param mpt The trie
/// @return true if empty
[[nodiscard]] bool mpt_is_empty(const mpt_t *mpt);

/// Clear all entries.
/// @param mpt The trie
void mpt_clear(mpt_t *mpt);

/// Destroy the MPT (also destroys the backend).
/// @param mpt The trie
void mpt_destroy(mpt_t *mpt);

// =============================================================================
// Backend Implementations (forward declarations)
// =============================================================================

/// Create an in-memory backend.
/// All nodes are stored in memory using the provided arena.
/// @param arena Arena for node allocations
/// @return New in-memory backend
[[nodiscard]] mpt_backend_t *mpt_memory_backend_create(div0_arena_t *arena);

#endif // DIV0_TRIE_MPT_H
