#ifndef DIV0_EVM_OPCODES_CREATE_H
#define DIV0_EVM_OPCODES_CREATE_H

#include "div0/evm/call_frame.h"
#include "div0/evm/frame_result.h"
#include "div0/evm/status.h"

#include <stdbool.h>

// Forward declarations
typedef struct evm evm_t;

/// Result of a CREATE opcode execution.
typedef struct {
  bool should_create; // true = child frame ready, false = continue or error
  bool has_error;     // true = error occurred (check error field)
  evm_status_t error; // Error status if has_error is true
} create_op_result_t;

/// Create a "continue" result (pushed failure, continue execution).
static inline create_op_result_t create_op_continue(void) {
  return (create_op_result_t){.should_create = false, .has_error = false, .error = EVM_OK};
}

/// Create a "create" result (child frame set up, return FRAME_CREATE).
static inline create_op_result_t create_op_create(void) {
  return (create_op_result_t){.should_create = true, .has_error = false, .error = EVM_OK};
}

/// Create an "error" result.
static inline create_op_result_t create_op_error(evm_status_t err) {
  return (create_op_result_t){.should_create = false, .has_error = true, .error = err};
}

/// Execute CREATE opcode.
/// Stack: [value, offset, size] => [address or 0]
/// @param evm EVM instance (for state access and pending frame)
/// @param frame Current call frame
/// @return Create operation result
create_op_result_t op_create(evm_t *evm, call_frame_t *frame);

/// Execute CREATE2 opcode.
/// Stack: [value, offset, size, salt] => [address or 0]
/// @param evm EVM instance (for state access and pending frame)
/// @param frame Current call frame
/// @return Create operation result
create_op_result_t op_create2(evm_t *evm, call_frame_t *frame);

#endif // DIV0_EVM_OPCODES_CREATE_H
