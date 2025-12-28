#ifndef DIV0_EVM_MEMORY_POOL_H
#define DIV0_EVM_MEMORY_POOL_H

#include "div0/evm/memory.h"
#include "div0/mem/arena.h"

#include <assert.h>
#include <stddef.h>

/// Maximum call depth (matches EVM spec).
#define EVM_MAX_CALL_DEPTH 1024

/// Pool of EVM memory buffers for nested calls.
/// Pre-allocates memory structures to avoid allocation during execution.
typedef struct {
  evm_memory_t memories[EVM_MAX_CALL_DEPTH];
  size_t depth;
  div0_arena_t *arena;
} evm_memory_pool_t;

/// Initializes the memory pool with an arena allocator.
static inline void evm_memory_pool_init(evm_memory_pool_t *pool, div0_arena_t *arena) {
  pool->depth = 0;
  pool->arena = arena;
  // Note: memory structs are initialized lazily when borrowed
}

/// Borrows a memory buffer from the pool.
/// @param pool The memory pool
/// @return Pointer to a fresh memory buffer, or nullptr if max depth exceeded
static inline evm_memory_t *evm_memory_pool_borrow(evm_memory_pool_t *pool) {
  assert(pool->depth < EVM_MAX_CALL_DEPTH);
  if (pool->depth >= EVM_MAX_CALL_DEPTH) {
    return nullptr;
  }

  evm_memory_t *mem = &pool->memories[pool->depth++];
  evm_memory_init(mem, pool->arena);
  return mem;
}

/// Returns a memory buffer to the pool.
/// @param pool The memory pool
static inline void evm_memory_pool_return(evm_memory_pool_t *pool) {
  assert(pool->depth > 0);
  if (pool->depth > 0) {
    --pool->depth;
    // Reset the returned memory (arena handles actual deallocation)
    evm_memory_reset(&pool->memories[pool->depth]);
  }
}

/// Returns current pool depth.
static inline size_t evm_memory_pool_depth(const evm_memory_pool_t *pool) {
  return pool->depth;
}

#endif // DIV0_EVM_MEMORY_POOL_H
