#include "div0/evm/evm.h"

#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

// Constants for dispatch table
#define OPCODE_TABLE_SIZE 256
#define OPCODE_MAX 0xFF

void evm_context_init(evm_context_t *ctx, const uint8_t *code, size_t code_size,
                      evm_stack_t *stack) {
  ctx->code = code;
  ctx->code_size = code_size;
  ctx->pc = 0;
  ctx->stack = stack;
  ctx->status = EVM_OK;
}

evm_result_t evm_execute(evm_context_t *ctx) {
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
  };

#define DISPATCH()               \
  if (ctx->pc >= ctx->code_size) \
    goto done;                   \
  goto *dispatch_table[ctx->code[ctx->pc++]]

  // Start execution
  DISPATCH();

op_stop:
  return EVM_RESULT_STOP;

op_add:
  if (evm_stack_size(ctx->stack) < 2) {
    ctx->status = EVM_STACK_UNDERFLOW;
    return EVM_RESULT_ERROR;
  }
  uint256_t a = evm_stack_pop_unsafe(ctx->stack);
  uint256_t b = evm_stack_pop_unsafe(ctx->stack);
  evm_stack_push_unsafe(ctx->stack, uint256_add(a, b));
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
  if (!evm_stack_can_push(ctx->stack)) {
    ctx->status = EVM_STACK_OVERFLOW;
    return EVM_RESULT_ERROR;
  }
  // Calculate n from opcode (we went back 1 in pc after dispatch)
  uint8_t prev_opcode = ctx->code[ctx->pc - 1];
  size_t n = (size_t)prev_opcode - (size_t)OP_PUSH1 + 1U;

  size_t available = ctx->code_size - ctx->pc;
  size_t to_read = (n < available) ? n : available;
  uint256_t value = uint256_from_bytes_be(ctx->code + ctx->pc, to_read);
  ctx->pc += n;
  evm_stack_push_unsafe(ctx->stack, value);
  DISPATCH();
}

op_invalid:
  ctx->status = EVM_INVALID_OPCODE;
  return EVM_RESULT_ERROR;

done:
  return EVM_RESULT_STOP;

#undef DISPATCH
}