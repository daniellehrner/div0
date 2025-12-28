#ifndef DIV0_EVM_OPCODES_CALL_H
#define DIV0_EVM_OPCODES_CALL_H

#include "div0/evm/call_frame.h"
#include "div0/evm/frame_result.h"
#include "div0/evm/status.h"

#include <stdbool.h>

// Forward declarations
typedef struct evm evm_t;

/// Result of a CALL opcode execution.
typedef struct {
  bool should_call;   // true = child frame ready, false = continue or error
  bool has_error;     // true = error occurred (check error field)
  evm_status_t error; // Error status if has_error is true
} call_op_result_t;

/// Create a "continue" result (pushed failure, continue execution).
static inline call_op_result_t call_op_continue(void) {
  return (call_op_result_t){.should_call = false, .has_error = false, .error = EVM_OK};
}

/// Create a "call" result (child frame set up, return FRAME_CALL).
static inline call_op_result_t call_op_call(void) {
  return (call_op_result_t){.should_call = true, .has_error = false, .error = EVM_OK};
}

/// Create an "error" result.
static inline call_op_result_t call_op_error(evm_status_t err) {
  return (call_op_result_t){.should_call = false, .has_error = true, .error = err};
}

/// Execute CALL opcode.
/// Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
/// @param evm EVM instance (for state access and pending frame)
/// @param frame Current call frame
/// @return Call operation result
call_op_result_t op_call(evm_t *evm, call_frame_t *frame);

/// Execute STATICCALL opcode.
/// Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
/// @param evm EVM instance
/// @param frame Current call frame
/// @return Call operation result
call_op_result_t op_staticcall(evm_t *evm, call_frame_t *frame);

/// Execute DELEGATECALL opcode.
/// Stack: [gas, addr, argsOffset, argsSize, retOffset, retSize] => []
/// @param evm EVM instance
/// @param frame Current call frame
/// @return Call operation result
call_op_result_t op_delegatecall(evm_t *evm, call_frame_t *frame);

/// Execute CALLCODE opcode.
/// Stack: [gas, addr, value, argsOffset, argsSize, retOffset, retSize] => []
/// @param evm EVM instance
/// @param frame Current call frame
/// @return Call operation result
call_op_result_t op_callcode(evm_t *evm, call_frame_t *frame);

#endif // DIV0_EVM_OPCODES_CALL_H
