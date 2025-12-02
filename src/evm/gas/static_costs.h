#ifndef DIV0_EVM_GAS_STATIC_COSTS_H
#define DIV0_EVM_GAS_STATIC_COSTS_H

#include <array>

#include "../opcodes.h"

namespace div0::evm::gas {

namespace detail {

constexpr std::array<uint64_t, 256> create_shanghai_table() {
  std::array<uint64_t, 256> table{};

  // Arithmetic opcodes
  table[op(Opcode::Add)] = 3;
  table[op(Opcode::Mul)] = 5;
  table[op(Opcode::Sub)] = 3;
  table[op(Opcode::Div)] = 5;
  table[op(Opcode::SDiv)] = 5;
  table[op(Opcode::Mod)] = 5;
  table[op(Opcode::SMod)] = 5;
  table[op(Opcode::AddMod)] = 8;
  table[op(Opcode::MulMod)] = 8;
  table[op(Opcode::SignExtend)] = 5;

  // Comparison opcodes
  table[op(Opcode::Lt)] = 3;
  table[op(Opcode::Gt)] = 3;
  table[op(Opcode::Eq)] = 3;
  table[op(Opcode::IsZero)] = 3;

  // Keccak256 (SHA3) opcode - base cost only, word cost is dynamic
  table[op(Opcode::Sha3)] = 30;

  // Memory opcodes (static portion only - expansion cost calculated dynamically)
  table[op(Opcode::MLoad)] = 3;
  table[op(Opcode::MStore)] = 3;
  table[op(Opcode::MStore8)] = 3;
  table[op(Opcode::MSize)] = 2;

  // Stack opcodes
  table[op(Opcode::Push0)] = 2;
  table[op(Opcode::Push1)] = 3;
  table[op(Opcode::Push2)] = 3;
  table[op(Opcode::Push3)] = 3;
  table[op(Opcode::Push4)] = 3;
  table[op(Opcode::Push5)] = 3;
  table[op(Opcode::Push6)] = 3;
  table[op(Opcode::Push7)] = 3;
  table[op(Opcode::Push8)] = 3;
  table[op(Opcode::Push9)] = 3;
  table[op(Opcode::Push10)] = 3;
  table[op(Opcode::Push11)] = 3;
  table[op(Opcode::Push12)] = 3;
  table[op(Opcode::Push13)] = 3;
  table[op(Opcode::Push14)] = 3;
  table[op(Opcode::Push15)] = 3;
  table[op(Opcode::Push16)] = 3;
  table[op(Opcode::Push17)] = 3;
  table[op(Opcode::Push18)] = 3;
  table[op(Opcode::Push19)] = 3;
  table[op(Opcode::Push20)] = 3;
  table[op(Opcode::Push21)] = 3;
  table[op(Opcode::Push22)] = 3;
  table[op(Opcode::Push23)] = 3;
  table[op(Opcode::Push24)] = 3;
  table[op(Opcode::Push25)] = 3;
  table[op(Opcode::Push26)] = 3;
  table[op(Opcode::Push27)] = 3;
  table[op(Opcode::Push28)] = 3;
  table[op(Opcode::Push29)] = 3;
  table[op(Opcode::Push30)] = 3;
  table[op(Opcode::Push31)] = 3;
  table[op(Opcode::Push32)] = 3;
  table[op(Opcode::Dup1)] = 3;
  table[op(Opcode::Dup2)] = 3;
  table[op(Opcode::Dup3)] = 3;
  table[op(Opcode::Dup4)] = 3;
  table[op(Opcode::Dup5)] = 3;
  table[op(Opcode::Dup6)] = 3;
  table[op(Opcode::Dup7)] = 3;
  table[op(Opcode::Dup8)] = 3;
  table[op(Opcode::Dup9)] = 3;
  table[op(Opcode::Dup10)] = 3;
  table[op(Opcode::Dup11)] = 3;
  table[op(Opcode::Dup12)] = 3;
  table[op(Opcode::Dup13)] = 3;
  table[op(Opcode::Dup14)] = 3;
  table[op(Opcode::Dup15)] = 3;
  table[op(Opcode::Dup16)] = 3;
  table[op(Opcode::Swap1)] = 3;
  table[op(Opcode::Swap2)] = 3;
  table[op(Opcode::Swap3)] = 3;
  table[op(Opcode::Swap4)] = 3;
  table[op(Opcode::Swap5)] = 3;
  table[op(Opcode::Swap6)] = 3;
  table[op(Opcode::Swap7)] = 3;
  table[op(Opcode::Swap8)] = 3;
  table[op(Opcode::Swap9)] = 3;
  table[op(Opcode::Swap10)] = 3;
  table[op(Opcode::Swap11)] = 3;
  table[op(Opcode::Swap12)] = 3;
  table[op(Opcode::Swap13)] = 3;
  table[op(Opcode::Swap14)] = 3;
  table[op(Opcode::Swap15)] = 3;
  table[op(Opcode::Swap16)] = 3;

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
