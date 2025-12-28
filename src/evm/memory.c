#include "div0/evm/memory.h"

#include <string.h>

/// Rounds up to the nearest multiple of 32.
static inline size_t round_up_32(size_t n) {
  return (n + 31) & ~((size_t)31);
}

void evm_memory_init(evm_memory_t *mem, div0_arena_t *arena) {
  mem->data = nullptr;
  mem->size = 0;
  mem->capacity = 0;
  mem->arena = arena;
}

void evm_memory_reset(evm_memory_t *mem) {
  // Don't free - arena handles deallocation
  mem->data = nullptr;
  mem->size = 0;
  mem->capacity = 0;
}

uint64_t evm_memory_expansion_cost(size_t current_words, size_t new_words) {
  if (new_words <= current_words) {
    return 0;
  }

  // Memory cost formula from Yellow Paper:
  // G_memory * a + a^2 / 512
  // where a = number of 32-byte words
  // G_memory = 3

  const uint64_t g_memory = 3;
  uint64_t new_cost = (g_memory * new_words) + ((new_words * new_words) / 512);
  uint64_t old_cost = (g_memory * current_words) + ((current_words * current_words) / 512);

  return new_cost - old_cost;
}

bool evm_memory_expand(evm_memory_t *mem, size_t offset, size_t size, uint64_t *gas_cost) {
  if (size == 0) {
    *gas_cost = 0;
    return true;
  }

  // Check for overflow
  if (offset > SIZE_MAX - size) {
    return false;
  }

  size_t required = offset + size;
  size_t new_size = round_up_32(required);

  // Calculate gas cost
  size_t current_words = mem->size / 32;
  size_t new_words = new_size / 32;
  *gas_cost = evm_memory_expansion_cost(current_words, new_words);

  // No expansion needed
  if (new_size <= mem->size) {
    return true;
  }

  // Expand capacity if needed
  if (new_size > mem->capacity) {
    size_t new_capacity = mem->capacity;
    if (new_capacity == 0) {
      new_capacity = EVM_MEMORY_INITIAL_CAPACITY;
    }
    while (new_capacity < new_size) {
      new_capacity *= 2;
    }

    uint8_t *new_data = div0_arena_alloc(mem->arena, new_capacity);
    if (new_data == nullptr) {
      return false;
    }

    // Copy existing data
    if (mem->data != nullptr && mem->size > 0) {
      // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
      memcpy(new_data, mem->data, mem->size);
    }

    mem->data = new_data;
    mem->capacity = new_capacity;
  }

  // Zero-fill new memory
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memset(mem->data + mem->size, 0, new_size - mem->size);
  mem->size = new_size;

  return true;
}

void evm_memory_store32_unsafe(evm_memory_t *mem, size_t offset, uint256_t value) {
  uint8_t bytes[32];
  uint256_to_bytes_be(value, bytes);
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(mem->data + offset, bytes, 32);
}

uint256_t evm_memory_load32_unsafe(const evm_memory_t *mem, size_t offset) {
  return uint256_from_bytes_be(mem->data + offset, 32);
}
