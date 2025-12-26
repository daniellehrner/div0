#ifndef DIV0_MEM_ARENA_H
#define DIV0_MEM_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// Alignment for arena allocations (8 bytes).
static constexpr size_t DIV0_ARENA_ALIGNMENT = 8;
static constexpr size_t DIV0_ARENA_ALIGN_MASK = DIV0_ARENA_ALIGNMENT - 1;

/// Block size for arena allocations.
/// Note: Using #define instead of constexpr because this is used as an array
/// size in div0_arena_block_t and some compilers (especially with bare-metal
/// toolchains) may have issues with constexpr in this context.
#ifndef DIV0_ARENA_BLOCK_SIZE
#define DIV0_ARENA_BLOCK_SIZE ((size_t)64 * 1024)
#endif

/// Arena block - a single 64KB memory region.
/// Blocks are chained together when more memory is needed.
typedef struct div0_arena_block {
  struct div0_arena_block *next;
  size_t offset; // Current allocation offset within data[]
  uint8_t data[DIV0_ARENA_BLOCK_SIZE];
} div0_arena_block_t;

/// Arena allocator using chained 64KB blocks.
/// Fast bump-pointer allocation, bulk reset, reusable between executions.
typedef struct {
  div0_arena_block_t *head;    // First block in chain
  div0_arena_block_t *current; // Block currently allocating from
} div0_arena_t;

/// Initialize arena with first block.
/// @param arena Arena to initialize
/// @return true on success, false on allocation failure
[[nodiscard]] static inline bool div0_arena_init(div0_arena_t *arena) {
  div0_arena_block_t *block = (div0_arena_block_t *)malloc(sizeof(div0_arena_block_t));
  if (!block) {
    return false;
  }
  block->next = nullptr;
  block->offset = 0;
  arena->head = block;
  arena->current = block;
  return true;
}

/// Allocate memory from arena (8-byte aligned).
/// @param arena Arena to allocate from
/// @param size Number of bytes to allocate
/// @return Pointer to allocated memory, or nullptr if allocation fails
[[nodiscard]] static inline void *div0_arena_alloc(div0_arena_t *arena, size_t size) {
  if (size == 0 || size > SIZE_MAX - DIV0_ARENA_ALIGN_MASK) {
    return nullptr;
  }

  // Align size to 8 bytes
  size_t aligned_size = (size + DIV0_ARENA_ALIGN_MASK) & ~DIV0_ARENA_ALIGN_MASK;

  // Allocation larger than block size not supported
  if (aligned_size > DIV0_ARENA_BLOCK_SIZE) {
    return nullptr;
  }

  // Try current block
  div0_arena_block_t *block = arena->current;
  size_t aligned_offset = (block->offset + DIV0_ARENA_ALIGN_MASK) & ~DIV0_ARENA_ALIGN_MASK;

  if (aligned_offset + aligned_size <= DIV0_ARENA_BLOCK_SIZE) {
    void *ptr = block->data + aligned_offset;
    block->offset = aligned_offset + aligned_size;
    return ptr;
  }

  // Current block full - check if next block exists (from previous execution)
  if (block->next) {
    arena->current = block->next;
    void *ptr = arena->current->data;
    arena->current->offset = aligned_size;
    return ptr;
  }

  // Need new block
  div0_arena_block_t *new_block = (div0_arena_block_t *)malloc(sizeof(div0_arena_block_t));
  if (!new_block) {
    return nullptr;
  }
  new_block->next = nullptr;
  new_block->offset = aligned_size;

  block->next = new_block;
  arena->current = new_block;
  return new_block->data;
}

/// Reallocate memory (allocates new space, copies data, old space wasted until reset).
/// @param arena Arena to allocate from
/// @param ptr Previous pointer (can be nullptr)
/// @param old_size Previous size (used for copy)
/// @param new_size New size to allocate
/// @return Pointer to new memory, or nullptr on failure
[[nodiscard]] static inline void *div0_arena_realloc(div0_arena_t *arena, void *ptr,
                                                     size_t old_size, size_t new_size) {
  void *new_ptr = div0_arena_alloc(arena, new_size);
  if (!new_ptr) {
    return nullptr;
  }
  if (ptr && old_size > 0) {
    size_t copy_size = old_size < new_size ? old_size : new_size;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(new_ptr, ptr, copy_size);
  }
  return new_ptr;
}

/// Free memory (no-op - memory reclaimed via reset).
static inline void div0_arena_free([[maybe_unused]] div0_arena_t *arena, [[maybe_unused]] void *ptr,
                                   [[maybe_unused]] size_t size) {}

/// Reset arena for reuse (keeps blocks allocated).
/// All previous allocations become invalid.
static inline void div0_arena_reset(div0_arena_t *arena) {
  for (div0_arena_block_t *block = arena->head; block != nullptr; block = block->next) {
    block->offset = 0;
  }
  arena->current = arena->head;
}

/// Destroy arena and free all blocks.
static inline void div0_arena_destroy(div0_arena_t *arena) {
  div0_arena_block_t *block = arena->head;
  while (block) {
    div0_arena_block_t *next = block->next;
    free(block);
    block = next;
  }
  arena->head = nullptr;
  arena->current = nullptr;
}

#endif // DIV0_MEM_ARENA_H
