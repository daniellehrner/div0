#include "div0/evm/log_vec.h"

void evm_log_vec_init(evm_log_vec_t *vec, div0_arena_t *arena) {
  vec->data = nullptr;
  vec->size = 0;
  vec->capacity = 0;
  vec->arena = arena;
}

void evm_log_vec_reset(evm_log_vec_t *vec) {
  vec->size = 0;
}

bool evm_log_vec_push(evm_log_vec_t *vec, const evm_log_t *log) {
  if (vec->size >= vec->capacity) {
    // Double capacity (or start with 16)
    const size_t new_cap = vec->capacity == 0 ? 16 : vec->capacity * 2;
    const auto new_data = (evm_log_t *)div0_arena_alloc(vec->arena, new_cap * sizeof(evm_log_t));
    if (new_data == nullptr) {
      return false;
    }
    if (vec->data != nullptr && vec->size > 0) {
      __builtin___memcpy_chk(new_data, vec->data, vec->size * sizeof(evm_log_t),
                             new_cap * sizeof(evm_log_t));
    }
    vec->data = new_data;
    vec->capacity = new_cap;
  }

  vec->data[vec->size++] = *log;
  return true;
}
