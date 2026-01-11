#ifndef DIV0_EVM_LOG_VEC_H
#define DIV0_EVM_LOG_VEC_H

#include "div0/evm/log.h"
#include "div0/mem/arena.h"

#include <stdbool.h>
#include <stddef.h>

/// Dynamic vector of logs.
/// No explicit capacity limit - gas costs naturally constrain log count.
typedef struct {
  evm_log_t *data;     // Array of logs (arena-allocated)
  size_t size;         // Number of logs
  size_t capacity;     // Allocated capacity
  div0_arena_t *arena; // Backing allocator
} evm_log_vec_t;

/// Initializes log vector.
void evm_log_vec_init(evm_log_vec_t *vec, div0_arena_t *arena);

/// Resets log vector (size = 0, keeps capacity).
void evm_log_vec_reset(evm_log_vec_t *vec);

/// Appends a log entry. Returns false on allocation failure.
[[nodiscard]] bool evm_log_vec_push(evm_log_vec_t *vec, const evm_log_t *log);

/// Returns pointer to logs array.
[[nodiscard]] static inline const evm_log_t *evm_log_vec_data(const evm_log_vec_t *vec) {
  return vec->data;
}

/// Returns number of logs.
[[nodiscard]] static inline size_t evm_log_vec_size(const evm_log_vec_t *vec) {
  return vec->size;
}

#endif // DIV0_EVM_LOG_VEC_H
