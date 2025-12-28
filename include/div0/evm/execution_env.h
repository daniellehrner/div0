#ifndef DIV0_EVM_EXECUTION_ENV_H
#define DIV0_EVM_EXECUTION_ENV_H

#include "div0/evm/block_context.h"
#include "div0/evm/tx_context.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Initial call parameters for transaction execution.
/// Defines the entry point for EVM execution.
typedef struct {
  uint256_t value;      // CALLVALUE opcode (0x34) - wei sent with call
  uint64_t gas;         // Available gas for execution
  const uint8_t *code;  // Bytecode to execute
  size_t code_size;     // Length of bytecode
  const uint8_t *input; // Calldata (CALLDATALOAD, CALLDATASIZE, CALLDATACOPY)
  size_t input_size;    // Length of calldata
  address_t caller;     // CALLER opcode (0x33) - msg.sender
  address_t address;    // ADDRESS opcode (0x30) - address(this)
  bool is_static;       // True if in static call context
} call_params_t;

/// Initializes call parameters with default values.
static inline void call_params_init(call_params_t *params) {
  params->value = uint256_zero();
  params->gas = 0;
  params->code = nullptr;
  params->code_size = 0;
  params->input = nullptr;
  params->input_size = 0;
  params->caller = address_zero();
  params->address = address_zero();
  params->is_static = false;
}

/// Complete execution environment.
/// Combines block, transaction, and call-level context.
typedef struct {
  const block_context_t *block; // Block context (shared across transactions)
  tx_context_t tx;              // Transaction context
  call_params_t call;           // Initial call parameters
} execution_env_t;

/// Initializes an execution environment with default values.
static inline void execution_env_init(execution_env_t *env) {
  env->block = nullptr;
  tx_context_init(&env->tx);
  call_params_init(&env->call);
}

#endif // DIV0_EVM_EXECUTION_ENV_H
