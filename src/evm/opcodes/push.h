#ifndef DIV0_EVM_OPCODES_PUSH_IMPL_H
#define DIV0_EVM_OPCODES_PUSH_IMPL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/opcodes.h"
#include "div0/evm/stack.h"
#include "div0/evm/status.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline evm_status_t op_push0(call_frame_t *frame, uint64_t gas_cost) {
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  frame->gas -= gas_cost;
  evm_stack_push_unsafe(frame->stack, uint256_zero());
  return EVM_OK;
}

static inline evm_status_t op_push_n(call_frame_t *frame, size_t n, uint64_t gas_cost) {
  if (frame->gas < gas_cost) {
    return EVM_OUT_OF_GAS;
  }
  if (!evm_stack_ensure_space(frame->stack, 1)) {
    return EVM_STACK_OVERFLOW;
  }
  frame->gas -= gas_cost;

  size_t available = frame->code_size - frame->pc;
  size_t to_read = (n < available) ? n : available;
  uint256_t value = uint256_from_bytes_be(frame->code + frame->pc, to_read);
  frame->pc += n;
  evm_stack_push_unsafe(frame->stack, value);
  return EVM_OK;
}

#endif // DIV0_EVM_OPCODES_PUSH_IMPL_H
