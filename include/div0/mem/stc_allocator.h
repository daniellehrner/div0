#ifndef DIV0_MEM_STC_ALLOCATOR_H
#define DIV0_MEM_STC_ALLOCATOR_H

/// STC allocator bridge for div0 arena.
///
/// Include this header BEFORE any STC headers to redirect
/// STC's memory operations to the div0 arena allocator.
///
/// Usage:
///   div0_arena_t arena;
///   div0_arena_init(&arena);
///   div0_stc_arena = &arena;
///
///   // Now STC containers use the arena
///   #include <stc/vec.h>
///   ...

#include "div0/mem/arena.h"

#include <string.h>

/// Global arena pointer for STC containers.
/// Must be set before using any STC containers.
extern div0_arena_t *div0_stc_arena;

/// Allocate zero-initialized memory from arena.
/// @param arena Arena to allocate from
/// @param n Number of elements
/// @param sz Size of each element
/// @return Pointer to zero-initialized memory, or nullptr on failure
static inline void *div0_arena_calloc(div0_arena_t *arena, size_t n, size_t sz) {
  size_t total = n * sz;
  void *ptr = div0_arena_alloc(arena, total);
  if (ptr) {
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memset(ptr, 0, total);
  }
  return ptr;
}

// Override STC's allocator macros
// NOLINTBEGIN(readability-identifier-naming)
#define c_malloc(sz) div0_arena_alloc(div0_stc_arena, (sz))
#define c_calloc(n, sz) div0_arena_calloc(div0_stc_arena, (n), (sz))
#define c_realloc(ptr, old_sz, new_sz) div0_arena_realloc(div0_stc_arena, (ptr), (old_sz), (new_sz))
#define c_free(ptr, sz) div0_arena_free(div0_stc_arena, (ptr), (sz))
// NOLINTEND(readability-identifier-naming)

#endif // DIV0_MEM_STC_ALLOCATOR_H