#ifndef DIV0_EVM_EVM_H
#define DIV0_EVM_EVM_H

#include "div0/evm/stack.h"
#include "div0/evm/status.h"

#include <stddef.h>
#include <stdint.h>

/// EVM execution context.
typedef struct {
  const uint8_t *code; // Bytecode
  size_t code_size;    // Bytecode length
  size_t pc;           // Program counter
  evm_stack_t *stack;  // Operand stack (pointer to external stack)
  evm_status_t status; // Last error status
} evm_context_t;

/// Initializes an EVM execution context.
/// @param ctx Context to initialize
/// @param code Bytecode to execute
/// @param code_size Length of bytecode
/// @param stack Pointer to an initialized stack
void evm_context_init(evm_context_t *ctx, const uint8_t *code, size_t code_size,
                      evm_stack_t *stack);

/// Executes bytecode until termination or error.
/// @param ctx Initialized execution context
/// @return EVM_RESULT_STOP on success, EVM_RESULT_ERROR on failure (check ctx->status)
evm_result_t evm_execute(evm_context_t *ctx);

#endif // DIV0_EVM_EVM_H