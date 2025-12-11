#ifndef DIV0_EVM_GAS_STATIC_COSTS_H
#define DIV0_EVM_GAS_STATIC_COSTS_H

#include <array>

#include "div0/evm/opcodes.h"

namespace div0::evm::gas {

namespace detail {

constexpr std::array<uint64_t, 256> create_shanghai_table() {
  std::array<uint64_t, 256> table{};

  // Arithmetic opcodes
  table[op::ADD] = 3;
  table[op::MUL] = 5;
  table[op::SUB] = 3;
  table[op::DIV] = 5;
  table[op::SDIV] = 5;
  table[op::MOD] = 5;
  table[op::SMOD] = 5;
  table[op::ADDMOD] = 8;
  table[op::MULMOD] = 8;
  table[op::SIGNEXTEND] = 5;

  // Comparison opcodes
  table[op::LT] = 3;
  table[op::GT] = 3;
  table[op::SLT] = 3;
  table[op::SGT] = 3;
  table[op::EQ] = 3;
  table[op::ISZERO] = 3;

  // Bitwise opcodes
  table[op::AND] = 3;
  table[op::OR] = 3;
  table[op::XOR] = 3;
  table[op::NOT] = 3;
  table[op::BYTE] = 3;
  table[op::SHL] = 3;
  table[op::SHR] = 3;
  table[op::SAR] = 3;

  // Keccak256 (SHA3) opcode - base cost only, word cost is dynamic
  table[op::KECCAK256] = 30;

  // Memory opcodes (static portion only - expansion cost calculated dynamically)
  table[op::MLOAD] = 3;
  table[op::MSTORE] = 3;
  table[op::MSTORE8] = 3;
  table[op::MSIZE] = 2;

  // Context opcodes
  table[op::CALLER] = 2;
  table[op::CALLDATALOAD] = 3;
  table[op::CALLDATASIZE] = 2;
  table[op::CALLDATACOPY] = 3;  // Base cost; word cost and memory expansion added dynamically

  // Control flow opcodes
  table[op::POP] = 2;
  table[op::PC] = 2;
  table[op::GAS] = 2;

  // Stack opcodes
  table[op::PUSH0] = 2;
  table[op::PUSH1] = 3;
  table[op::PUSH2] = 3;
  table[op::PUSH3] = 3;
  table[op::PUSH4] = 3;
  table[op::PUSH5] = 3;
  table[op::PUSH6] = 3;
  table[op::PUSH7] = 3;
  table[op::PUSH8] = 3;
  table[op::PUSH9] = 3;
  table[op::PUSH10] = 3;
  table[op::PUSH11] = 3;
  table[op::PUSH12] = 3;
  table[op::PUSH13] = 3;
  table[op::PUSH14] = 3;
  table[op::PUSH15] = 3;
  table[op::PUSH16] = 3;
  table[op::PUSH17] = 3;
  table[op::PUSH18] = 3;
  table[op::PUSH19] = 3;
  table[op::PUSH20] = 3;
  table[op::PUSH21] = 3;
  table[op::PUSH22] = 3;
  table[op::PUSH23] = 3;
  table[op::PUSH24] = 3;
  table[op::PUSH25] = 3;
  table[op::PUSH26] = 3;
  table[op::PUSH27] = 3;
  table[op::PUSH28] = 3;
  table[op::PUSH29] = 3;
  table[op::PUSH30] = 3;
  table[op::PUSH31] = 3;
  table[op::PUSH32] = 3;
  table[op::DUP1] = 3;
  table[op::DUP2] = 3;
  table[op::DUP3] = 3;
  table[op::DUP4] = 3;
  table[op::DUP5] = 3;
  table[op::DUP6] = 3;
  table[op::DUP7] = 3;
  table[op::DUP8] = 3;
  table[op::DUP9] = 3;
  table[op::DUP10] = 3;
  table[op::DUP11] = 3;
  table[op::DUP12] = 3;
  table[op::DUP13] = 3;
  table[op::DUP14] = 3;
  table[op::DUP15] = 3;
  table[op::DUP16] = 3;
  table[op::SWAP1] = 3;
  table[op::SWAP2] = 3;
  table[op::SWAP3] = 3;
  table[op::SWAP4] = 3;
  table[op::SWAP5] = 3;
  table[op::SWAP6] = 3;
  table[op::SWAP7] = 3;
  table[op::SWAP8] = 3;
  table[op::SWAP9] = 3;
  table[op::SWAP10] = 3;
  table[op::SWAP11] = 3;
  table[op::SWAP12] = 3;
  table[op::SWAP13] = 3;
  table[op::SWAP14] = 3;
  table[op::SWAP15] = 3;
  table[op::SWAP16] = 3;

  return table;
}

constexpr std::array<uint64_t, 256> create_cancun_table() {
  auto table = create_shanghai_table();
  // Cancun-specific changes go here
  return table;
}

constexpr std::array<uint64_t, 256> create_prague_table() {
  auto table = create_cancun_table();
  // Prague-specific changes go here
  return table;
}

}  // namespace detail

struct StaticCosts {
  static constexpr std::array<uint64_t, 256> SHANGHAI = detail::create_shanghai_table();
  static constexpr std::array<uint64_t, 256> CANCUN = detail::create_cancun_table();
  static constexpr std::array<uint64_t, 256> PRAGUE = detail::create_prague_table();
};

}  // namespace div0::evm::gas

#endif  // DIV0_EVM_GAS_STATIC_COSTS_H
