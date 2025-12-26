#include "div0/mem/arena.h"

#include "div0/mem/stc_allocator.h"

/// Global arena pointer for STC containers.
/// Must be initialized before using STC containers.
div0_arena_t *div0_stc_arena = nullptr;