#ifndef DIV0_EVM_STATUS_H
#define DIV0_EVM_STATUS_H

/// EVM operation status codes.
typedef enum {
  EVM_OK = 0,
  EVM_STACK_OVERFLOW,
  EVM_STACK_UNDERFLOW,
  EVM_INVALID_OPCODE,
  EVM_OUT_OF_GAS,
} evm_status_t;

/// EVM execution result.
typedef enum {
  EVM_RESULT_STOP,  // Normal termination (STOP opcode or end of code)
  EVM_RESULT_ERROR, // Execution error (see status field)
} evm_result_t;

#endif // DIV0_EVM_STATUS_H