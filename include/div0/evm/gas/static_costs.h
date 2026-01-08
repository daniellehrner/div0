#ifndef DIV0_EVM_GAS_STATIC_COSTS_H
#define DIV0_EVM_GAS_STATIC_COSTS_H

#include "div0/evm/evm.h"
#include "div0/evm/opcodes.h"

#include <stdint.h>

static inline void gas_table_init_shanghai(gas_table_t table) {
  for (int i = 0; i < GAS_TABLE_SIZE; i++) {
    table[i] = 0;
  }

  // Arithmetic opcodes
  table[OP_ADD] = 3;
  table[OP_MUL] = 5;
  table[OP_SUB] = 3;
  table[OP_DIV] = 5;
  table[OP_SDIV] = 5;
  table[OP_MOD] = 5;
  table[OP_SMOD] = 5;
  table[OP_ADDMOD] = 8;
  table[OP_MULMOD] = 8;
  table[OP_SIGNEXTEND] = 5;

  // Comparison opcodes
  table[OP_LT] = 3;
  table[OP_GT] = 3;
  table[OP_SLT] = 3;
  table[OP_SGT] = 3;
  table[OP_EQ] = 3;
  table[OP_ISZERO] = 3;

  // Bitwise opcodes
  table[OP_AND] = 3;
  table[OP_OR] = 3;
  table[OP_XOR] = 3;
  table[OP_NOT] = 3;
  table[OP_BYTE] = 3;
  table[OP_SHL] = 3;
  table[OP_SHR] = 3;
  table[OP_SAR] = 3;

  // Keccak256 - base cost only, word cost is dynamic
  table[OP_KECCAK256] = 30;

  // Memory opcodes - static portion only
  table[OP_MLOAD] = 3;
  table[OP_MSTORE] = 3;
  table[OP_MSTORE8] = 3;
  table[OP_MSIZE] = 2;

  // Context opcodes
  table[OP_ADDRESS] = 2;
  table[OP_ORIGIN] = 2;
  table[OP_CALLER] = 2;
  table[OP_CALLVALUE] = 2;
  table[OP_CALLDATALOAD] = 3;
  table[OP_CALLDATASIZE] = 2;
  table[OP_CALLDATACOPY] = 3;
  table[OP_CODESIZE] = 2;
  table[OP_CODECOPY] = 3;
  table[OP_GASPRICE] = 2;
  table[OP_RETURNDATASIZE] = 2;
  table[OP_RETURNDATACOPY] = 3;

  // Control flow opcodes
  table[OP_POP] = 2;
  table[OP_PC] = 2;
  table[OP_GAS] = 2;
  table[OP_JUMP] = 8;
  table[OP_JUMPI] = 10;
  table[OP_JUMPDEST] = 1;

  // Stack opcodes - PUSH0
  table[OP_PUSH0] = 2;

  // Stack opcodes - PUSH1-32
  table[OP_PUSH1] = 3;
  table[OP_PUSH2] = 3;
  table[OP_PUSH3] = 3;
  table[OP_PUSH4] = 3;
  table[OP_PUSH5] = 3;
  table[OP_PUSH6] = 3;
  table[OP_PUSH7] = 3;
  table[OP_PUSH8] = 3;
  table[OP_PUSH9] = 3;
  table[OP_PUSH10] = 3;
  table[OP_PUSH11] = 3;
  table[OP_PUSH12] = 3;
  table[OP_PUSH13] = 3;
  table[OP_PUSH14] = 3;
  table[OP_PUSH15] = 3;
  table[OP_PUSH16] = 3;
  table[OP_PUSH17] = 3;
  table[OP_PUSH18] = 3;
  table[OP_PUSH19] = 3;
  table[OP_PUSH20] = 3;
  table[OP_PUSH21] = 3;
  table[OP_PUSH22] = 3;
  table[OP_PUSH23] = 3;
  table[OP_PUSH24] = 3;
  table[OP_PUSH25] = 3;
  table[OP_PUSH26] = 3;
  table[OP_PUSH27] = 3;
  table[OP_PUSH28] = 3;
  table[OP_PUSH29] = 3;
  table[OP_PUSH30] = 3;
  table[OP_PUSH31] = 3;
  table[OP_PUSH32] = 3;

  // DUP opcodes
  table[OP_DUP1] = 3;
  table[OP_DUP2] = 3;
  table[OP_DUP3] = 3;
  table[OP_DUP4] = 3;
  table[OP_DUP5] = 3;
  table[OP_DUP6] = 3;
  table[OP_DUP7] = 3;
  table[OP_DUP8] = 3;
  table[OP_DUP9] = 3;
  table[OP_DUP10] = 3;
  table[OP_DUP11] = 3;
  table[OP_DUP12] = 3;
  table[OP_DUP13] = 3;
  table[OP_DUP14] = 3;
  table[OP_DUP15] = 3;
  table[OP_DUP16] = 3;

  // SWAP opcodes
  table[OP_SWAP1] = 3;
  table[OP_SWAP2] = 3;
  table[OP_SWAP3] = 3;
  table[OP_SWAP4] = 3;
  table[OP_SWAP5] = 3;
  table[OP_SWAP6] = 3;
  table[OP_SWAP7] = 3;
  table[OP_SWAP8] = 3;
  table[OP_SWAP9] = 3;
  table[OP_SWAP10] = 3;
  table[OP_SWAP11] = 3;
  table[OP_SWAP12] = 3;
  table[OP_SWAP13] = 3;
  table[OP_SWAP14] = 3;
  table[OP_SWAP15] = 3;
  table[OP_SWAP16] = 3;
}

static inline void gas_table_init_cancun(gas_table_t table) {
  gas_table_init_shanghai(table);
  // Cancun-specific changes go here
}

static inline void gas_table_init_prague(gas_table_t table) {
  gas_table_init_cancun(table);
  // Prague-specific changes go here
}

#endif // DIV0_EVM_GAS_STATIC_COSTS_H
