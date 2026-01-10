#ifndef DIV0_EVM_JUMPDEST_H
#define DIV0_EVM_JUMPDEST_H

#include "div0/evm/opcodes.h"
#include "div0/mem/arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// =============================================================================
// Jump Destination Analysis
// =============================================================================

/// Get bitmap size required for code of given size.
/// @param code_size Size of bytecode in bytes
/// @return Size of bitmap in bytes (1 bit per code byte)
[[nodiscard]] static inline size_t jumpdest_bitmap_size(size_t code_size) {
  return (code_size + 7) / 8;
}

/// Check if position is a valid jump destination.
/// @param bitmap Jumpdest bitmap from jumpdest_compute_bitmap
/// @param code_size Size of original bytecode
/// @param dest Destination position to check
/// @return true if dest is a valid JUMPDEST
[[nodiscard]] static inline bool jumpdest_is_valid(const uint8_t *bitmap, size_t code_size,
                                                   uint64_t dest) {
  if (dest >= code_size) {
    return false;
  }
  return (bitmap[dest / 8] & (1U << (dest % 8))) != 0;
}

/// Compute jumpdest bitmap for bytecode.
/// Scans bytecode to find JUMPDEST opcodes, skipping PUSH data bytes.
/// Allocates (code_size + 7) / 8 bytes from arena.
/// @param code Bytecode to analyze
/// @param code_size Length of bytecode
/// @param arena Arena allocator for bitmap allocation
/// @return Bitmap pointer, or nullptr on allocation failure or empty code
[[nodiscard]] static inline uint8_t *jumpdest_compute_bitmap(const uint8_t *code, size_t code_size,
                                                             div0_arena_t *arena) {
  if (code_size == 0) {
    return nullptr;
  }

  size_t bitmap_size = jumpdest_bitmap_size(code_size);
  uint8_t *bitmap = (uint8_t *)div0_arena_alloc(arena, bitmap_size);
  if (bitmap == nullptr) {
    return nullptr;
  }

  // Zero-initialize bitmap
  __builtin___memset_chk(bitmap, 0, bitmap_size, __builtin_object_size(bitmap, 0));

  // Scan bytecode for JUMPDESTs, skipping PUSH data
  for (size_t pc = 0; pc < code_size;) {
    uint8_t opcode = code[pc];

    if (opcode == OP_JUMPDEST) {
      // Set bit at position pc
      bitmap[pc / 8] |= (1U << (pc % 8));
      pc++;
    } else if (opcode >= OP_PUSH1 && opcode <= OP_PUSH32) {
      // Skip PUSH opcode and its data bytes (1-32 bytes)
      size_t push_size = (size_t)(opcode - OP_PUSH1) + 1;
      pc += 1 + push_size;
    } else {
      pc++;
    }
  }

  return bitmap;
}

#endif // DIV0_EVM_JUMPDEST_H
