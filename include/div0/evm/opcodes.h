#ifndef DIV0_EVM_OPCODES_H
#define DIV0_EVM_OPCODES_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace div0::evm {

/// Opcode definition containing both value and name
struct Opcode {
  uint8_t value;
  const char* name;

  constexpr operator uint8_t() const noexcept { return value; }
};

/// All 256 opcodes indexed by value for O(1) lookup
inline constexpr std::array<Opcode, 256> OPCODES = {{
    // 0x00 - 0x0F: Stop and Arithmetic
    {0x00, "STOP"},
    {0x01, "ADD"},
    {0x02, "MUL"},
    {0x03, "SUB"},
    {0x04, "DIV"},
    {0x05, "SDIV"},
    {0x06, "MOD"},
    {0x07, "SMOD"},
    {0x08, "ADDMOD"},
    {0x09, "MULMOD"},
    {0x0A, "EXP"},
    {0x0B, "SIGNEXTEND"},
    {0x0C, nullptr},
    {0x0D, nullptr},
    {0x0E, nullptr},
    {0x0F, nullptr},

    // 0x10 - 0x1F: Comparison and Bitwise
    {0x10, "LT"},
    {0x11, "GT"},
    {0x12, "SLT"},
    {0x13, "SGT"},
    {0x14, "EQ"},
    {0x15, "ISZERO"},
    {0x16, "AND"},
    {0x17, "OR"},
    {0x18, "XOR"},
    {0x19, "NOT"},
    {0x1A, "BYTE"},
    {0x1B, "SHL"},
    {0x1C, "SHR"},
    {0x1D, "SAR"},
    {0x1E, nullptr},
    {0x1F, nullptr},

    // 0x20 - 0x2F: Keccak256
    {0x20, "KECCAK256"},
    {0x21, nullptr},
    {0x22, nullptr},
    {0x23, nullptr},
    {0x24, nullptr},
    {0x25, nullptr},
    {0x26, nullptr},
    {0x27, nullptr},
    {0x28, nullptr},
    {0x29, nullptr},
    {0x2A, nullptr},
    {0x2B, nullptr},
    {0x2C, nullptr},
    {0x2D, nullptr},
    {0x2E, nullptr},
    {0x2F, nullptr},

    // 0x30 - 0x3F: Environmental Information
    {0x30, "ADDRESS"},
    {0x31, "BALANCE"},
    {0x32, "ORIGIN"},
    {0x33, "CALLER"},
    {0x34, "CALLVALUE"},
    {0x35, "CALLDATALOAD"},
    {0x36, "CALLDATASIZE"},
    {0x37, "CALLDATACOPY"},
    {0x38, "CODESIZE"},
    {0x39, "CODECOPY"},
    {0x3A, "GASPRICE"},
    {0x3B, "EXTCODESIZE"},
    {0x3C, "EXTCODECOPY"},
    {0x3D, "RETURNDATASIZE"},
    {0x3E, "RETURNDATACOPY"},
    {0x3F, "EXTCODEHASH"},

    // 0x40 - 0x4F: Block Information
    {0x40, "BLOCKHASH"},
    {0x41, "COINBASE"},
    {0x42, "TIMESTAMP"},
    {0x43, "NUMBER"},
    {0x44, "PREVRANDAO"},
    {0x45, "GASLIMIT"},
    {0x46, "CHAINID"},
    {0x47, "SELFBALANCE"},
    {0x48, "BASEFEE"},
    {0x49, "BLOBHASH"},
    {0x4A, "BLOBBASEFEE"},
    {0x4B, nullptr},
    {0x4C, nullptr},
    {0x4D, nullptr},
    {0x4E, nullptr},
    {0x4F, nullptr},

    // 0x50 - 0x5F: Stack, Memory, Storage, Flow
    {0x50, "POP"},
    {0x51, "MLOAD"},
    {0x52, "MSTORE"},
    {0x53, "MSTORE8"},
    {0x54, "SLOAD"},
    {0x55, "SSTORE"},
    {0x56, "JUMP"},
    {0x57, "JUMPI"},
    {0x58, "PC"},
    {0x59, "MSIZE"},
    {0x5A, "GAS"},
    {0x5B, "JUMPDEST"},
    {0x5C, "TLOAD"},
    {0x5D, "TSTORE"},
    {0x5E, "MCOPY"},
    {0x5F, "PUSH0"},

    // 0x60 - 0x6F: PUSH1 - PUSH16
    {0x60, "PUSH1"},
    {0x61, "PUSH2"},
    {0x62, "PUSH3"},
    {0x63, "PUSH4"},
    {0x64, "PUSH5"},
    {0x65, "PUSH6"},
    {0x66, "PUSH7"},
    {0x67, "PUSH8"},
    {0x68, "PUSH9"},
    {0x69, "PUSH10"},
    {0x6A, "PUSH11"},
    {0x6B, "PUSH12"},
    {0x6C, "PUSH13"},
    {0x6D, "PUSH14"},
    {0x6E, "PUSH15"},
    {0x6F, "PUSH16"},

    // 0x70 - 0x7F: PUSH17 - PUSH32
    {0x70, "PUSH17"},
    {0x71, "PUSH18"},
    {0x72, "PUSH19"},
    {0x73, "PUSH20"},
    {0x74, "PUSH21"},
    {0x75, "PUSH22"},
    {0x76, "PUSH23"},
    {0x77, "PUSH24"},
    {0x78, "PUSH25"},
    {0x79, "PUSH26"},
    {0x7A, "PUSH27"},
    {0x7B, "PUSH28"},
    {0x7C, "PUSH29"},
    {0x7D, "PUSH30"},
    {0x7E, "PUSH31"},
    {0x7F, "PUSH32"},

    // 0x80 - 0x8F: DUP1 - DUP16
    {0x80, "DUP1"},
    {0x81, "DUP2"},
    {0x82, "DUP3"},
    {0x83, "DUP4"},
    {0x84, "DUP5"},
    {0x85, "DUP6"},
    {0x86, "DUP7"},
    {0x87, "DUP8"},
    {0x88, "DUP9"},
    {0x89, "DUP10"},
    {0x8A, "DUP11"},
    {0x8B, "DUP12"},
    {0x8C, "DUP13"},
    {0x8D, "DUP14"},
    {0x8E, "DUP15"},
    {0x8F, "DUP16"},

    // 0x90 - 0x9F: SWAP1 - SWAP16
    {0x90, "SWAP1"},
    {0x91, "SWAP2"},
    {0x92, "SWAP3"},
    {0x93, "SWAP4"},
    {0x94, "SWAP5"},
    {0x95, "SWAP6"},
    {0x96, "SWAP7"},
    {0x97, "SWAP8"},
    {0x98, "SWAP9"},
    {0x99, "SWAP10"},
    {0x9A, "SWAP11"},
    {0x9B, "SWAP12"},
    {0x9C, "SWAP13"},
    {0x9D, "SWAP14"},
    {0x9E, "SWAP15"},
    {0x9F, "SWAP16"},

    // 0xA0 - 0xAF: LOG0 - LOG4
    {0xA0, "LOG0"},
    {0xA1, "LOG1"},
    {0xA2, "LOG2"},
    {0xA3, "LOG3"},
    {0xA4, "LOG4"},
    {0xA5, nullptr},
    {0xA6, nullptr},
    {0xA7, nullptr},
    {0xA8, nullptr},
    {0xA9, nullptr},
    {0xAA, nullptr},
    {0xAB, nullptr},
    {0xAC, nullptr},
    {0xAD, nullptr},
    {0xAE, nullptr},
    {0xAF, nullptr},

    // 0xB0 - 0xBF: Undefined
    {0xB0, nullptr},
    {0xB1, nullptr},
    {0xB2, nullptr},
    {0xB3, nullptr},
    {0xB4, nullptr},
    {0xB5, nullptr},
    {0xB6, nullptr},
    {0xB7, nullptr},
    {0xB8, nullptr},
    {0xB9, nullptr},
    {0xBA, nullptr},
    {0xBB, nullptr},
    {0xBC, nullptr},
    {0xBD, nullptr},
    {0xBE, nullptr},
    {0xBF, nullptr},

    // 0xC0 - 0xCF: Undefined
    {0xC0, nullptr},
    {0xC1, nullptr},
    {0xC2, nullptr},
    {0xC3, nullptr},
    {0xC4, nullptr},
    {0xC5, nullptr},
    {0xC6, nullptr},
    {0xC7, nullptr},
    {0xC8, nullptr},
    {0xC9, nullptr},
    {0xCA, nullptr},
    {0xCB, nullptr},
    {0xCC, nullptr},
    {0xCD, nullptr},
    {0xCE, nullptr},
    {0xCF, nullptr},

    // 0xD0 - 0xDF: Undefined
    {0xD0, nullptr},
    {0xD1, nullptr},
    {0xD2, nullptr},
    {0xD3, nullptr},
    {0xD4, nullptr},
    {0xD5, nullptr},
    {0xD6, nullptr},
    {0xD7, nullptr},
    {0xD8, nullptr},
    {0xD9, nullptr},
    {0xDA, nullptr},
    {0xDB, nullptr},
    {0xDC, nullptr},
    {0xDD, nullptr},
    {0xDE, nullptr},
    {0xDF, nullptr},

    // 0xE0 - 0xEF: Undefined
    {0xE0, nullptr},
    {0xE1, nullptr},
    {0xE2, nullptr},
    {0xE3, nullptr},
    {0xE4, nullptr},
    {0xE5, nullptr},
    {0xE6, nullptr},
    {0xE7, nullptr},
    {0xE8, nullptr},
    {0xE9, nullptr},
    {0xEA, nullptr},
    {0xEB, nullptr},
    {0xEC, nullptr},
    {0xED, nullptr},
    {0xEE, nullptr},
    {0xEF, nullptr},

    // 0xF0 - 0xFF: System Operations
    {0xF0, "CREATE"},
    {0xF1, "CALL"},
    {0xF2, "CALLCODE"},
    {0xF3, "RETURN"},
    {0xF4, "DELEGATECALL"},
    {0xF5, "CREATE2"},
    {0xF6, nullptr},
    {0xF7, nullptr},
    {0xF8, nullptr},
    {0xF9, nullptr},
    {0xFA, "STATICCALL"},
    {0xFB, nullptr},
    {0xFC, nullptr},
    {0xFD, "REVERT"},
    {0xFE, "INVALID"},
    {0xFF, "SELFDESTRUCT"},
}};

// Named constants for convenient access
namespace op {
inline constexpr const Opcode& STOP = OPCODES[0x00];
inline constexpr const Opcode& ADD = OPCODES[0x01];
inline constexpr const Opcode& MUL = OPCODES[0x02];
inline constexpr const Opcode& SUB = OPCODES[0x03];
inline constexpr const Opcode& DIV = OPCODES[0x04];
inline constexpr const Opcode& SDIV = OPCODES[0x05];
inline constexpr const Opcode& MOD = OPCODES[0x06];
inline constexpr const Opcode& SMOD = OPCODES[0x07];
inline constexpr const Opcode& ADDMOD = OPCODES[0x08];
inline constexpr const Opcode& MULMOD = OPCODES[0x09];
inline constexpr const Opcode& EXP = OPCODES[0x0A];
inline constexpr const Opcode& SIGNEXTEND = OPCODES[0x0B];
inline constexpr const Opcode& LT = OPCODES[0x10];
inline constexpr const Opcode& GT = OPCODES[0x11];
inline constexpr const Opcode& SLT = OPCODES[0x12];
inline constexpr const Opcode& SGT = OPCODES[0x13];
inline constexpr const Opcode& EQ = OPCODES[0x14];
inline constexpr const Opcode& ISZERO = OPCODES[0x15];
inline constexpr const Opcode& AND = OPCODES[0x16];
inline constexpr const Opcode& OR = OPCODES[0x17];
inline constexpr const Opcode& XOR = OPCODES[0x18];
inline constexpr const Opcode& NOT = OPCODES[0x19];
inline constexpr const Opcode& BYTE = OPCODES[0x1A];
inline constexpr const Opcode& SHL = OPCODES[0x1B];
inline constexpr const Opcode& SHR = OPCODES[0x1C];
inline constexpr const Opcode& SAR = OPCODES[0x1D];
inline constexpr const Opcode& KECCAK256 = OPCODES[0x20];
inline constexpr const Opcode& SHA3 = OPCODES[0x20];  // Alias
inline constexpr const Opcode& ADDRESS = OPCODES[0x30];
inline constexpr const Opcode& BALANCE = OPCODES[0x31];
inline constexpr const Opcode& ORIGIN = OPCODES[0x32];
inline constexpr const Opcode& CALLER = OPCODES[0x33];
inline constexpr const Opcode& CALLVALUE = OPCODES[0x34];
inline constexpr const Opcode& CALLDATALOAD = OPCODES[0x35];
inline constexpr const Opcode& CALLDATASIZE = OPCODES[0x36];
inline constexpr const Opcode& CALLDATACOPY = OPCODES[0x37];
inline constexpr const Opcode& CODESIZE = OPCODES[0x38];
inline constexpr const Opcode& CODECOPY = OPCODES[0x39];
inline constexpr const Opcode& GASPRICE = OPCODES[0x3A];
inline constexpr const Opcode& EXTCODESIZE = OPCODES[0x3B];
inline constexpr const Opcode& EXTCODECOPY = OPCODES[0x3C];
inline constexpr const Opcode& RETURNDATASIZE = OPCODES[0x3D];
inline constexpr const Opcode& RETURNDATACOPY = OPCODES[0x3E];
inline constexpr const Opcode& EXTCODEHASH = OPCODES[0x3F];
inline constexpr const Opcode& BLOCKHASH = OPCODES[0x40];
inline constexpr const Opcode& COINBASE = OPCODES[0x41];
inline constexpr const Opcode& TIMESTAMP = OPCODES[0x42];
inline constexpr const Opcode& NUMBER = OPCODES[0x43];
inline constexpr const Opcode& PREVRANDAO = OPCODES[0x44];
inline constexpr const Opcode& GASLIMIT = OPCODES[0x45];
inline constexpr const Opcode& CHAINID = OPCODES[0x46];
inline constexpr const Opcode& SELFBALANCE = OPCODES[0x47];
inline constexpr const Opcode& BASEFEE = OPCODES[0x48];
inline constexpr const Opcode& BLOBHASH = OPCODES[0x49];
inline constexpr const Opcode& BLOBBASEFEE = OPCODES[0x4A];
inline constexpr const Opcode& POP = OPCODES[0x50];
inline constexpr const Opcode& MLOAD = OPCODES[0x51];
inline constexpr const Opcode& MSTORE = OPCODES[0x52];
inline constexpr const Opcode& MSTORE8 = OPCODES[0x53];
inline constexpr const Opcode& SLOAD = OPCODES[0x54];
inline constexpr const Opcode& SSTORE = OPCODES[0x55];
inline constexpr const Opcode& JUMP = OPCODES[0x56];
inline constexpr const Opcode& JUMPI = OPCODES[0x57];
inline constexpr const Opcode& PC = OPCODES[0x58];
inline constexpr const Opcode& MSIZE = OPCODES[0x59];
inline constexpr const Opcode& GAS = OPCODES[0x5A];
inline constexpr const Opcode& JUMPDEST = OPCODES[0x5B];
inline constexpr const Opcode& TLOAD = OPCODES[0x5C];
inline constexpr const Opcode& TSTORE = OPCODES[0x5D];
inline constexpr const Opcode& MCOPY = OPCODES[0x5E];
inline constexpr const Opcode& PUSH0 = OPCODES[0x5F];
inline constexpr const Opcode& PUSH1 = OPCODES[0x60];
inline constexpr const Opcode& PUSH2 = OPCODES[0x61];
inline constexpr const Opcode& PUSH3 = OPCODES[0x62];
inline constexpr const Opcode& PUSH4 = OPCODES[0x63];
inline constexpr const Opcode& PUSH5 = OPCODES[0x64];
inline constexpr const Opcode& PUSH6 = OPCODES[0x65];
inline constexpr const Opcode& PUSH7 = OPCODES[0x66];
inline constexpr const Opcode& PUSH8 = OPCODES[0x67];
inline constexpr const Opcode& PUSH9 = OPCODES[0x68];
inline constexpr const Opcode& PUSH10 = OPCODES[0x69];
inline constexpr const Opcode& PUSH11 = OPCODES[0x6A];
inline constexpr const Opcode& PUSH12 = OPCODES[0x6B];
inline constexpr const Opcode& PUSH13 = OPCODES[0x6C];
inline constexpr const Opcode& PUSH14 = OPCODES[0x6D];
inline constexpr const Opcode& PUSH15 = OPCODES[0x6E];
inline constexpr const Opcode& PUSH16 = OPCODES[0x6F];
inline constexpr const Opcode& PUSH17 = OPCODES[0x70];
inline constexpr const Opcode& PUSH18 = OPCODES[0x71];
inline constexpr const Opcode& PUSH19 = OPCODES[0x72];
inline constexpr const Opcode& PUSH20 = OPCODES[0x73];
inline constexpr const Opcode& PUSH21 = OPCODES[0x74];
inline constexpr const Opcode& PUSH22 = OPCODES[0x75];
inline constexpr const Opcode& PUSH23 = OPCODES[0x76];
inline constexpr const Opcode& PUSH24 = OPCODES[0x77];
inline constexpr const Opcode& PUSH25 = OPCODES[0x78];
inline constexpr const Opcode& PUSH26 = OPCODES[0x79];
inline constexpr const Opcode& PUSH27 = OPCODES[0x7A];
inline constexpr const Opcode& PUSH28 = OPCODES[0x7B];
inline constexpr const Opcode& PUSH29 = OPCODES[0x7C];
inline constexpr const Opcode& PUSH30 = OPCODES[0x7D];
inline constexpr const Opcode& PUSH31 = OPCODES[0x7E];
inline constexpr const Opcode& PUSH32 = OPCODES[0x7F];
inline constexpr const Opcode& DUP1 = OPCODES[0x80];
inline constexpr const Opcode& DUP2 = OPCODES[0x81];
inline constexpr const Opcode& DUP3 = OPCODES[0x82];
inline constexpr const Opcode& DUP4 = OPCODES[0x83];
inline constexpr const Opcode& DUP5 = OPCODES[0x84];
inline constexpr const Opcode& DUP6 = OPCODES[0x85];
inline constexpr const Opcode& DUP7 = OPCODES[0x86];
inline constexpr const Opcode& DUP8 = OPCODES[0x87];
inline constexpr const Opcode& DUP9 = OPCODES[0x88];
inline constexpr const Opcode& DUP10 = OPCODES[0x89];
inline constexpr const Opcode& DUP11 = OPCODES[0x8A];
inline constexpr const Opcode& DUP12 = OPCODES[0x8B];
inline constexpr const Opcode& DUP13 = OPCODES[0x8C];
inline constexpr const Opcode& DUP14 = OPCODES[0x8D];
inline constexpr const Opcode& DUP15 = OPCODES[0x8E];
inline constexpr const Opcode& DUP16 = OPCODES[0x8F];
inline constexpr const Opcode& SWAP1 = OPCODES[0x90];
inline constexpr const Opcode& SWAP2 = OPCODES[0x91];
inline constexpr const Opcode& SWAP3 = OPCODES[0x92];
inline constexpr const Opcode& SWAP4 = OPCODES[0x93];
inline constexpr const Opcode& SWAP5 = OPCODES[0x94];
inline constexpr const Opcode& SWAP6 = OPCODES[0x95];
inline constexpr const Opcode& SWAP7 = OPCODES[0x96];
inline constexpr const Opcode& SWAP8 = OPCODES[0x97];
inline constexpr const Opcode& SWAP9 = OPCODES[0x98];
inline constexpr const Opcode& SWAP10 = OPCODES[0x99];
inline constexpr const Opcode& SWAP11 = OPCODES[0x9A];
inline constexpr const Opcode& SWAP12 = OPCODES[0x9B];
inline constexpr const Opcode& SWAP13 = OPCODES[0x9C];
inline constexpr const Opcode& SWAP14 = OPCODES[0x9D];
inline constexpr const Opcode& SWAP15 = OPCODES[0x9E];
inline constexpr const Opcode& SWAP16 = OPCODES[0x9F];
inline constexpr const Opcode& LOG0 = OPCODES[0xA0];
inline constexpr const Opcode& LOG1 = OPCODES[0xA1];
inline constexpr const Opcode& LOG2 = OPCODES[0xA2];
inline constexpr const Opcode& LOG3 = OPCODES[0xA3];
inline constexpr const Opcode& LOG4 = OPCODES[0xA4];
inline constexpr const Opcode& CREATE = OPCODES[0xF0];
inline constexpr const Opcode& CALL = OPCODES[0xF1];
inline constexpr const Opcode& CALLCODE = OPCODES[0xF2];
inline constexpr const Opcode& RETURN = OPCODES[0xF3];
inline constexpr const Opcode& DELEGATECALL = OPCODES[0xF4];
inline constexpr const Opcode& CREATE2 = OPCODES[0xF5];
inline constexpr const Opcode& STATICCALL = OPCODES[0xFA];
inline constexpr const Opcode& REVERT = OPCODES[0xFD];
inline constexpr const Opcode& INVALID = OPCODES[0xFE];
inline constexpr const Opcode& SELFDESTRUCT = OPCODES[0xFF];
}  // namespace op

/// Get the human-readable name for an opcode byte (O(1) lookup)
constexpr const char* opcode_name(uint8_t opcode) {
  const char* name = OPCODES[opcode].name;
  return name != nullptr ? name : "INVALID";
}

}  // namespace div0::evm

#endif  // DIV0_EVM_OPCODES_H
