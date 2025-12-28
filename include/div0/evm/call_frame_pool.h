#ifndef DIV0_EVM_CALL_FRAME_POOL_H
#define DIV0_EVM_CALL_FRAME_POOL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/memory_pool.h"

#include <assert.h>
#include <stddef.h>

/// Pool of call frames for nested calls.
/// Pre-allocates 1024 frames to avoid allocation during execution.
/// Note: This is a large structure (~128 KB). Allocate on heap, not stack.
typedef struct {
  call_frame_t frames[EVM_MAX_CALL_DEPTH];
  size_t depth;
} call_frame_pool_t;

/// Initializes the call frame pool.
static inline void call_frame_pool_init(call_frame_pool_t *pool) {
  pool->depth = 0;
}

/// Borrows a call frame from the pool.
/// @param pool The frame pool
/// @return Pointer to a fresh call frame, or nullptr if max depth exceeded
static inline call_frame_t *call_frame_pool_rent(call_frame_pool_t *pool) {
  assert(pool->depth < EVM_MAX_CALL_DEPTH);
  if (pool->depth >= EVM_MAX_CALL_DEPTH) {
    return nullptr;
  }

  call_frame_t *frame = &pool->frames[pool->depth++];
  call_frame_reset(frame);
  return frame;
}

/// Returns a call frame to the pool.
/// @param pool The frame pool
static inline void call_frame_pool_return(call_frame_pool_t *pool) {
  assert(pool->depth > 0);
  if (pool->depth > 0) {
    --pool->depth;
  }
}

/// Returns current pool depth.
static inline size_t call_frame_pool_depth(const call_frame_pool_t *pool) {
  return pool->depth;
}

/// Returns the frame at a specific depth (0 = root frame).
static inline call_frame_t *call_frame_pool_at(call_frame_pool_t *pool, size_t depth) {
  assert(depth < pool->depth);
  return &pool->frames[depth];
}

#endif // DIV0_EVM_CALL_FRAME_POOL_H
