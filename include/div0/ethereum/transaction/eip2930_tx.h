#ifndef DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H

#include "div0/ethereum/transaction/access_list.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// EIP-2930 transaction (Type 1).
/// Access list transaction with optional storage slot pre-warming.
/// RLP envelope: 0x01 || RLP([chain_id, nonce, gas_price, gas_limit, to, value,
///                            data, access_list, y_parity, r, s])
typedef struct {
  uint64_t chain_id;
  uint64_t nonce;
  uint256_t gas_price;
  uint64_t gas_limit;
  address_t *to; // nullptr = contract creation
  uint256_t value;
  bytes_t data;
  access_list_t access_list;
  uint8_t y_parity; // 0 or 1
  uint256_t r;
  uint256_t s;
} eip2930_tx_t;

/// Initializes an EIP-2930 transaction to zero values.
static inline void eip2930_tx_init(eip2930_tx_t *tx) {
  tx->chain_id = 0;
  tx->nonce = 0;
  tx->gas_price = uint256_zero();
  tx->gas_limit = 0;
  tx->to = nullptr;
  tx->value = uint256_zero();
  bytes_init(&tx->data);
  access_list_init(&tx->access_list);
  tx->y_parity = 0;
  tx->r = uint256_zero();
  tx->s = uint256_zero();
}

/// Returns the recovery ID (0 or 1) from y_parity.
static inline int eip2930_tx_recovery_id(const eip2930_tx_t *tx) {
  return tx->y_parity;
}

/// Checks if transaction is a contract creation.
static inline bool eip2930_tx_is_create(const eip2930_tx_t *tx) {
  return tx->to == nullptr;
}

#endif // DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H
