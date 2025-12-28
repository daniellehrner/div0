#ifndef DIV0_EVM_MEMORY_H
#define DIV0_EVM_MEMORY_H

#include "div0/mem/arena.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// Initial memory capacity (1 KB).
static constexpr size_t EVM_MEMORY_INITIAL_CAPACITY = 1024;

/// EVM linear memory.
/// Byte-addressable, grows in 32-byte words.
/// Memory expansion is charged gas according to EIP-150.
typedef struct {
  uint8_t *data;       // Memory buffer
  size_t size;         // Current size (always multiple of 32)
  size_t capacity;     // Allocated capacity
  div0_arena_t *arena; // Backing allocator
} evm_memory_t;

/// Initializes EVM memory with an arena allocator.
void evm_memory_init(evm_memory_t *mem, div0_arena_t *arena);

/// Resets memory to empty state (keeps arena reference).
void evm_memory_reset(evm_memory_t *mem);

/// Calculates memory expansion gas cost.
/// @param current_words Current memory size in 32-byte words
/// @param new_words New required memory size in 32-byte words
/// @return Gas cost for expansion (0 if no expansion needed)
uint64_t evm_memory_expansion_cost(size_t current_words, size_t new_words);

/// Ensures memory is expanded to cover [offset, offset + size).
/// @param mem Memory to expand
/// @param offset Start offset
/// @param size Number of bytes needed
/// @param gas_cost Output gas cost for expansion (0 if no expansion needed)
/// @return true if successful, false on allocation failure
bool evm_memory_expand(evm_memory_t *mem, size_t offset, size_t size, uint64_t *gas_cost);

/// Returns current memory size in bytes (MSIZE opcode).
static inline size_t evm_memory_size(const evm_memory_t *mem) {
  return mem->size;
}

/// Stores a single byte at offset.
/// Caller must ensure memory is already expanded.
static inline void evm_memory_store8_unsafe(evm_memory_t *mem, size_t offset, uint8_t value) {
  mem->data[offset] = value;
}

/// Stores a 256-bit value at offset (big-endian, MSTORE opcode).
/// Caller must ensure memory is already expanded.
void evm_memory_store32_unsafe(evm_memory_t *mem, size_t offset, uint256_t value);

/// Stores arbitrary bytes at offset.
/// Caller must ensure memory is already expanded.
static inline void evm_memory_store_unsafe(evm_memory_t *mem, size_t offset, const uint8_t *data,
                                           size_t len) {
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(mem->data + offset, data, len);
}

/// Loads a 256-bit value from offset (big-endian, MLOAD opcode).
/// Caller must ensure memory is already expanded.
uint256_t evm_memory_load32_unsafe(const evm_memory_t *mem, size_t offset);

/// Loads arbitrary bytes from offset.
/// Caller must ensure memory is already expanded.
static inline void evm_memory_load_unsafe(const evm_memory_t *mem, size_t offset, uint8_t *out,
                                          size_t len) {
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(out, mem->data + offset, len);
}

/// Returns a read-only pointer to memory data.
/// @param mem Memory
/// @param offset Start offset
/// @param len Length (caller must ensure in bounds)
/// @return Pointer to memory region
static inline const uint8_t *evm_memory_ptr_unsafe(const evm_memory_t *mem, size_t offset) {
  return mem->data + offset;
}

/// Copies data within memory (for MCOPY opcode, EIP-5656).
/// Handles overlapping regions correctly.
/// Caller must ensure memory is already expanded.
static inline void evm_memory_copy_unsafe(evm_memory_t *mem, size_t dest, size_t src, size_t len) {
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memmove(mem->data + dest, mem->data + src, len);
}

#endif // DIV0_EVM_MEMORY_H
