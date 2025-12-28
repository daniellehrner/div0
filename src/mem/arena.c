#include "div0/mem/arena.h"

#include "div0/mem/stc_allocator.h"

/// Thread-local arena pointer for STC containers.
/// Must be initialized before using STC containers.
/// Defined here as the canonical definition; world_state.c also initializes it.
_Thread_local div0_arena_t *div0_stc_arena = nullptr;