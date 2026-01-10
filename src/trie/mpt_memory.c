#include "div0/trie/mpt.h"

#include <stdalign.h>

// =============================================================================
// In-Memory Backend Implementation
// =============================================================================

/// In-memory backend structure.
/// Stores nodes directly in an arena with direct pointer access.
typedef struct {
  mpt_backend_t base;  // Must be first for vtable access
  mpt_node_t *root;    // Root node pointer
  div0_arena_t *arena; // Arena for node allocations
} mpt_memory_backend_t;

// =============================================================================
// Vtable Function Implementations
// =============================================================================

static mpt_node_t *memory_get_root(mpt_backend_t *const backend) {
  const auto mem = (mpt_memory_backend_t *)backend;
  return mem->root;
}

static void memory_set_root(mpt_backend_t *const backend, mpt_node_t *const root) {
  const auto mem = (mpt_memory_backend_t *)backend;
  mem->root = root;
}

static mpt_node_t *memory_alloc_node(mpt_backend_t *const backend) {
  const auto mem = (mpt_memory_backend_t *)backend;
  // mpt_node_t contains hash_t which requires 32-byte alignment
  mpt_node_t *const node =
      div0_arena_alloc_aligned(mem->arena, sizeof(mpt_node_t), alignof(mpt_node_t));
  if (node != nullptr) {
    *node = mpt_node_empty();
  }
  return node;
}

static mpt_node_t *memory_get_node_by_hash(const mpt_backend_t *const backend,
                                           const hash_t *const hash) {
  // In-memory backend doesn't support hash-based lookup
  // Nodes are accessed directly via pointers
  (void)backend;
  (void)hash;
  return nullptr;
}

static hash_t memory_store_node(mpt_backend_t *const backend, mpt_node_t *const node) {
  // In-memory backend doesn't need explicit storage
  // Just compute and return the hash
  const auto mem = (mpt_memory_backend_t *)backend;
  return mpt_node_hash(node, mem->arena);
}

// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - vtable semantic: begin_batch modifies state
static void memory_begin_batch(mpt_backend_t *const backend) {
  // In-memory backend doesn't need batch operations
  (void)backend;
}

// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - vtable semantic: commit_batch modifies state
static void memory_commit_batch(mpt_backend_t *const backend) {
  // In-memory backend doesn't need batch operations
  (void)backend;
}

// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - vtable semantic: rollback_batch modifies state
static void memory_rollback_batch(mpt_backend_t *const backend) {
  // In-memory backend doesn't support rollback
  // Would need to copy the entire trie structure
  (void)backend;
}

static void memory_clear(mpt_backend_t *const backend) {
  const auto mem = (mpt_memory_backend_t *)backend;
  // Clear root only - don't reset arena since it may be shared.
  // Orphaned nodes will be reclaimed when arena is reset/destroyed.
  mem->root = nullptr;
}

static void memory_destroy(mpt_backend_t *const backend) {
  const auto mem = (mpt_memory_backend_t *)backend;
  // The backend itself is allocated in the arena, so just reset
  div0_arena_reset(mem->arena);
  // Note: The arena is not destroyed here - it's owned by the caller
}

// =============================================================================
// Vtable Definition
// =============================================================================

static const mpt_backend_vtable_t MEMORY_VTABLE = {
    .get_root = memory_get_root,
    .set_root = memory_set_root,
    .alloc_node = memory_alloc_node,
    .get_node_by_hash = memory_get_node_by_hash,
    .store_node = memory_store_node,
    .begin_batch = memory_begin_batch,
    .commit_batch = memory_commit_batch,
    .rollback_batch = memory_rollback_batch,
    .clear = memory_clear,
    .destroy = memory_destroy,
};

// =============================================================================
// Public API
// =============================================================================

mpt_backend_t *mpt_memory_backend_create(div0_arena_t *const arena) {
  if (arena == nullptr) {
    return nullptr;
  }

  mpt_memory_backend_t *const backend = div0_arena_alloc(arena, sizeof(mpt_memory_backend_t));
  if (backend == nullptr) {
    return nullptr;
  }

  backend->base.vtable = &MEMORY_VTABLE;
  backend->root = nullptr;
  backend->arena = arena;

  return &backend->base;
}
