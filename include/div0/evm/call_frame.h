#ifndef DIV0_EVM_CALL_FRAME_H
#define DIV0_EVM_CALL_FRAME_H

#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Execution type for call frames.
typedef enum {
  EXEC_TX_START = 0, // Top-level transaction entry (root frame)
  EXEC_CALL,         // CALL - new context, can transfer value
  EXEC_STATICCALL,   // STATICCALL - read-only, no state changes
  EXEC_DELEGATECALL, // DELEGATECALL - callee code in caller context
  EXEC_CALLCODE,     // CALLCODE - legacy, similar to DELEGATECALL
  EXEC_CREATE,       // CREATE opcode - contract creation
  EXEC_CREATE2,      // CREATE2 opcode - deterministic address
} exec_type_t;

/// Call frame for nested EVM execution.
/// Cache-aligned for performance: hot data in first 64 bytes.
struct call_frame {
  // === Hot data (first cache line, 64 bytes) ===
  alignas(64) uint64_t pc; // Program counter (aligned to cache line)
  uint64_t gas;            // Remaining gas
  evm_stack_t *stack;      // Operand stack (from pool)
  evm_memory_t *memory;    // Linear memory (from pool)
  const uint8_t *code;     // Bytecode pointer
  size_t code_size;        // Bytecode length
  uint64_t output_offset;  // Parent's return data offset
  uint32_t output_size;    // Max return size parent accepts
  uint16_t depth;          // Call depth (max 1024)
  exec_type_t exec_type;   // Execution type
  bool is_static;          // Static context flag (no state modifications)

  // === Cold data (second cache line) ===
  address_t caller;     // CALLER opcode - msg.sender
  address_t address;    // ADDRESS opcode - address(this)
  uint256_t value;      // CALLVALUE opcode - msg.value
  const uint8_t *input; // Calldata pointer
  size_t input_size;    // Calldata size

  // Jump destination analysis (lazy, set on first JUMP/JUMPI)
  const uint8_t *jumpdest_bitmap; // 1 bit per code byte, nullptr = not analyzed
  hash_t code_hash;               // For cache lookup (set if known)
};

typedef struct call_frame call_frame_t;

/// Resets a call frame to initial state.
static inline void call_frame_reset(call_frame_t *frame) {
  frame->pc = 0;
  frame->gas = 0;
  frame->stack = nullptr;
  frame->memory = nullptr;
  frame->code = nullptr;
  frame->code_size = 0;
  frame->output_offset = 0;
  frame->output_size = 0;
  frame->depth = 0;
  frame->exec_type = EXEC_TX_START;
  frame->is_static = false;
  frame->caller = address_zero();
  frame->address = address_zero();
  frame->value = uint256_zero();
  frame->input = nullptr;
  frame->input_size = 0;
  frame->jumpdest_bitmap = nullptr;
  frame->code_hash = hash_zero();
}

/// Returns true if the frame is in a static context.
/// Static context prohibits state modifications (SSTORE, CREATE, LOG, etc.).
static inline bool call_frame_is_static(const call_frame_t *frame) {
  return frame->is_static;
}

/// Returns true if the frame can transfer value.
static inline bool call_frame_can_transfer_value(const call_frame_t *frame) {
  switch (frame->exec_type) {
  case EXEC_TX_START:
  case EXEC_CALL:
  case EXEC_CALLCODE:
    return true;
  default:
    return false;
  }
}

/// Returns the current opcode at the program counter.
/// Returns 0x00 (STOP) if PC is out of bounds.
static inline uint8_t call_frame_current_opcode(const call_frame_t *frame) {
  if (frame->pc >= frame->code_size) {
    return 0x00; // STOP
  }
  return frame->code[frame->pc];
}

#endif // DIV0_EVM_CALL_FRAME_H
