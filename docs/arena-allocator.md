# Arena Allocator

The div0 arena allocator provides fast, bulk-resettable memory allocation optimized for EVM execution. It uses a bump-pointer allocation strategy with chained 64KB blocks.

## Overview

### Why Arena Allocation?

EVM execution has a specific memory usage pattern:
- Many small allocations during execution (stack values, temporary buffers)
- All memory can be freed at once when execution completes
- Memory is reused across multiple contract executions

Traditional `malloc`/`free` is inefficient for this pattern because:
- Each allocation has overhead (headers, fragmentation)
- Individual `free` calls are expensive
- Memory isn't easily reused between executions

The arena allocator solves this with:
- **Fast allocation**: Simple pointer bump, no searching for free blocks
- **Bulk reset**: One operation resets all memory for reuse
- **Block reuse**: Blocks are kept after reset, avoiding repeated `malloc` calls

### Architecture

```
┌───────────────────────────────────────────────────────────────────┐
│                         div0_arena_t                              │
│  ┌─────────┐    ┌─────────────────────────────────────────────┐   │
│  │  head   │───▶│ div0_arena_block_t (64KB)                   │   │
│  ├─────────┤    │  ┌──────┬────────┬────────────────────────┐ │   │
│  │ current │    │  │ next │ offset │       data[]           │ │   │
│  └─────────┘    │  └──┬───┴────────┴────────────────────────┘ │   │
│                 └─────┼───────────────────────────────────────┘   │
│                       │                                           │
│                       ▼                                           │
│                 ┌─────────────────────────────────────────────┐   │
│                 │ div0_arena_block_t (64KB)                   │   │
│                 │  ┌──────┬────────┬────────────────────────┐ │   │
│                 │  │ next │ offset │       data[]           │ │   │
│                 │  └──┬───┴────────┴────────────────────────┘ │   │
│                 └─────┼───────────────────────────────────────┘   │
│                       │                                           │
│                       ▼                                           │
│                     nullptr                                       │
└───────────────────────────────────────────────────────────────────┘
```

## API Reference

### Types

```c
// Block size (configurable via DIV0_ARENA_BLOCK_SIZE)
#define DIV0_ARENA_BLOCK_SIZE (64 * 1024)  // 64KB default

// Single memory block in the chain
typedef struct div0_arena_block {
  struct div0_arena_block *next;  // Next block in chain
  size_t offset;                   // Current allocation offset
  uint8_t data[DIV0_ARENA_BLOCK_SIZE];
} div0_arena_block_t;

// Arena allocator
typedef struct {
  div0_arena_block_t *head;     // First block
  div0_arena_block_t *current;  // Block being allocated from
} div0_arena_t;
```

### Functions

#### `div0_arena_init`

```c
[[nodiscard]] bool div0_arena_init(div0_arena_t *arena);
```

Initialize an arena with its first block.

- **Returns**: `true` on success, `false` if `malloc` fails
- **Note**: Must be called before any other arena operations

#### `div0_arena_alloc`

```c
[[nodiscard]] void *div0_arena_alloc(div0_arena_t *arena, size_t size);
```

Allocate memory from the arena.

- All allocations are 8-byte aligned
- Allocations larger than `DIV0_ARENA_BLOCK_SIZE` return `nullptr`
- New blocks are allocated automatically when needed
- **Returns**: Pointer to allocated memory, or `nullptr` on failure

#### `div0_arena_realloc`

```c
[[nodiscard]] void *div0_arena_realloc(div0_arena_t *arena, void *ptr,
                                        size_t old_size, size_t new_size);
```

Reallocate memory (allocates new space, copies data).

- Old memory is not freed (reclaimed on reset)
- **Returns**: Pointer to new memory, or `nullptr` on failure

#### `div0_arena_free`

```c
void div0_arena_free(div0_arena_t *arena, void *ptr, size_t size);
```

Free memory (no-op). Memory is reclaimed via `div0_arena_reset`.

#### `div0_arena_reset`

```c
void div0_arena_reset(div0_arena_t *arena);
```

Reset arena for reuse.

- All previous allocations become invalid
- Blocks are kept allocated for reuse
- `current` pointer resets to `head`

#### `div0_arena_destroy`

```c
void div0_arena_destroy(div0_arena_t *arena);
```

Destroy arena and free all blocks.

## Usage Examples

### Basic Usage

```c
#include "div0/mem/arena.h"

// Initialize arena
div0_arena_t arena;
if (!div0_arena_init(&arena)) {
  // Handle allocation failure
  return;
}

// Allocate memory
void *ptr = div0_arena_alloc(&arena, 256);

// Reset for next execution (keeps blocks)
div0_arena_reset(&arena);

// Cleanup when done
div0_arena_destroy(&arena);
```

### With EVM Stack

The EVM stack uses the arena for its backing storage:

```c
#include "div0/mem/arena.h"
#include "div0/evm/stack.h"

div0_arena_t arena;
div0_arena_init(&arena);

// Stack allocates from arena
evm_stack_t stack;
evm_stack_init(&stack, &arena);

// Push values (may grow, allocating from arena)
evm_stack_push(&stack, uint256_from_u64(42));

// Reset arena between executions
div0_arena_reset(&arena);
```

### With Stack Pool

For managing multiple stacks (e.g., call frames):

```c
#include "div0/evm/stack_pool.h"

div0_arena_t arena;
div0_arena_init(&arena);

evm_stack_pool_t pool;
evm_stack_pool_init(&pool, &arena);

// Borrow stacks for call frames
evm_stack_t *stack1 = evm_stack_pool_borrow(&pool);
evm_stack_t *stack2 = evm_stack_pool_borrow(&pool);

// Return is a no-op (memory reclaimed on arena reset)
evm_stack_pool_return(&pool, stack1);
evm_stack_pool_return(&pool, stack2);

// Reset arena for next transaction
div0_arena_reset(&arena);
```

### With STC Containers

The arena integrates with STC (Simple Template Containers) library:

```c
#include "div0/mem/stc_allocator.h"  // Include BEFORE STC headers
#include <stc/vec.h>

div0_arena_t arena;
div0_arena_init(&arena);

// Point STC at our arena
div0_stc_arena = &arena;

// STC containers now use arena allocation
// ...

div0_arena_reset(&arena);
```

## Execution Lifecycle

Typical usage pattern for contract execution:

```
┌─────────────────────────────────────────────────────────────┐
│ Application Start                                           │
│   div0_arena_init(&arena)  ←── Allocates first 64KB block   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ Transaction 1                                               │
│   - evm_stack_init (allocates from arena)                   │
│   - Execute bytecode (stack grows as needed)                │
│   - div0_arena_reset()  ←── Resets offsets, keeps blocks    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ Transaction 2                                               │
│   - Reuses existing blocks (no malloc calls)                │
│   - May allocate new blocks if needed                       │
│   - div0_arena_reset()                                      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ Application Shutdown                                        │
│   div0_arena_destroy(&arena)  ←── Frees all blocks          │
└─────────────────────────────────────────────────────────────┘
```

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `alloc` | O(1) amortized | Bump pointer; new block on overflow |
| `realloc` | O(n) | Copies data to new location |
| `free` | O(1) | No-op |
| `reset` | O(blocks) | Resets offset in each block |
| `destroy` | O(blocks) | Frees each block |

### Memory Overhead

- Per-block overhead: `sizeof(void*) + sizeof(size_t)` (~16 bytes)
- Per-allocation overhead: 0-7 bytes (alignment padding)
- No fragmentation within blocks

## Configuration

### Block Size

Override the default 64KB block size at compile time:

```c
#define DIV0_ARENA_BLOCK_SIZE (128 * 1024)  // 128KB blocks
#include "div0/mem/arena.h"
```

Considerations:
- Larger blocks = fewer allocations, more potential waste
- Smaller blocks = more allocations, less waste
- 64KB is a good balance for EVM workloads

## Thread Safety

The arena allocator is **not thread-safe**. Each thread should have its own arena, or external synchronization must be used.

## Files

| File | Description |
|------|-------------|
| `include/div0/mem/arena.h` | Arena allocator (header-only) |
| `include/div0/mem/stc_allocator.h` | STC integration |
| `src/mem/arena.c` | Global STC arena pointer |
| `include/div0/evm/stack.h` | Stack using arena |
| `include/div0/evm/stack_pool.h` | Stack pool using arena |
