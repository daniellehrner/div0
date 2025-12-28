#include "div0/mem/arena.h"

#include "div0/mem/stc_allocator.h"

/// Thread-local arena pointer for STC containers.
/// Must be set before using STC containers (e.g., by world_state_create).
_Thread_local div0_arena_t *div0_stc_arena = nullptr;