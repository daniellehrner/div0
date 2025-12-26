#ifndef DIV0_EVM_OPCODES_H
#define DIV0_EVM_OPCODES_H

/// EVM Opcodes
/// See https://www.evm.codes/ for reference

// Stop and Arithmetic Operations
constexpr auto OP_STOP = 0x00;
constexpr auto OP_ADD = 0x01;
constexpr auto OP_MUL = 0x02;
constexpr auto OP_SUB = 0x03;
constexpr auto OP_DIV = 0x04;
constexpr auto OP_SDIV = 0x05;
constexpr auto OP_MOD = 0x06;
constexpr auto OP_SMOD = 0x07;
constexpr auto OP_ADDMOD = 0x08;
constexpr auto OP_MULMOD = 0x09;
constexpr auto OP_EXP = 0x0A;
constexpr auto OP_SIGNEXTEND = 0x0B;

// Comparison & Bitwise Logic Operations
constexpr auto OP_LT = 0x10;
constexpr auto OP_GT = 0x11;
constexpr auto OP_SLT = 0x12;
constexpr auto OP_SGT = 0x13;
constexpr auto OP_EQ = 0x14;
constexpr auto OP_ISZERO = 0x15;
constexpr auto OP_AND = 0x16;
constexpr auto OP_OR = 0x17;
constexpr auto OP_XOR = 0x18;
constexpr auto OP_NOT = 0x19;
constexpr auto OP_BYTE = 0x1A;
constexpr auto OP_SHL = 0x1B;
constexpr auto OP_SHR = 0x1C;
constexpr auto OP_SAR = 0x1D;

// SHA3
constexpr auto OP_KECCAK256 = 0x20;

// Environmental Information
constexpr auto OP_ADDRESS = 0x30;
constexpr auto OP_BALANCE = 0x31;
constexpr auto OP_ORIGIN = 0x32;
constexpr auto OP_CALLER = 0x33;
constexpr auto OP_CALLVALUE = 0x34;
constexpr auto OP_CALLDATALOAD = 0x35;
constexpr auto OP_CALLDATASIZE = 0x36;
constexpr auto OP_CALLDATACOPY = 0x37;
constexpr auto OP_CODESIZE = 0x38;
constexpr auto OP_CODECOPY = 0x39;
constexpr auto OP_GASPRICE = 0x3A;
constexpr auto OP_EXTCODESIZE = 0x3B;
constexpr auto OP_EXTCODECOPY = 0x3C;
constexpr auto OP_RETURNDATASIZE = 0x3D;
constexpr auto OP_RETURNDATACOPY = 0x3E;
constexpr auto OP_EXTCODEHASH = 0x3F;

// Block Information
constexpr auto OP_BLOCKHASH = 0x40;
constexpr auto OP_COINBASE = 0x41;
constexpr auto OP_TIMESTAMP = 0x42;
constexpr auto OP_NUMBER = 0x43;
constexpr auto OP_PREVRANDAO = 0x44;
constexpr auto OP_GASLIMIT = 0x45;
constexpr auto OP_CHAINID = 0x46;
constexpr auto OP_SELFBALANCE = 0x47;
constexpr auto OP_BASEFEE = 0x48;
constexpr auto OP_BLOBHASH = 0x49;
constexpr auto OP_BLOBBASEFEE = 0x4A;

// Stack, Memory, Storage and Flow Operations
constexpr auto OP_POP = 0x50;
constexpr auto OP_MLOAD = 0x51;
constexpr auto OP_MSTORE = 0x52;
constexpr auto OP_MSTORE8 = 0x53;
constexpr auto OP_SLOAD = 0x54;
constexpr auto OP_SSTORE = 0x55;
constexpr auto OP_JUMP = 0x56;
constexpr auto OP_JUMPI = 0x57;
constexpr auto OP_PC = 0x58;
constexpr auto OP_MSIZE = 0x59;
constexpr auto OP_GAS = 0x5A;
constexpr auto OP_JUMPDEST = 0x5B;
constexpr auto OP_TLOAD = 0x5C;
constexpr auto OP_TSTORE = 0x5D;
constexpr auto OP_MCOPY = 0x5E;
constexpr auto OP_PUSH0 = 0x5F;

// Push Operations
constexpr auto OP_PUSH1 = 0x60;
constexpr auto OP_PUSH2 = 0x61;
constexpr auto OP_PUSH3 = 0x62;
constexpr auto OP_PUSH4 = 0x63;
constexpr auto OP_PUSH5 = 0x64;
constexpr auto OP_PUSH6 = 0x65;
constexpr auto OP_PUSH7 = 0x66;
constexpr auto OP_PUSH8 = 0x67;
constexpr auto OP_PUSH9 = 0x68;
constexpr auto OP_PUSH10 = 0x69;
constexpr auto OP_PUSH11 = 0x6A;
constexpr auto OP_PUSH12 = 0x6B;
constexpr auto OP_PUSH13 = 0x6C;
constexpr auto OP_PUSH14 = 0x6D;
constexpr auto OP_PUSH15 = 0x6E;
constexpr auto OP_PUSH16 = 0x6F;
constexpr auto OP_PUSH17 = 0x70;
constexpr auto OP_PUSH18 = 0x71;
constexpr auto OP_PUSH19 = 0x72;
constexpr auto OP_PUSH20 = 0x73;
constexpr auto OP_PUSH21 = 0x74;
constexpr auto OP_PUSH22 = 0x75;
constexpr auto OP_PUSH23 = 0x76;
constexpr auto OP_PUSH24 = 0x77;
constexpr auto OP_PUSH25 = 0x78;
constexpr auto OP_PUSH26 = 0x79;
constexpr auto OP_PUSH27 = 0x7A;
constexpr auto OP_PUSH28 = 0x7B;
constexpr auto OP_PUSH29 = 0x7C;
constexpr auto OP_PUSH30 = 0x7D;
constexpr auto OP_PUSH31 = 0x7E;
constexpr auto OP_PUSH32 = 0x7F;

// Duplication Operations
constexpr auto OP_DUP1 = 0x80;
constexpr auto OP_DUP2 = 0x81;
constexpr auto OP_DUP3 = 0x82;
constexpr auto OP_DUP4 = 0x83;
constexpr auto OP_DUP5 = 0x84;
constexpr auto OP_DUP6 = 0x85;
constexpr auto OP_DUP7 = 0x86;
constexpr auto OP_DUP8 = 0x87;
constexpr auto OP_DUP9 = 0x88;
constexpr auto OP_DUP10 = 0x89;
constexpr auto OP_DUP11 = 0x8A;
constexpr auto OP_DUP12 = 0x8B;
constexpr auto OP_DUP13 = 0x8C;
constexpr auto OP_DUP14 = 0x8D;
constexpr auto OP_DUP15 = 0x8E;
constexpr auto OP_DUP16 = 0x8F;

// Exchange Operations
constexpr auto OP_SWAP1 = 0x90;
constexpr auto OP_SWAP2 = 0x91;
constexpr auto OP_SWAP3 = 0x92;
constexpr auto OP_SWAP4 = 0x93;
constexpr auto OP_SWAP5 = 0x94;
constexpr auto OP_SWAP6 = 0x95;
constexpr auto OP_SWAP7 = 0x96;
constexpr auto OP_SWAP8 = 0x97;
constexpr auto OP_SWAP9 = 0x98;
constexpr auto OP_SWAP10 = 0x99;
constexpr auto OP_SWAP11 = 0x9A;
constexpr auto OP_SWAP12 = 0x9B;
constexpr auto OP_SWAP13 = 0x9C;
constexpr auto OP_SWAP14 = 0x9D;
constexpr auto OP_SWAP15 = 0x9E;
constexpr auto OP_SWAP16 = 0x9F;

// Logging Operations
constexpr auto OP_LOG0 = 0xA0;
constexpr auto OP_LOG1 = 0xA1;
constexpr auto OP_LOG2 = 0xA2;
constexpr auto OP_LOG3 = 0xA3;
constexpr auto OP_LOG4 = 0xA4;

// System Operations
constexpr auto OP_CREATE = 0xF0;
constexpr auto OP_CALL = 0xF1;
constexpr auto OP_CALLCODE = 0xF2;
constexpr auto OP_RETURN = 0xF3;
constexpr auto OP_DELEGATECALL = 0xF4;
constexpr auto OP_CREATE2 = 0xF5;
constexpr auto OP_STATICCALL = 0xFA;
constexpr auto OP_REVERT = 0xFD;
constexpr auto OP_INVALID = 0xFE;
constexpr auto OP_SELFDESTRUCT = 0xFF;

#endif // DIV0_EVM_OPCODES_H