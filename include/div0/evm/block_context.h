#ifndef DIV0_EVM_BLOCK_CONTEXT_H
#define DIV0_EVM_BLOCK_CONTEXT_H

#include <cstdint>
#include <functional>

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief Callback for lazy BLOCKHASH lookup.
 *
 * Returns the hash for the given block number, or zero if outside
 * the 256-block window.
 */
using GetBlockHashFn = std::function<types::Uint256(uint64_t block_number)>;

/**
 * @brief Block-level context for EVM execution.
 *
 * Set once per block via EVM::set_block_context(). Provides data for:
 * - NUMBER (0x43), TIMESTAMP (0x42), GASLIMIT (0x45), COINBASE (0x41)
 * - BASEFEE (0x48, EIP-1559), BLOBBASEFEE (0x4A, EIP-4844)
 * - PREVRANDAO (0x44, post-merge)
 * - BLOCKHASH (0x40) via lazy callback
 *
 * The EVM is reusable across blocks - create once, set context per block.
 */
struct BlockContext {
  uint64_t number{};     // NUMBER opcode (0x43)
  uint64_t timestamp{};  // TIMESTAMP opcode (0x42)
  uint64_t gas_limit{};  // GASLIMIT opcode (0x45)
  uint64_t chain_id{};   // CHAINID opcode (0x46)

  types::Uint256 base_fee;       // BASEFEE opcode (0x48, EIP-1559)
  types::Uint256 blob_base_fee;  // BLOBBASEFEE opcode (0x4A, EIP-4844)
  types::Uint256 prev_randao;    // PREVRANDAO opcode (0x44, post-merge)

  types::Address coinbase;  // COINBASE opcode (0x41)

  /// Lookup for BLOCKHASH opcode (0x40).
  /// Returns hash for block number, or zero if out of range.
  GetBlockHashFn get_block_hash;
};

}  // namespace div0::evm

#endif  // DIV0_EVM_BLOCK_CONTEXT_H
