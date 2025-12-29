#include "div0/evm/evm.h"

#include "div0/evm/gas.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/opcodes/call.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void evm_init(evm_t *evm, div0_arena_t *arena) {
  __builtin___memset_chk(evm, 0, sizeof(evm_t), __builtin_object_size(evm, 0));
  evm->arena = arena;
  call_frame_pool_init(&evm->frame_pool);
  evm_stack_pool_init(&evm->stack_pool, arena);
  evm_memory_pool_init(&evm->memory_pool, arena);
}

void evm_reset(evm_t *evm) {
  // Reset pools that support depth tracking to their initial logical state.
  // This function only resets per-execution state; it does NOT reclaim memory
  // from evm->arena. The arena is expected to have a lifetime that encompasses
  // all uses of this evm_t instance (or be reset/destroyed by the caller when
  // the EVM is no longer needed).
  evm->frame_pool.depth = 0;
  evm->memory_pool.depth = 0;
  // stack_pool doesn't have depth tracking: individual stacks are allocated
  // on-demand from evm->arena and are not reclaimed by evm_reset. Reusing an
  // evm_t across multiple transactions with the same arena is only safe if the
  // arena itself is periodically reset or discarded to avoid unbounded growth.

  // Clear return data size (buffer storage is kept and reused via evm->arena)
  evm->return_data_size = 0;

  // Clear current execution state
  evm->current_frame = nullptr;
  evm->pending_frame = nullptr;
}

/// Initializes the root frame from execution environment.
static void init_root_frame(evm_t *evm, call_frame_t *frame, const execution_env_t *env) {
  frame->pc = 0;
  frame->gas = env->call.gas;
  frame->stack = evm_stack_pool_borrow(&evm->stack_pool);
  frame->memory = evm_memory_pool_borrow(&evm->memory_pool);
  frame->code = env->call.code;
  frame->code_size = env->call.code_size;
  frame->output_offset = 0;
  frame->output_size = 0;
  frame->depth = 0;
  frame->exec_type = EXEC_TX_START;
  frame->is_static = env->call.is_static;
  frame->caller = env->call.caller;
  frame->address = env->call.address;
  frame->value = env->call.value;
  frame->input = env->call.input;
  frame->input_size = env->call.input_size;
}

/// Executes a single frame until it returns, calls, or errors.
static frame_result_t execute_frame(evm_t *evm, call_frame_t *frame);

/// Copies return data from frame memory to EVM's stable buffer.
/// Must be called before releasing the frame's memory.
static void copy_return_data(evm_t *evm, const evm_memory_t *mem, uint64_t offset, uint64_t size) {
  if (size == 0) {
    evm->return_data_size = 0;
    return;
  }

  // Ensure we have enough capacity
  if (size > evm->return_data_capacity) {
    // Allocate from arena (round up to power of 2 for efficiency)
    size_t new_capacity = 256;
    while (new_capacity < size) {
      // Prevent overflow when doubling
      if (new_capacity > SIZE_MAX / 2) {
        new_capacity = size;
        break;
      }
      new_capacity *= 2;
    }
    evm->return_data = div0_arena_alloc(evm->arena, new_capacity);
    evm->return_data_capacity = new_capacity;
  }

  // Copy data from frame memory to stable buffer
  evm_memory_load_unsafe(mem, offset, evm->return_data, size);
  evm->return_data_size = size;
}

evm_execution_result_t evm_execute_env(evm_t *evm, const execution_env_t *env) {
  // Set context references
  evm->block = env->block;
  evm->tx = &env->tx;

  // Initialize root frame
  call_frame_t *frame = call_frame_pool_rent(&evm->frame_pool);
  if (frame == nullptr) {
    return (evm_execution_result_t){
        .result = EVM_RESULT_ERROR,
        .error = EVM_CALL_DEPTH_EXCEEDED,
        .gas_used = 0,
        .gas_refund = 0,
        .output = nullptr,
        .output_size = 0,
    };
  }
  init_root_frame(evm, frame, env);
  evm->current_frame = frame;

  // Frame stack for parent tracking during nested calls
  call_frame_t *frame_stack[EVM_MAX_CALL_DEPTH];
  size_t stack_depth = 0;

  uint64_t initial_gas = env->call.gas;

  // Main execution loop
  while (true) {
    frame_result_t result = execute_frame(evm, frame);

    switch (result.action) {
    case FRAME_STOP:
    case FRAME_RETURN:
      if (frame->depth == 0) {
        // Top-level success - copy return data if RETURN
        if (result.action == FRAME_RETURN) {
          copy_return_data(evm, frame->memory, result.return_offset, result.return_size);
        }
        uint64_t gas_used = initial_gas - frame->gas;
        return (evm_execution_result_t){
            .result = EVM_RESULT_STOP,
            .error = EVM_OK,
            .gas_used = gas_used,
            .gas_refund = 0,
            .output = evm->return_data,
            .output_size = evm->return_data_size,
        };
      }
      // Child frame returned successfully - handle return to parent
      {
        call_frame_t *parent = frame_stack[--stack_depth];

        // Copy return data to EVM's stable buffer (before releasing frame memory)
        copy_return_data(evm, frame->memory, result.return_offset, result.return_size);

        // Copy return data to parent's memory at output location
        size_t copy_size = parent->output_size;
        if (copy_size > evm->return_data_size) {
          copy_size = evm->return_data_size;
        }
        if (copy_size > 0 && parent->memory != nullptr) {
          evm_memory_store_unsafe(parent->memory, parent->output_offset, evm->return_data,
                                  copy_size);
        }

        // Return unused gas to parent
        parent->gas += frame->gas;

        // Push 1 (success) onto parent's stack
        evm_stack_push_unsafe(parent->stack, uint256_from_u64(1));

        // Release child frame resources and switch to parent
        evm_stack_pool_return(&evm->stack_pool, frame->stack);
        evm_memory_pool_return(&evm->memory_pool);
        call_frame_pool_return(&evm->frame_pool);
        frame = parent;
        evm->current_frame = frame;
      }
      break;

    case FRAME_REVERT:
      if (frame->depth == 0) {
        // Top-level revert - copy revert data
        copy_return_data(evm, frame->memory, result.return_offset, result.return_size);
        uint64_t gas_used = initial_gas - frame->gas;
        return (evm_execution_result_t){
            .result = EVM_RESULT_ERROR,
            .error = EVM_OK, // Revert is not a fatal error
            .gas_used = gas_used,
            .gas_refund = 0, // No refund on revert
            .output = evm->return_data,
            .output_size = evm->return_data_size,
        };
      }
      // Child frame reverted - handle return to parent
      {
        call_frame_t *parent = frame_stack[--stack_depth];

        // Revert state changes if state is set
        // (snapshot_id would be stored in frame, simplified here)

        // Copy revert data to EVM's stable buffer (before releasing frame memory)
        copy_return_data(evm, frame->memory, result.return_offset, result.return_size);

        // Copy revert data to parent's memory at output location
        size_t copy_size = parent->output_size;
        if (copy_size > evm->return_data_size) {
          copy_size = evm->return_data_size;
        }
        if (copy_size > 0 && parent->memory != nullptr) {
          evm_memory_store_unsafe(parent->memory, parent->output_offset, evm->return_data,
                                  copy_size);
        }

        // Return unused gas to parent (on revert, gas is returned)
        parent->gas += frame->gas;

        // Push 0 (failure) onto parent's stack
        evm_stack_push_unsafe(parent->stack, uint256_zero());

        // Release child frame resources and switch to parent
        evm_stack_pool_return(&evm->stack_pool, frame->stack);
        evm_memory_pool_return(&evm->memory_pool);
        call_frame_pool_return(&evm->frame_pool);
        frame = parent;
        evm->current_frame = frame;
      }
      break;

    case FRAME_CALL:
    case FRAME_CREATE:
      // Bounds check before pushing to frame stack
      if (stack_depth >= EVM_MAX_CALL_DEPTH) {
        return (evm_execution_result_t){
            .result = EVM_RESULT_ERROR,
            .error = EVM_CALL_DEPTH_EXCEEDED,
            .gas_used = initial_gas,
            .gas_refund = 0,
            .output = nullptr,
            .output_size = 0,
        };
      }
      // Push current frame onto stack, switch to child
      frame_stack[stack_depth++] = frame;
      frame = evm->pending_frame;
      evm->pending_frame = nullptr;
      evm->current_frame = frame;
      break;

    case FRAME_ERROR:
      if (frame->depth == 0) {
        // Top-level error - all gas consumed
        return (evm_execution_result_t){
            .result = EVM_RESULT_ERROR,
            .error = result.error,
            .gas_used = initial_gas,
            .gas_refund = 0,
            .output = nullptr,
            .output_size = 0,
        };
      }
      // Child frame error - handle return to parent (all child gas consumed)
      {
        call_frame_t *parent = frame_stack[--stack_depth];

        // Clear return data on error
        evm->return_data_size = 0;

        // Child gas is NOT returned on error (consumed)

        // Push 0 (failure) onto parent's stack
        evm_stack_push_unsafe(parent->stack, uint256_zero());

        // Release child frame resources and switch to parent
        evm_stack_pool_return(&evm->stack_pool, frame->stack);
        evm_memory_pool_return(&evm->memory_pool);
        call_frame_pool_return(&evm->frame_pool);
        frame = parent;
        evm->current_frame = frame;
      }
      break;
    }
  }
}

// Constants for dispatch table
static constexpr int OPCODE_TABLE_SIZE = 256;
static constexpr uint8_t OPCODE_MAX = 0xFF;

/// Executes a single frame until it returns, calls, or errors.
/// Uses computed gotos for efficient opcode dispatch.
static frame_result_t execute_frame(evm_t *evm, call_frame_t *frame) {
  // Dispatch table using computed gotos (GCC/Clang extension)
  static void *dispatch_table[OPCODE_TABLE_SIZE] = {
      [0x00 ... OPCODE_MAX] = &&op_invalid,
      [OP_STOP] = &&op_stop,
      [OP_ADD] = &&op_add,
      [OP_PUSH1] = &&op_push1,
      [OP_PUSH2] = &&op_push2,
      [OP_PUSH3] = &&op_push3,
      [OP_PUSH4] = &&op_push4,
      [OP_PUSH5] = &&op_push5,
      [OP_PUSH6] = &&op_push6,
      [OP_PUSH7] = &&op_push7,
      [OP_PUSH8] = &&op_push8,
      [OP_PUSH9] = &&op_push9,
      [OP_PUSH10] = &&op_push10,
      [OP_PUSH11] = &&op_push11,
      [OP_PUSH12] = &&op_push12,
      [OP_PUSH13] = &&op_push13,
      [OP_PUSH14] = &&op_push14,
      [OP_PUSH15] = &&op_push15,
      [OP_PUSH16] = &&op_push16,
      [OP_PUSH17] = &&op_push17,
      [OP_PUSH18] = &&op_push18,
      [OP_PUSH19] = &&op_push19,
      [OP_PUSH20] = &&op_push20,
      [OP_PUSH21] = &&op_push21,
      [OP_PUSH22] = &&op_push22,
      [OP_PUSH23] = &&op_push23,
      [OP_PUSH24] = &&op_push24,
      [OP_PUSH25] = &&op_push25,
      [OP_PUSH26] = &&op_push26,
      [OP_PUSH27] = &&op_push27,
      [OP_PUSH28] = &&op_push28,
      [OP_PUSH29] = &&op_push29,
      [OP_PUSH30] = &&op_push30,
      [OP_PUSH31] = &&op_push31,
      [OP_PUSH32] = &&op_push32,
      [OP_MSTORE] = &&op_mstore,
      [OP_MSTORE8] = &&op_mstore8,
      [OP_CALL] = &&op_call,
      [OP_STATICCALL] = &&op_staticcall,
      [OP_DELEGATECALL] = &&op_delegatecall,
      [OP_CALLCODE] = &&op_callcode,
      [OP_RETURN] = &&op_return,
      [OP_REVERT] = &&op_revert,
  };

#define DISPATCH()                   \
  if (frame->pc >= frame->code_size) \
    goto done;                       \
  goto *dispatch_table[frame->code[frame->pc++]]

  // Start execution
  DISPATCH();

op_stop:
  return frame_result_stop();

op_add:
  if (!evm_stack_has_items(frame->stack, 2)) {
    return frame_result_error(EVM_STACK_UNDERFLOW);
  }
  {
    uint256_t a = evm_stack_pop_unsafe(frame->stack);
    uint256_t b = evm_stack_pop_unsafe(frame->stack);
    evm_stack_push_unsafe(frame->stack, uint256_add(a, b));
  }
  DISPATCH();

op_push1:
op_push2:
op_push3:
op_push4:
op_push5:
op_push6:
op_push7:
op_push8:
op_push9:
op_push10:
op_push11:
op_push12:
op_push13:
op_push14:
op_push15:
op_push16:
op_push17:
op_push18:
op_push19:
op_push20:
op_push21:
op_push22:
op_push23:
op_push24:
op_push25:
op_push26:
op_push27:
op_push28:
op_push29:
op_push30:
op_push31:
op_push32: {
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return frame_result_error(EVM_STACK_OVERFLOW);
  }
  // Calculate n from opcode (we went back 1 in pc after dispatch)
  uint8_t prev_opcode = frame->code[frame->pc - 1];
  size_t n = (size_t)prev_opcode - (size_t)OP_PUSH1 + 1U;

  size_t available = frame->code_size - frame->pc;
  size_t to_read = (n < available) ? n : available;
  uint256_t value = uint256_from_bytes_be(frame->code + frame->pc, to_read);
  frame->pc += n;
  evm_stack_push_unsafe(frame->stack, value);
  DISPATCH();
}

op_mstore: {
  // Stack: [offset, value] => []
  if (!evm_stack_has_items(frame->stack, 2)) {
    return frame_result_error(EVM_STACK_UNDERFLOW);
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t value = evm_stack_pop_unsafe(frame->stack);

  if (!uint256_fits_u64(offset_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, 32, &mem_cost)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }

  // Charge base gas (GAS_VERY_LOW) + memory expansion
  if (mem_cost > UINT64_MAX - GAS_VERY_LOW) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t total_cost = GAS_VERY_LOW + mem_cost;
  if (frame->gas < total_cost) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  frame->gas -= total_cost;

  evm_memory_store32_unsafe(frame->memory, offset, value);
  DISPATCH();
}

op_mstore8: {
  // Stack: [offset, value] => []
  if (!evm_stack_has_items(frame->stack, 2)) {
    return frame_result_error(EVM_STACK_UNDERFLOW);
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t value = evm_stack_pop_unsafe(frame->stack);

  if (!uint256_fits_u64(offset_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);

  // Calculate memory expansion cost
  uint64_t mem_cost = 0;
  if (!evm_memory_expand(frame->memory, offset, 1, &mem_cost)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }

  // Charge base gas (GAS_VERY_LOW) + memory expansion
  if (mem_cost > UINT64_MAX - GAS_VERY_LOW) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t total_cost = GAS_VERY_LOW + mem_cost;
  if (frame->gas < total_cost) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  frame->gas -= total_cost;

  // Store low byte of value
  evm_memory_store8_unsafe(frame->memory, offset, (uint8_t)(value.limbs[0] & 0xFF));
  DISPATCH();
}

op_call: {
  call_op_result_t result = op_call(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_staticcall: {
  call_op_result_t result = op_staticcall(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_delegatecall: {
  call_op_result_t result = op_delegatecall(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_callcode: {
  call_op_result_t result = op_callcode(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_return: {
  // Stack: [offset, size] => []
  if (!evm_stack_has_items(frame->stack, 2)) {
    return frame_result_error(EVM_STACK_UNDERFLOW);
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Validate offset and size fit in uint64
  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Expand memory if needed
  if (size > 0) {
    uint64_t mem_cost = 0;
    if (!evm_memory_expand(frame->memory, offset, size, &mem_cost)) {
      return frame_result_error(EVM_OUT_OF_GAS);
    }
    if (frame->gas < mem_cost) {
      return frame_result_error(EVM_OUT_OF_GAS);
    }
    frame->gas -= mem_cost;
  }

  return frame_result_return(offset, size);
}

op_revert: {
  // Stack: [offset, size] => []
  if (!evm_stack_has_items(frame->stack, 2)) {
    return frame_result_error(EVM_STACK_UNDERFLOW);
  }
  uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Validate offset and size fit in uint64
  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  uint64_t size = uint256_to_u64_unsafe(size_u256);

  // Expand memory if needed
  if (size > 0) {
    uint64_t mem_cost = 0;
    if (!evm_memory_expand(frame->memory, offset, size, &mem_cost)) {
      return frame_result_error(EVM_OUT_OF_GAS);
    }
    if (frame->gas < mem_cost) {
      return frame_result_error(EVM_OUT_OF_GAS);
    }
    frame->gas -= mem_cost;
  }

  return frame_result_revert(offset, size);
}

op_invalid:
  return frame_result_error(EVM_INVALID_OPCODE);

done:
  return frame_result_stop();

#undef DISPATCH
}
