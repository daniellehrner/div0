#ifndef DIV0_EVM_TX_CONTEXT_H
#define DIV0_EVM_TX_CONTEXT_H

#include <span>

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief Transaction-level context for EVM execution.
 *
 * Set once per transaction via EVM::set_tx_context(). Provides data for:
 * - ORIGIN (0x32): Original transaction sender
 * - GASPRICE (0x3A): Effective gas price
 * - BLOBHASH (0x49, EIP-4844): Blob versioned hashes
 */
struct TxContext {
  types::Address origin;     // ORIGIN opcode (0x32) - original tx sender
  types::Uint256 gas_price;  // GASPRICE opcode (0x3A) - effective gas price

  // Blob versioned hashes for BLOBHASH opcode (0x49, EIP-4844).
  // Non-owning view into transaction's blob hashes.
  // Empty span for non-blob transactions.
  std::span<const types::Uint256> blob_hashes;
};

}  // namespace div0::evm

#endif  // DIV0_EVM_TX_CONTEXT_H
