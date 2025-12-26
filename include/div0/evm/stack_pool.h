#ifndef DIV0_EVM_STACK_POOL_H
#define DIV0_EVM_STACK_POOL_H

#include "div0/evm/stack.h"
#include "div0/mem/arena.h"

/// Stack pool for managing EVM stacks.
/// Provides stacks for call frames, backed by arena.
typedef struct {
  div0_arena_t *arena;
} evm_stack_pool_t;

/// Initialize stack pool with arena.
/// @param pool Pool to initialize
/// @param arena Arena for stack allocations
static inline void evm_stack_pool_init(evm_stack_pool_t *pool, div0_arena_t *arena) {
  pool->arena = arena;
}

/// Borrow a stack from the pool.
/// @param pool Pool to borrow from
/// @return New stack, or nullptr on allocation failure
[[nodiscard]] static inline evm_stack_t *evm_stack_pool_borrow(evm_stack_pool_t *pool) {
  evm_stack_t *stack = (evm_stack_t *)div0_arena_alloc(pool->arena, sizeof(evm_stack_t));
  if (!stack) {
    return nullptr;
  }
  if (!evm_stack_init(stack, pool->arena)) {
    return nullptr;
  }
  return stack;
}

/// Return a stack to the pool.
/// Currently a no-op; memory reclaimed on arena reset.
static inline void evm_stack_pool_return([[maybe_unused]] evm_stack_pool_t *pool,
                                         [[maybe_unused]] evm_stack_t *stack) {}

#endif // DIV0_EVM_STACK_POOL_H
