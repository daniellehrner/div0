#ifndef DIV0_EVM_TX_CONTEXT_H
#define DIV0_EVM_TX_CONTEXT_H

#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

/// Transaction-level execution context.
/// Set once per transaction execution.
typedef struct {
  address_t origin;          // ORIGIN opcode (0x32) - initial transaction sender
  uint256_t gas_price;       // GASPRICE opcode (0x3A) - effective gas price
  const hash_t *blob_hashes; // BLOBHASH opcode (0x49, EIP-4844)
  size_t blob_hashes_count;  // Number of blob hashes
} tx_context_t;

/// Initializes a transaction context with default values.
static inline void tx_context_init(tx_context_t *ctx) {
  ctx->origin = address_zero();
  ctx->gas_price = uint256_zero();
  ctx->blob_hashes = nullptr;
  ctx->blob_hashes_count = 0;
}

#endif // DIV0_EVM_TX_CONTEXT_H
