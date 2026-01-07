// Benchmarks for EVM stack manipulation operations
// Tests POP, DUP, and SWAP opcode performance

#include "bench.h"
#include "div0/evm/stack.h"
#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include <stdint.h>
#include <stdio.h>

// Fixed seed for reproducibility
enum { BENCH_SEED = 42 };

// Simple PRNG (xorshift64)
static uint64_t prng_state = BENCH_SEED;

static uint64_t xorshift64(void) {
  uint64_t x = prng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  prng_state = x;
  return x;
}

static uint256_t random_uint256(void) {
  return uint256_from_limbs(xorshift64(), xorshift64(), xorshift64(), xorshift64());
}

static void reset_prng(void) { prng_state = BENCH_SEED; }

// =============================================================================
// POP Benchmarks
// =============================================================================

static void bench_pop(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Pre-fill stack with values
  for (int i = 0; i < 100; i++) {
    (void)evm_stack_push(&stack, random_uint256());
  }

  uint256_t result;
  BENCH_RUN("stack_pop_unsafe", BENCH_DEFAULT_ITERATIONS, {
    // Refill if empty
    if (evm_stack_is_empty(&stack)) {
      for (int i = 0; i < 100; i++) {
        evm_stack_push_unsafe(&stack, uint256_from_u64(i));
      }
    }
    result = evm_stack_pop_unsafe(&stack);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// DUP Benchmarks
// =============================================================================

static void bench_dup1(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push one value
  (void)evm_stack_push(&stack, random_uint256());

  BENCH_RUN("stack_dup_unsafe (depth=1)", BENCH_DEFAULT_ITERATIONS, {
    // Reset if stack getting too large
    if (evm_stack_size(&stack) > 500) {
      evm_stack_clear(&stack);
      evm_stack_push_unsafe(&stack, uint256_from_u64(1));
    }
    evm_stack_dup_unsafe(&stack, 1);
  });
}

static void bench_dup8(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push 8 values
  for (int i = 0; i < 8; i++) {
    (void)evm_stack_push(&stack, uint256_from_u64(i + 1));
  }

  BENCH_RUN("stack_dup_unsafe (depth=8)", BENCH_DEFAULT_ITERATIONS, {
    // Reset if stack getting too large
    if (evm_stack_size(&stack) > 500) {
      evm_stack_clear(&stack);
      for (int i = 0; i < 8; i++) {
        evm_stack_push_unsafe(&stack, uint256_from_u64(i + 1));
      }
    }
    evm_stack_dup_unsafe(&stack, 8);
  });
}

static void bench_dup16(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push 16 values
  for (int i = 0; i < 16; i++) {
    (void)evm_stack_push(&stack, uint256_from_u64(i + 1));
  }

  BENCH_RUN("stack_dup_unsafe (depth=16)", BENCH_DEFAULT_ITERATIONS, {
    // Reset if stack getting too large
    if (evm_stack_size(&stack) > 500) {
      evm_stack_clear(&stack);
      for (int i = 0; i < 16; i++) {
        evm_stack_push_unsafe(&stack, uint256_from_u64(i + 1));
      }
    }
    evm_stack_dup_unsafe(&stack, 16);
  });
}

// =============================================================================
// SWAP Benchmarks
// =============================================================================

static void bench_swap1(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push 2 values
  (void)evm_stack_push(&stack, uint256_from_u64(1));
  (void)evm_stack_push(&stack, uint256_from_u64(2));

  BENCH_RUN("stack_swap_unsafe (depth=1)", BENCH_DEFAULT_ITERATIONS, {
    evm_stack_swap_unsafe(&stack, 1);
  });
}

static void bench_swap8(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push 9 values (need depth+1 items for SWAP)
  for (int i = 0; i < 9; i++) {
    (void)evm_stack_push(&stack, uint256_from_u64(i + 1));
  }

  BENCH_RUN("stack_swap_unsafe (depth=8)", BENCH_DEFAULT_ITERATIONS, {
    evm_stack_swap_unsafe(&stack, 8);
  });
}

static void bench_swap16(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  // Push 17 values (need depth+1 items for SWAP)
  for (int i = 0; i < 17; i++) {
    (void)evm_stack_push(&stack, uint256_from_u64(i + 1));
  }

  BENCH_RUN("stack_swap_unsafe (depth=16)", BENCH_DEFAULT_ITERATIONS, {
    evm_stack_swap_unsafe(&stack, 16);
  });
}

// =============================================================================
// Push/Pop Combined Benchmarks
// =============================================================================

static void bench_push_pop_cycle(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  const uint256_t value = random_uint256();
  uint256_t result;

  BENCH_RUN("push_unsafe + pop_unsafe cycle", BENCH_DEFAULT_ITERATIONS, {
    evm_stack_push_unsafe(&stack, value);
    result = evm_stack_pop_unsafe(&stack);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_push_dup_pop_cycle(div0_arena_t *arena) {
  evm_stack_t stack;
  (void)evm_stack_init(&stack, arena);

  const uint256_t value = random_uint256();
  uint256_t result;

  BENCH_RUN("push + dup1 + pop + pop cycle", BENCH_DEFAULT_ITERATIONS, {
    evm_stack_push_unsafe(&stack, value);
    evm_stack_dup_unsafe(&stack, 1);
    result = evm_stack_pop_unsafe(&stack);
    result = evm_stack_pop_unsafe(&stack);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
  printf("Stack Operations Benchmarks\n");
  printf("============================\n\n");

  div0_arena_t arena;
  if (!div0_arena_init(&arena)) {
    (void)fprintf(stderr, "Failed to initialize arena\n"); // NOLINT(cert-err33-c)
    return 1;
  }

  bench_section("POP Operations");
  reset_prng();
  bench_pop(&arena);
  div0_arena_reset(&arena);

  bench_section("DUP Operations");
  reset_prng();
  bench_dup1(&arena);
  div0_arena_reset(&arena);
  bench_dup8(&arena);
  div0_arena_reset(&arena);
  bench_dup16(&arena);
  div0_arena_reset(&arena);

  bench_section("SWAP Operations");
  reset_prng();
  bench_swap1(&arena);
  div0_arena_reset(&arena);
  bench_swap8(&arena);
  div0_arena_reset(&arena);
  bench_swap16(&arena);
  div0_arena_reset(&arena);

  bench_section("Combined Operations");
  reset_prng();
  bench_push_pop_cycle(&arena);
  div0_arena_reset(&arena);
  bench_push_dup_pop_cycle(&arena);
  div0_arena_reset(&arena);

  div0_arena_destroy(&arena);

  printf("\nBenchmarks complete.\n");
  return 0;
}
