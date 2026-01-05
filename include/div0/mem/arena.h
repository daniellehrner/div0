#ifndef DIV0_MEM_ARENA_H
#define DIV0_MEM_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// Default alignment for arena allocations (8 bytes).
static constexpr size_t DIV0_ARENA_ALIGNMENT = 8;

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
  div0_arena_block_t *head;         // First block in chain
  div0_arena_block_t *current;      // Block currently allocating from
  div0_arena_block_t *large_blocks; // Separate chain for large allocations
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
  arena->large_blocks = nullptr;
  return true;
}

/// Allocate memory from arena with specific alignment.
/// For allocations larger than DIV0_ARENA_BLOCK_SIZE, use div0_arena_alloc_large.
/// @param arena Arena to allocate from
/// @param size Number of bytes to allocate (must be <= DIV0_ARENA_BLOCK_SIZE)
/// @param alignment Required alignment (must be power of 2)
/// @return Pointer to allocated memory, or nullptr if allocation fails
[[nodiscard]] static inline void *div0_arena_alloc_aligned(div0_arena_t *arena, size_t size,
                                                           size_t alignment) {
  if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
    return nullptr; // Invalid size or alignment not power of 2
  }

  size_t align_mask = alignment - 1;
  if (size > SIZE_MAX - align_mask) {
    return nullptr;
  }

  // Align size to requested alignment
  size_t aligned_size = (size + align_mask) & ~align_mask;

  // Reject allocations larger than block size
  if (aligned_size > DIV0_ARENA_BLOCK_SIZE) {
    return nullptr;
  }

  // Try current block
  div0_arena_block_t *block = arena->current;

  // Calculate aligned offset within current block
  uintptr_t current_addr = (uintptr_t)(block->data + block->offset);
  uintptr_t aligned_addr = (current_addr + align_mask) & ~align_mask;
  size_t aligned_offset = aligned_addr - (uintptr_t)block->data;

  if (aligned_offset + aligned_size <= DIV0_ARENA_BLOCK_SIZE) {
    void *ptr = block->data + aligned_offset;
    block->offset = aligned_offset + aligned_size;
    return ptr;
  }

  // Current block full - check if next block exists (from previous execution)
  if (block->next) {
    arena->current = block->next;
    // Calculate aligned offset from start of new block
    uintptr_t data_start = (uintptr_t)arena->current->data;
    uintptr_t new_aligned_addr = (data_start + align_mask) & ~align_mask;
    size_t new_aligned_offset = new_aligned_addr - data_start;
    void *ptr = arena->current->data + new_aligned_offset;
    arena->current->offset = new_aligned_offset + aligned_size;
    return ptr;
  }

  // Need new block
  div0_arena_block_t *new_block = (div0_arena_block_t *)malloc(sizeof(div0_arena_block_t));
  if (!new_block) {
    return nullptr;
  }
  new_block->next = nullptr;

  // Calculate aligned offset from start of new block
  uintptr_t data_start = (uintptr_t)new_block->data;
  uintptr_t new_aligned_addr = (data_start + align_mask) & ~align_mask;
  size_t new_aligned_offset = new_aligned_addr - data_start;
  new_block->offset = new_aligned_offset + aligned_size;

  block->next = new_block;
  arena->current = new_block;
  return new_block->data + new_aligned_offset;
}

/// Allocate a large block of memory from arena.
/// Use this for allocations larger than DIV0_ARENA_BLOCK_SIZE (64KB).
/// The block is inserted into the arena chain and freed on arena destruction.
/// @param arena Arena to allocate from
/// @param size Number of bytes to allocate
/// @param alignment Required alignment (must be power of 2)
/// @return Pointer to allocated memory, or nullptr if allocation fails
[[nodiscard]] static inline void *div0_arena_alloc_large(div0_arena_t *arena, size_t size,
                                                         size_t alignment) {
  if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
    return nullptr;
  }

  size_t align_mask = alignment - 1;
  if (size > SIZE_MAX - align_mask) {
    return nullptr;
  }

  size_t aligned_size = (size + align_mask) & ~align_mask;

  // Calculate total block size needed (header + data + alignment padding)
  size_t block_data_size = aligned_size + alignment;
  size_t total_size = sizeof(div0_arena_block_t) - DIV0_ARENA_BLOCK_SIZE + block_data_size;

  div0_arena_block_t *large_block = (div0_arena_block_t *)malloc(total_size);
  if (!large_block) {
    return nullptr;
  }

  // Calculate aligned offset from start of data
  uintptr_t data_start = (uintptr_t)large_block->data;
  uintptr_t aligned_addr = (data_start + align_mask) & ~align_mask;
  size_t aligned_offset = aligned_addr - data_start;

  large_block->offset = block_data_size; // Store block size (not used for allocation)

  // Prepend to separate large blocks chain (keeps them out of regular block iteration)
  large_block->next = arena->large_blocks;
  arena->large_blocks = large_block;

  return large_block->data + aligned_offset;
}

/// Allocate memory from arena (8-byte aligned).
/// @param arena Arena to allocate from
/// @param size Number of bytes to allocate
/// @return Pointer to allocated memory, or nullptr if allocation fails
[[nodiscard]] static inline void *div0_arena_alloc(div0_arena_t *arena, size_t size) {
  return div0_arena_alloc_aligned(arena, size, DIV0_ARENA_ALIGNMENT);
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

/// Free a chain of arena blocks.
static inline void free_block_chain(div0_arena_block_t *head) {
  while (head) {
    div0_arena_block_t *next = head->next;
    free(head);
    head = next;
  }
}

/// Reset arena for reuse (keeps regular blocks allocated, frees large blocks).
/// All previous allocations become invalid.
static inline void div0_arena_reset(div0_arena_t *arena) {
  // Reset regular blocks (keep allocated for reuse)
  for (div0_arena_block_t *block = arena->head; block != nullptr; block = block->next) {
    block->offset = 0;
  }
  arena->current = arena->head;

  // Free large blocks (can't reuse effectively due to variable sizes)
  free_block_chain(arena->large_blocks);
  arena->large_blocks = nullptr;
}

/// Destroy arena and free all blocks.
static inline void div0_arena_destroy(div0_arena_t *arena) {
  free_block_chain(arena->head);
  free_block_chain(arena->large_blocks);
  arena->head = nullptr;
  arena->current = nullptr;
  arena->large_blocks = nullptr;
}

#endif // DIV0_MEM_ARENA_H
