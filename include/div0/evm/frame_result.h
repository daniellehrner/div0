#ifndef DIV0_EVM_FRAME_RESULT_H
#define DIV0_EVM_FRAME_RESULT_H

#include "div0/evm/status.h"

#include <stdint.h>

/// Frame execution action.
/// Signals to the main loop what action to take next.
typedef enum : uint8_t {
  FRAME_STOP = 0, // STOP opcode or end of code
  FRAME_RETURN,   // RETURN opcode with data
  FRAME_REVERT,   // REVERT opcode with data
  FRAME_CALL,     // CALL/STATICCALL/DELEGATECALL/CALLCODE
  FRAME_CREATE,   // CREATE/CREATE2
  FRAME_ERROR,    // Execution error (out of gas, stack error, etc.)
} frame_action_t;

/// Frame execution result.
/// Returned by execute_frame() to signal the main loop.
typedef struct {
  frame_action_t action;  // What action to take
  evm_status_t error;     // Error code if action == FRAME_ERROR
  uint64_t return_offset; // Memory offset for RETURN/REVERT data
  uint64_t return_size;   // Size of RETURN/REVERT data
} frame_result_t;

/// Creates a STOP result.
static inline frame_result_t frame_result_stop(void) {
  return (frame_result_t){
      .action = FRAME_STOP,
      .error = EVM_OK,
      .return_offset = 0,
      .return_size = 0,
  };
}

/// Creates a RETURN result.
static inline frame_result_t frame_result_return(uint64_t offset, uint64_t size) {
  return (frame_result_t){
      .action = FRAME_RETURN,
      .error = EVM_OK,
      .return_offset = offset,
      .return_size = size,
  };
}

/// Creates a REVERT result.
static inline frame_result_t frame_result_revert(uint64_t offset, uint64_t size) {
  return (frame_result_t){
      .action = FRAME_REVERT,
      .error = EVM_OK,
      .return_offset = offset,
      .return_size = size,
  };
}

/// Creates a CALL result (signals main loop to push frame and switch).
static inline frame_result_t frame_result_call(void) {
  return (frame_result_t){
      .action = FRAME_CALL,
      .error = EVM_OK,
      .return_offset = 0,
      .return_size = 0,
  };
}

/// Creates a CREATE result (signals main loop to push frame and switch).
static inline frame_result_t frame_result_create(void) {
  return (frame_result_t){
      .action = FRAME_CREATE,
      .error = EVM_OK,
      .return_offset = 0,
      .return_size = 0,
  };
}

/// Creates an error result.
static inline frame_result_t frame_result_error(evm_status_t error) {
  return (frame_result_t){
      .action = FRAME_ERROR,
      .error = error,
      .return_offset = 0,
      .return_size = 0,
  };
}

#endif // DIV0_EVM_FRAME_RESULT_H
