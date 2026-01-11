#ifndef DIV0_EVM_OPCODES_LOGGING_H
#define DIV0_EVM_OPCODES_LOGGING_H

#include "div0/evm/call_frame.h"
#include "div0/evm/gas.h"
#include "div0/evm/log.h"
#include "div0/evm/log_vec.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/mem/arena.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdint.h>

// =============================================================================
// Logging Operations
// =============================================================================

/// LOGn opcode (0xA0 - 0xA4): Append log record with n topics
/// Stack: [offset, size, topic0?, ...] => []
/// Gas: 375 + 375*n + 8*size + memory_expansion_cost
/// @param logs Log vector to append to
/// @param arena Arena for log data allocation
/// @param frame Current call frame
/// @param topic_count Number of topics (0-4)
/// @return EVM_OK on success, error status otherwise
static inline evm_status_t op_log_n(evm_log_vec_t *logs, div0_arena_t *arena, call_frame_t *frame,
                                    const uint8_t topic_count) {
  // Stack items needed: 2 (offset, size) + topic_count
  const uint16_t stack_items_needed = (uint16_t)(2 + topic_count);
  if (!evm_stack_has_items(frame->stack, stack_items_needed)) {
    return EVM_STACK_UNDERFLOW;
  }

  // Check static context - LOG is a state-modifying operation
  if (frame->is_static) {
    return EVM_WRITE_PROTECTION;
  }

  // Pop offset and size
  const uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Validate offset and size fit in uint64
  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Pop topics
  hash_t topics[LOG_MAX_TOPICS];
  for (uint8_t i = 0; i < topic_count; i++) {
    const uint256_t topic = evm_stack_pop_unsafe(frame->stack);
    topics[i] = hash_from_uint256(&topic);
  }

  // Calculate gas: base + topics + data + memory expansion
  uint64_t gas_cost = GAS_LOG;

  // Topic cost with overflow check
  const uint64_t topic_cost = (uint64_t)topic_count * GAS_LOG_TOPIC;
  if (gas_cost > UINT64_MAX - topic_cost) {
    return EVM_OUT_OF_GAS;
  }
  gas_cost += topic_cost;

  // Data cost with overflow check
  if (size > UINT64_MAX / GAS_LOG_DATA) {
    return EVM_OUT_OF_GAS;
  }
  const uint64_t data_cost = size * GAS_LOG_DATA;
  if (gas_cost > UINT64_MAX - data_cost) {
    return EVM_OUT_OF_GAS;
  }
  gas_cost += data_cost;

  // Memory expansion cost
  uint64_t mem_cost = 0;
  if (size > 0) {
    if (!evm_memory_expand(frame->memory, offset, size, &mem_cost)) {
      return EVM_OUT_OF_GAS;
    }
    if (gas_cost > UINT64_MAX - mem_cost) {
      return EVM_OUT_OF_GAS;
    }
    gas_cost += mem_cost;
  }

  // Deduct gas
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  frame->gas -= gas_cost;

  // Copy log data from memory to arena
  uint8_t *log_data = nullptr;
  if (size > 0) {
    log_data = (uint8_t *)div0_arena_alloc(arena, size);
    if (log_data == nullptr) {
      return EVM_OUT_OF_GAS;
    }
    evm_memory_load_unsafe(frame->memory, offset, log_data, size);
  }

  // Create log entry
  evm_log_t log = {
      .address = frame->address,
      .topics = {0},
      .topic_count = topic_count,
      .data = log_data,
      .data_size = size,
  };
  // Copy topics
  for (uint8_t i = 0; i < topic_count; i++) {
    log.topics[i] = topics[i];
  }

  // Append to log vector
  if (!evm_log_vec_push(logs, &log)) {
    return EVM_OUT_OF_GAS;
  }

  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_LOGGING_H
