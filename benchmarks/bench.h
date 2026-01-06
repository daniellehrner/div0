#ifndef DIV0_BENCH_H
#define DIV0_BENCH_H

// Simple C benchmarking macros for div0
// Uses CLOCK_MONOTONIC for high-resolution timing

#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Default number of iterations for benchmarks
#define BENCH_DEFAULT_ITERATIONS 1000000

// Get current time in nanoseconds
static inline uint64_t bench_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Benchmark context
typedef struct {
  const char *name;
  uint64_t iterations;
  uint64_t start_ns;
  uint64_t end_ns;
} bench_ctx_t;

// Initialize a benchmark
static inline void bench_init(bench_ctx_t *ctx, const char *name, uint64_t iterations) {
  ctx->name = name;
  ctx->iterations = iterations;
  ctx->start_ns = 0;
  ctx->end_ns = 0;
}

// Start timing
static inline void bench_start(bench_ctx_t *ctx) { ctx->start_ns = bench_now_ns(); }

// Stop timing
static inline void bench_stop(bench_ctx_t *ctx) { ctx->end_ns = bench_now_ns(); }

// Get elapsed time in nanoseconds
static inline uint64_t bench_elapsed_ns(const bench_ctx_t *ctx) {
  return ctx->end_ns - ctx->start_ns;
}

// Get time per operation in nanoseconds
static inline double bench_ns_per_op(const bench_ctx_t *ctx) {
  return (double)bench_elapsed_ns(ctx) / (double)ctx->iterations;
}

// Print benchmark results
static inline void bench_print(const bench_ctx_t *ctx) {
  double ns_per_op = bench_ns_per_op(ctx);
  double ops_per_sec = 1e9 / ns_per_op;

  printf("%-40s %10.2f ns/op   %12.0f ops/sec\n", ctx->name, ns_per_op, ops_per_sec);
}

// Simple macro for running a benchmark
#define BENCH_RUN(name, iterations, code)                                                          \
  do {                                                                                             \
    bench_ctx_t _bench_ctx;                                                                        \
    bench_init(&_bench_ctx, name, iterations);                                                     \
    bench_start(&_bench_ctx);                                                                      \
    for (uint64_t _bench_i = 0; _bench_i < (iterations); _bench_i++) {                             \
      code;                                                                                        \
    }                                                                                              \
    bench_stop(&_bench_ctx);                                                                       \
    bench_print(&_bench_ctx);                                                                      \
  } while (0)

// Macro to prevent compiler from optimizing away a value
#define BENCH_DO_NOT_OPTIMIZE(var)                                                                 \
  do {                                                                                             \
    __asm__ volatile("" : : "r,m"(var) : "memory");                                                \
  } while (0)

// Print benchmark section header
static inline void bench_section(const char *name) {
  printf("\n=== %s ===\n", name);
  printf("%-40s %14s   %14s\n", "Benchmark", "Time", "Throughput");
  printf("--------------------------------------------------------------------------------\n");
}

#endif // DIV0_BENCH_H
