#ifndef DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H
#define DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Legacy transaction (Type 0).
/// Pre-EIP-2718 transaction format with single gas price.
typedef struct {
  uint64_t nonce;
  uint256_t gas_price;
  uint64_t gas_limit;
  address_t *to; // nullptr = contract creation
  uint256_t value;
  bytes_t data;
  uint64_t v; // 27/28 (pre-EIP-155) or chain_id*2+35/36 (EIP-155)
  uint256_t r;
  uint256_t s;
} legacy_tx_t;

/// Initializes a legacy transaction to zero values.
static inline void legacy_tx_init(legacy_tx_t *tx) {
  tx->nonce = 0;
  tx->gas_price = uint256_zero();
  tx->gas_limit = 0;
  tx->to = nullptr;
  tx->value = uint256_zero();
  bytes_init(&tx->data);
  tx->v = 0;
  tx->r = uint256_zero();
  tx->s = uint256_zero();
}

/// Returns the chain ID from a legacy transaction.
/// @param tx The transaction
/// @param chain_id Output chain ID (undefined if not EIP-155)
/// @return true if EIP-155 encoded, false if pre-EIP-155 (v=27 or v=28)
static inline bool legacy_tx_chain_id(const legacy_tx_t *tx, uint64_t *chain_id) {
  if (tx->v == 27 || tx->v == 28) {
    return false; // Pre-EIP-155
  }
  // EIP-155: v = chain_id * 2 + 35 or chain_id * 2 + 36
  if (tx->v >= 35) {
    *chain_id = (tx->v - 35) / 2;
    return true;
  }
  return false;
}

/// Returns the recovery ID (0 or 1) from the v value.
/// @param tx The transaction
/// @return Recovery ID (0 or 1)
static inline int legacy_tx_recovery_id(const legacy_tx_t *tx) {
  if (tx->v == 27 || tx->v == 28) {
    return (int)(tx->v - 27);
  }
  // EIP-155: recovery_id = (v - 35) % 2
  return (int)((tx->v - 35) % 2);
}

/// Checks if transaction is a contract creation.
static inline bool legacy_tx_is_create(const legacy_tx_t *tx) {
  return tx->to == nullptr;
}

#endif // DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H
