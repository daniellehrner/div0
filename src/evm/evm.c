#include "div0/evm/evm.h"

#include "div0/evm/gas/static_costs.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/opcodes/call.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include "jumpdest.h"
#include "opcodes/arithmetic.h"
#include "opcodes/bitwise.h"
#include "opcodes/block.h"
#include "opcodes/comparison.h"
#include "opcodes/context.h"
#include "opcodes/control_flow.h"
#include "opcodes/external.h"
#include "opcodes/keccak.h"
#include "opcodes/logging.h"
#include "opcodes/memory.h"
#include "opcodes/push.h"
#include "opcodes/stack.h"
#include "opcodes/storage.h"

#include <stddef.h>
#include <stdint.h>

void evm_init(evm_t *const evm, div0_arena_t *const arena, const fork_t fork) {
  __builtin___memset_chk(evm, 0, sizeof(evm_t), __builtin_object_size(evm, 0));
  evm->arena = arena;
  evm->fork = fork;

  // Initialize gas table and schedule based on fork
  switch (fork) {
  case FORK_SHANGHAI:
    gas_table_init_shanghai(evm->gas_table);
    evm->gas_schedule = gas_schedule_shanghai();
    break;
  case FORK_CANCUN:
    gas_table_init_cancun(evm->gas_table);
    evm->gas_schedule = gas_schedule_cancun();
    break;
  case FORK_PRAGUE:
  case FORK_UNKNOWN:
    // FORK_UNKNOWN defaults to latest known fork (Prague)
    gas_table_init_prague(evm->gas_table);
    evm->gas_schedule = gas_schedule_prague();
    break;
  }

  call_frame_pool_init(&evm->frame_pool);
  evm_stack_pool_init(&evm->stack_pool, arena);
  evm_memory_pool_init(&evm->memory_pool, arena);

  // Initialize log vector for LOG0-LOG4 opcodes
  evm_log_vec_init(&evm->logs, arena);
}

void evm_reset(evm_t *const evm) {
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

  // Reset gas refund accumulator
  evm->gas_refund = 0;

  // Reset log vector for next transaction
  evm_log_vec_reset(&evm->logs);
}

/// Initializes the root frame from execution environment.
static void init_root_frame(evm_t *const evm, call_frame_t *const frame,
                            const execution_env_t *const env) {
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
  frame->jumpdest_bitmap = nullptr; // Lazy: computed on first JUMP/JUMPI
  frame->code_hash = hash_zero();   // Set by caller if code hash is known
}

/// Executes a single frame until it returns, calls, or errors.
static frame_result_t execute_frame(evm_t *evm, call_frame_t *frame);

/// Copies return data from frame memory to EVM's stable buffer.
/// Must be called before releasing the frame's memory.
static void copy_return_data(evm_t *const evm, const evm_memory_t *const mem, const uint64_t offset,
                             const uint64_t size) {
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

evm_execution_result_t evm_execute_env(evm_t *const evm, const execution_env_t *const env) {
  // Set context references
  evm->block = env->block;
  evm->tx = &env->tx;

  // Initialize root frame
  call_frame_t *const initial_frame = call_frame_pool_rent(&evm->frame_pool);
  if (initial_frame == nullptr) {
    return (evm_execution_result_t){
        .result = EVM_RESULT_ERROR,
        .error = EVM_CALL_DEPTH_EXCEEDED,
        .gas_used = 0,
        .gas_refund = 0,
        .output = nullptr,
        .output_size = 0,
        .logs = nullptr,
        .logs_count = 0,
    };
  }
  init_root_frame(evm, initial_frame, env);
  evm->current_frame = initial_frame;
  call_frame_t *frame = initial_frame;

  // Frame stack for parent tracking during nested calls
  call_frame_t *frame_stack[EVM_MAX_CALL_DEPTH];
  size_t stack_depth = 0;

  const uint64_t initial_gas = env->call.gas;

  // Main execution loop
  while (true) {
    const frame_result_t result = execute_frame(evm, frame);

    switch (result.action) {
    case FRAME_STOP:
    case FRAME_RETURN:
      if (frame->depth == 0) {
        // Top-level success - copy return data if RETURN
        if (result.action == FRAME_RETURN) {
          copy_return_data(evm, frame->memory, result.return_offset, result.return_size);
        }
        const uint64_t gas_used = initial_gas - frame->gas;
        return (evm_execution_result_t){
            .result = EVM_RESULT_STOP,
            .error = EVM_OK,
            .gas_used = gas_used,
            .gas_refund = evm->gas_refund,
            .output = evm->return_data,
            .output_size = evm->return_data_size,
            .logs = evm_log_vec_data(&evm->logs),
            .logs_count = evm_log_vec_size(&evm->logs),
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
        const uint64_t gas_used = initial_gas - frame->gas;
        return (evm_execution_result_t){
            .result = EVM_RESULT_ERROR,
            .error = EVM_OK, // Revert is not a fatal error
            .gas_used = gas_used,
            .gas_refund = 0, // No refund on revert
            .output = evm->return_data,
            .output_size = evm->return_data_size,
            .logs = nullptr, // No logs on revert
            .logs_count = 0,
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
            .logs = nullptr,
            .logs_count = 0,
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
            .logs = nullptr,
            .logs_count = 0,
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

/// Get or compute jumpdest bitmap for current frame (lazy initialization).
/// Checks frame cache first, then state cache, finally computes if needed.
/// @return Bitmap pointer, or nullptr on allocation failure
static const uint8_t *get_jumpdest_bitmap(const evm_t *evm, call_frame_t *frame) {
  // Already computed for this frame?
  if (frame->jumpdest_bitmap != nullptr) {
    return frame->jumpdest_bitmap;
  }

  // Try cache lookup via state_access (if available and code_hash known)
  if (evm->state != nullptr && evm->state->vtable->get_jumpdest_analysis != nullptr) {
    if (!hash_is_zero(&frame->code_hash)) {
      const uint8_t *cached =
          evm->state->vtable->get_jumpdest_analysis(evm->state, &frame->code_hash);
      if (cached != nullptr) {
        frame->jumpdest_bitmap = cached;
        return cached;
      }
    }
  }

  // Compute bitmap from bytecode
  const uint8_t *bitmap = jumpdest_compute_bitmap(frame->code, frame->code_size, evm->arena);
  frame->jumpdest_bitmap = bitmap;

  // Store in cache if available
  if (bitmap != nullptr && evm->state != nullptr &&
      evm->state->vtable->set_jumpdest_analysis != nullptr && !hash_is_zero(&frame->code_hash)) {
    evm->state->vtable->set_jumpdest_analysis(evm->state, &frame->code_hash, bitmap,
                                              jumpdest_bitmap_size(frame->code_size));
  }

  return bitmap;
}

/// Executes a single frame until it returns, calls, or errors.
/// Uses computed gotos for efficient opcode dispatch.
// NOLINTBEGIN(readability-function-size)
static frame_result_t execute_frame(evm_t *evm, call_frame_t *frame) {
  // Dispatch table using computed gotos (GCC/Clang extension)
  static void *dispatch_table[OPCODE_TABLE_SIZE] = {
      [0x00 ... OPCODE_MAX] = &&op_invalid,
      [OP_STOP] = &&op_stop,
      [OP_ADD] = &&op_add,
      [OP_MUL] = &&op_mul,
      [OP_SUB] = &&op_sub,
      [OP_DIV] = &&op_div,
      [OP_SDIV] = &&op_sdiv,
      [OP_MOD] = &&op_mod,
      [OP_SMOD] = &&op_smod,
      [OP_ADDMOD] = &&op_addmod,
      [OP_MULMOD] = &&op_mulmod,
      [OP_EXP] = &&op_exp,
      [OP_SIGNEXTEND] = &&op_signextend,
      // Comparison opcodes
      [OP_LT] = &&op_lt,
      [OP_GT] = &&op_gt,
      [OP_SLT] = &&op_slt,
      [OP_SGT] = &&op_sgt,
      [OP_EQ] = &&op_eq,
      [OP_ISZERO] = &&op_iszero,
      // Bitwise opcodes
      [OP_AND] = &&op_and,
      [OP_OR] = &&op_or,
      [OP_XOR] = &&op_xor,
      [OP_NOT] = &&op_not,
      [OP_BYTE] = &&op_byte,
      [OP_SHL] = &&op_shl,
      [OP_SHR] = &&op_shr,
      [OP_SAR] = &&op_sar,
      [OP_PUSH0] = &&op_push0,
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
      [OP_MLOAD] = &&op_mload,
      [OP_MSTORE] = &&op_mstore,
      [OP_MSTORE8] = &&op_mstore8,
      [OP_MSIZE] = &&op_msize,
      [OP_KECCAK256] = &&op_keccak256,
      // Control flow opcodes
      [OP_JUMP] = &&op_jump,
      [OP_JUMPI] = &&op_jumpi,
      [OP_JUMPDEST] = &&op_jumpdest,
      [OP_PC] = &&op_pc,
      [OP_GAS] = &&op_gas,
      [OP_SLOAD] = &&op_sload,
      [OP_SSTORE] = &&op_sstore,
      // Environmental information opcodes
      [OP_ADDRESS] = &&op_address,
      [OP_BALANCE] = &&op_balance,
      [OP_ORIGIN] = &&op_origin,
      [OP_CALLER] = &&op_caller,
      [OP_CALLVALUE] = &&op_callvalue,
      [OP_CALLDATALOAD] = &&op_calldataload,
      [OP_CALLDATASIZE] = &&op_calldatasize,
      [OP_CALLDATACOPY] = &&op_calldatacopy,
      [OP_CODESIZE] = &&op_codesize,
      [OP_CODECOPY] = &&op_codecopy,
      [OP_GASPRICE] = &&op_gasprice,
      [OP_EXTCODESIZE] = &&op_extcodesize,
      [OP_EXTCODECOPY] = &&op_extcodecopy,
      [OP_RETURNDATASIZE] = &&op_returndatasize,
      [OP_RETURNDATACOPY] = &&op_returndatacopy,
      [OP_EXTCODEHASH] = &&op_extcodehash,
      // Block information opcodes
      [OP_BLOCKHASH] = &&op_blockhash,
      [OP_COINBASE] = &&op_coinbase,
      [OP_TIMESTAMP] = &&op_timestamp,
      [OP_NUMBER] = &&op_number,
      [OP_PREVRANDAO] = &&op_prevrandao,
      [OP_GASLIMIT] = &&op_gaslimit,
      [OP_CHAINID] = &&op_chainid,
      [OP_SELFBALANCE] = &&op_selfbalance,
      [OP_BASEFEE] = &&op_basefee,
      [OP_BLOBHASH] = &&op_blobhash,
      [OP_BLOBBASEFEE] = &&op_blobbasefee,
      [OP_POP] = &&op_pop,
      [OP_DUP1] = &&op_dup1,
      [OP_DUP2] = &&op_dup2,
      [OP_DUP3] = &&op_dup3,
      [OP_DUP4] = &&op_dup4,
      [OP_DUP5] = &&op_dup5,
      [OP_DUP6] = &&op_dup6,
      [OP_DUP7] = &&op_dup7,
      [OP_DUP8] = &&op_dup8,
      [OP_DUP9] = &&op_dup9,
      [OP_DUP10] = &&op_dup10,
      [OP_DUP11] = &&op_dup11,
      [OP_DUP12] = &&op_dup12,
      [OP_DUP13] = &&op_dup13,
      [OP_DUP14] = &&op_dup14,
      [OP_DUP15] = &&op_dup15,
      [OP_DUP16] = &&op_dup16,
      [OP_SWAP1] = &&op_swap1,
      [OP_SWAP2] = &&op_swap2,
      [OP_SWAP3] = &&op_swap3,
      [OP_SWAP4] = &&op_swap4,
      [OP_SWAP5] = &&op_swap5,
      [OP_SWAP6] = &&op_swap6,
      [OP_SWAP7] = &&op_swap7,
      [OP_SWAP8] = &&op_swap8,
      [OP_SWAP9] = &&op_swap9,
      [OP_SWAP10] = &&op_swap10,
      [OP_SWAP11] = &&op_swap11,
      [OP_SWAP12] = &&op_swap12,
      [OP_SWAP13] = &&op_swap13,
      [OP_SWAP14] = &&op_swap14,
      [OP_SWAP15] = &&op_swap15,
      [OP_SWAP16] = &&op_swap16,
      [OP_CALL] = &&op_call,
      [OP_STATICCALL] = &&op_staticcall,
      [OP_DELEGATECALL] = &&op_delegatecall,
      [OP_CALLCODE] = &&op_callcode,
      [OP_RETURN] = &&op_return,
      [OP_REVERT] = &&op_revert,
      // Logging opcodes
      [OP_LOG0] = &&op_log0,
      [OP_LOG1] = &&op_log1,
      [OP_LOG2] = &&op_log2,
      [OP_LOG3] = &&op_log3,
      [OP_LOG4] = &&op_log4,
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
    const uint256_t a = evm_stack_pop_unsafe(frame->stack);
    const uint256_t b = evm_stack_pop_unsafe(frame->stack);
    evm_stack_push_unsafe(frame->stack, uint256_add(a, b));
  }
  DISPATCH();

op_mul: {
  const evm_status_t status = op_mul(frame, evm->gas_table[OP_MUL]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sub: {
  const evm_status_t status = op_sub(frame, evm->gas_table[OP_SUB]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_div: {
  const evm_status_t status = op_div(frame, evm->gas_table[OP_DIV]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sdiv: {
  const evm_status_t status = op_sdiv(frame, evm->gas_table[OP_SDIV]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_mod: {
  const evm_status_t status = op_mod(frame, evm->gas_table[OP_MOD]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_smod: {
  const evm_status_t status = op_smod(frame, evm->gas_table[OP_SMOD]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_addmod: {
  const evm_status_t status = op_addmod(frame, evm->gas_table[OP_ADDMOD]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_mulmod: {
  const evm_status_t status = op_mulmod(frame, evm->gas_table[OP_MULMOD]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_exp: {
  const evm_status_t status = op_exp(frame);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_signextend: {
  const evm_status_t status = op_signextend(frame, evm->gas_table[OP_SIGNEXTEND]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

  // =========================================================================
  // Comparison opcodes
  // =========================================================================

op_lt: {
  const evm_status_t status = op_lt(frame, evm->gas_table[OP_LT]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_gt: {
  const evm_status_t status = op_gt(frame, evm->gas_table[OP_GT]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_slt: {
  const evm_status_t status = op_slt(frame, evm->gas_table[OP_SLT]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sgt: {
  const evm_status_t status = op_sgt(frame, evm->gas_table[OP_SGT]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_eq: {
  const evm_status_t status = op_eq(frame, evm->gas_table[OP_EQ]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_iszero: {
  const evm_status_t status = op_iszero(frame, evm->gas_table[OP_ISZERO]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

  // =========================================================================
  // Bitwise opcodes
  // =========================================================================

op_and: {
  const evm_status_t status = op_and(frame, evm->gas_table[OP_AND]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_or: {
  const evm_status_t status = op_or(frame, evm->gas_table[OP_OR]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_xor: {
  const evm_status_t status = op_xor(frame, evm->gas_table[OP_XOR]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_not: {
  const evm_status_t status = op_not(frame, evm->gas_table[OP_NOT]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_byte: {
  const evm_status_t status = op_byte(frame, evm->gas_table[OP_BYTE]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_shl: {
  const evm_status_t status = op_shl(frame, evm->gas_table[OP_SHL]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_shr: {
  const evm_status_t status = op_shr(frame, evm->gas_table[OP_SHR]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sar: {
  const evm_status_t status = op_sar(frame, evm->gas_table[OP_SAR]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_push0: {
  const evm_status_t status = op_push0(frame, evm->gas_table[OP_PUSH0]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

#define PUSH_N_HANDLER(n)                                                  \
  op_push##n : {                                                           \
    evm_status_t status = op_push_n(frame, n, evm->gas_table[OP_PUSH##n]); \
    if (status != EVM_OK) {                                                \
      return frame_result_error(status);                                   \
    }                                                                      \
    DISPATCH();                                                            \
  }

  PUSH_N_HANDLER(1)
  PUSH_N_HANDLER(2)
  PUSH_N_HANDLER(3)
  PUSH_N_HANDLER(4)
  PUSH_N_HANDLER(5)
  PUSH_N_HANDLER(6)
  PUSH_N_HANDLER(7)
  PUSH_N_HANDLER(8)
  PUSH_N_HANDLER(9)
  PUSH_N_HANDLER(10)
  PUSH_N_HANDLER(11)
  PUSH_N_HANDLER(12)
  PUSH_N_HANDLER(13)
  PUSH_N_HANDLER(14)
  PUSH_N_HANDLER(15)
  PUSH_N_HANDLER(16)
  PUSH_N_HANDLER(17)
  PUSH_N_HANDLER(18)
  PUSH_N_HANDLER(19)
  PUSH_N_HANDLER(20)
  PUSH_N_HANDLER(21)
  PUSH_N_HANDLER(22)
  PUSH_N_HANDLER(23)
  PUSH_N_HANDLER(24)
  PUSH_N_HANDLER(25)
  PUSH_N_HANDLER(26)
  PUSH_N_HANDLER(27)
  PUSH_N_HANDLER(28)
  PUSH_N_HANDLER(29)
  PUSH_N_HANDLER(30)
  PUSH_N_HANDLER(31)
  PUSH_N_HANDLER(32)

#undef PUSH_N_HANDLER

  // =============================================================================
  // Environmental Information Opcodes (0x30-0x3F)
  // =============================================================================

// Macro for simple context opcodes that only need frame and gas_table
#define CONTEXT_OP_HANDLER(name, opcode)                            \
  op_##name : {                                                     \
    evm_status_t status = op_##name(frame, evm->gas_table[opcode]); \
    if (status != EVM_OK) {                                         \
      return frame_result_error(status);                            \
    }                                                               \
    DISPATCH();                                                     \
  }

// Macro for state-dependent opcodes (BALANCE, EXTCODESIZE, etc.)
// Returns EVM_STATE_UNAVAILABLE if state is not available
#define STATE_OP_HANDLER(name, opcode)                  \
  op_##name : {                                         \
    if (evm->state == nullptr) {                        \
      return frame_result_error(EVM_STATE_UNAVAILABLE); \
    }                                                   \
    evm_status_t status = op_##name(frame, evm->state); \
    if (status != EVM_OK) {                             \
      return frame_result_error(status);                \
    }                                                   \
    DISPATCH();                                         \
  }

  CONTEXT_OP_HANDLER(address, OP_ADDRESS)
  STATE_OP_HANDLER(balance, OP_BALANCE)

op_origin: {
  // ORIGIN needs access to evm->tx, so inline here
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return frame_result_error(EVM_STACK_OVERFLOW);
  }
  if (frame->gas < evm->gas_table[OP_ORIGIN]) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  frame->gas -= evm->gas_table[OP_ORIGIN];
  evm_stack_push_unsafe(frame->stack, address_to_uint256(&evm->tx->origin));
  DISPATCH();
}

  CONTEXT_OP_HANDLER(caller, OP_CALLER)
  CONTEXT_OP_HANDLER(callvalue, OP_CALLVALUE)
  CONTEXT_OP_HANDLER(calldataload, OP_CALLDATALOAD)
  CONTEXT_OP_HANDLER(calldatasize, OP_CALLDATASIZE)
  CONTEXT_OP_HANDLER(calldatacopy, OP_CALLDATACOPY)
  CONTEXT_OP_HANDLER(codesize, OP_CODESIZE)
  CONTEXT_OP_HANDLER(codecopy, OP_CODECOPY)
  STATE_OP_HANDLER(extcodesize, OP_EXTCODESIZE)
  STATE_OP_HANDLER(extcodecopy, OP_EXTCODECOPY)

op_gasprice: {
  // GASPRICE needs access to evm->tx, so inline here
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return frame_result_error(EVM_STACK_OVERFLOW);
  }
  if (frame->gas < evm->gas_table[OP_GASPRICE]) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  frame->gas -= evm->gas_table[OP_GASPRICE];
  evm_stack_push_unsafe(frame->stack, evm->tx->gas_price);
  DISPATCH();
}

op_returndatasize: {
  // RETURNDATASIZE needs access to evm->return_data_size, so inline here
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return frame_result_error(EVM_STACK_OVERFLOW);
  }
  if (frame->gas < evm->gas_table[OP_RETURNDATASIZE]) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  frame->gas -= evm->gas_table[OP_RETURNDATASIZE];
  evm_stack_push_unsafe(frame->stack, uint256_from_u64(evm->return_data_size));
  DISPATCH();
}

op_returndatacopy: {
  const evm_status_t status = op_returndatacopy(frame, evm->gas_table[OP_RETURNDATACOPY],
                                                evm->return_data, evm->return_data_size);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

  STATE_OP_HANDLER(extcodehash, OP_EXTCODEHASH)

#undef CONTEXT_OP_HANDLER
#undef STATE_OP_HANDLER

  // =============================================================================
  // Block Information Opcodes (0x40-0x4A)
  // =============================================================================

// Macro for block opcodes that need block context
#define BLOCK_OP_HANDLER(name, opcode)                                          \
  op_##name : {                                                                 \
    evm_status_t status = op_##name(frame, evm->block, evm->gas_table[opcode]); \
    if (status != EVM_OK) {                                                     \
      return frame_result_error(status);                                        \
    }                                                                           \
    DISPATCH();                                                                 \
  }

  BLOCK_OP_HANDLER(blockhash, OP_BLOCKHASH)
  BLOCK_OP_HANDLER(coinbase, OP_COINBASE)
  BLOCK_OP_HANDLER(timestamp, OP_TIMESTAMP)
  BLOCK_OP_HANDLER(number, OP_NUMBER)
  BLOCK_OP_HANDLER(prevrandao, OP_PREVRANDAO)
  BLOCK_OP_HANDLER(gaslimit, OP_GASLIMIT)
  BLOCK_OP_HANDLER(chainid, OP_CHAINID)
  BLOCK_OP_HANDLER(basefee, OP_BASEFEE)
  BLOCK_OP_HANDLER(blobbasefee, OP_BLOBBASEFEE)

#undef BLOCK_OP_HANDLER

op_selfbalance: {
  if (evm->state == nullptr) {
    return frame_result_error(EVM_STATE_UNAVAILABLE);
  }
  const evm_status_t status = op_selfbalance(frame, evm->state, evm->gas_table[OP_SELFBALANCE]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_blobhash: {
  const evm_status_t status = op_blobhash(frame, evm->tx, evm->gas_table[OP_BLOBHASH]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

  // =============================================================================
  // Stack Manipulation Opcodes (POP, DUP, SWAP)
  // =============================================================================

op_pop: {
  const evm_status_t status = op_pop(frame, evm->gas_table[OP_POP]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

#define DUP_N_HANDLER(n)                                               \
  op_dup##n : {                                                        \
    evm_status_t status = op_dup(frame, n, evm->gas_table[OP_DUP##n]); \
    if (status != EVM_OK) {                                            \
      return frame_result_error(status);                               \
    }                                                                  \
    DISPATCH();                                                        \
  }

  DUP_N_HANDLER(1)
  DUP_N_HANDLER(2)
  DUP_N_HANDLER(3)
  DUP_N_HANDLER(4)
  DUP_N_HANDLER(5)
  DUP_N_HANDLER(6)
  DUP_N_HANDLER(7)
  DUP_N_HANDLER(8)
  DUP_N_HANDLER(9)
  DUP_N_HANDLER(10)
  DUP_N_HANDLER(11)
  DUP_N_HANDLER(12)
  DUP_N_HANDLER(13)
  DUP_N_HANDLER(14)
  DUP_N_HANDLER(15)
  DUP_N_HANDLER(16)

#undef DUP_N_HANDLER

#define SWAP_N_HANDLER(n)                                                \
  op_swap##n : {                                                         \
    evm_status_t status = op_swap(frame, n, evm->gas_table[OP_SWAP##n]); \
    if (status != EVM_OK) {                                              \
      return frame_result_error(status);                                 \
    }                                                                    \
    DISPATCH();                                                          \
  }

  SWAP_N_HANDLER(1)
  SWAP_N_HANDLER(2)
  SWAP_N_HANDLER(3)
  SWAP_N_HANDLER(4)
  SWAP_N_HANDLER(5)
  SWAP_N_HANDLER(6)
  SWAP_N_HANDLER(7)
  SWAP_N_HANDLER(8)
  SWAP_N_HANDLER(9)
  SWAP_N_HANDLER(10)
  SWAP_N_HANDLER(11)
  SWAP_N_HANDLER(12)
  SWAP_N_HANDLER(13)
  SWAP_N_HANDLER(14)
  SWAP_N_HANDLER(15)
  SWAP_N_HANDLER(16)

#undef SWAP_N_HANDLER

op_mload: {
  const evm_status_t status = op_mload(frame, evm->gas_table[OP_MLOAD]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_mstore: {
  const evm_status_t status = op_mstore(frame, evm->gas_table[OP_MSTORE]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_mstore8: {
  const evm_status_t status = op_mstore8(frame, evm->gas_table[OP_MSTORE8]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_keccak256: {
  const evm_status_t status = op_keccak256(frame, evm->gas_table[OP_KECCAK256]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_msize: {
  const evm_status_t status = op_msize(frame, evm->gas_table[OP_MSIZE]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

  // =============================================================================
  // Control Flow Opcodes (JUMP, JUMPI, JUMPDEST, PC, GAS)
  // =============================================================================

op_jump: {
  const uint8_t *bitmap = get_jumpdest_bitmap(evm, frame);
  if (bitmap == nullptr) {
    return frame_result_error(EVM_OUT_OF_GAS); // Allocation failure
  }
  const evm_status_t status = op_jump(frame, bitmap, evm->gas_table[OP_JUMP]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_jumpi: {
  const uint8_t *bitmap = get_jumpdest_bitmap(evm, frame);
  if (bitmap == nullptr) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  const evm_status_t status = op_jumpi(frame, bitmap, evm->gas_table[OP_JUMPI]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_jumpdest: {
  const evm_status_t status = op_jumpdest(frame, evm->gas_table[OP_JUMPDEST]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_pc: {
  // PC pushes the position of THIS instruction, frame->pc was already incremented by DISPATCH
  const evm_status_t status = op_pc(frame, frame->pc - 1, evm->gas_table[OP_PC]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_gas: {
  const evm_status_t status = op_gas(frame, evm->gas_table[OP_GAS]);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sload: {
  if (evm->state == nullptr) {
    return frame_result_error(EVM_INVALID_OPCODE);
  }
  const evm_status_t status = op_sload(frame, evm->state, &evm->gas_schedule);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_sstore: {
  if (evm->state == nullptr) {
    return frame_result_error(EVM_INVALID_OPCODE);
  }
  const evm_status_t status = op_sstore(frame, evm->state, &evm->gas_schedule, &evm->gas_refund);
  if (status != EVM_OK) {
    return frame_result_error(status);
  }
  DISPATCH();
}

op_call: {
  const call_op_result_t result = op_call(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_staticcall: {
  const call_op_result_t result = op_staticcall(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_delegatecall: {
  const call_op_result_t result = op_delegatecall(evm, frame);
  if (result.has_error) {
    return frame_result_error(result.error);
  }
  if (result.should_call) {
    return frame_result_call();
  }
  DISPATCH();
}

op_callcode: {
  const call_op_result_t result = op_callcode(evm, frame);
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
  const uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Validate offset and size fit in uint64
  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  const uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

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
  const uint256_t offset_u256 = evm_stack_pop_unsafe(frame->stack);
  const uint256_t size_u256 = evm_stack_pop_unsafe(frame->stack);

  // Validate offset and size fit in uint64
  if (!uint256_fits_u64(offset_u256) || !uint256_fits_u64(size_u256)) {
    return frame_result_error(EVM_OUT_OF_GAS);
  }
  const uint64_t offset = uint256_to_u64_unsafe(offset_u256);
  const uint64_t size = uint256_to_u64_unsafe(size_u256);

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

// =============================================================================
// Logging Opcodes (LOG0-LOG4)
// =============================================================================

#define LOG_N_HANDLER(n)                                              \
  op_log##n : {                                                       \
    evm_status_t status = op_log_n(&evm->logs, evm->arena, frame, n); \
    if (status != EVM_OK) {                                           \
      return frame_result_error(status);                              \
    }                                                                 \
    DISPATCH();                                                       \
  }

  LOG_N_HANDLER(0)
  LOG_N_HANDLER(1)
  LOG_N_HANDLER(2)
  LOG_N_HANDLER(3)
  LOG_N_HANDLER(4)

#undef LOG_N_HANDLER

op_invalid:
  return frame_result_error(EVM_INVALID_OPCODE);

done:
  return frame_result_stop();

#undef DISPATCH
}
// NOLINTEND(readability-function-size)
