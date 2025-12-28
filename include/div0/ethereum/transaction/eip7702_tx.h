#ifndef DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H

#include "div0/ethereum/transaction/access_list.h"
#include "div0/ethereum/transaction/authorization.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// EIP-7702 transaction (Type 4).
/// SetCode transaction allowing EOA code delegation.
/// Note: Cannot create contracts (to field is required).
/// RLP envelope: 0x04 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
///                            gas_limit, to, value, data, access_list, authorization_list,
///                            y_parity, r, s])
typedef struct {
  uint64_t chain_id;
  uint64_t nonce;
  uint256_t max_priority_fee_per_gas;
  uint256_t max_fee_per_gas;
  uint64_t gas_limit;
  address_t to; // Required (no contract creation allowed)
  uint256_t value;
  bytes_t data;
  access_list_t access_list;
  authorization_list_t authorization_list; // Must not be empty
  uint8_t y_parity;                        // 0 or 1
  uint256_t r;
  uint256_t s;
} eip7702_tx_t;

/// Initializes an EIP-7702 transaction to zero values.
static inline void eip7702_tx_init(eip7702_tx_t *tx) {
  tx->chain_id = 0;
  tx->nonce = 0;
  tx->max_priority_fee_per_gas = uint256_zero();
  tx->max_fee_per_gas = uint256_zero();
  tx->gas_limit = 0;
  tx->to = address_zero();
  tx->value = uint256_zero();
  bytes_init(&tx->data);
  access_list_init(&tx->access_list);
  authorization_list_init(&tx->authorization_list);
  tx->y_parity = 0;
  tx->r = uint256_zero();
  tx->s = uint256_zero();
}

/// Returns the recovery ID (0 or 1) from y_parity.
static inline int eip7702_tx_recovery_id(const eip7702_tx_t *tx) {
  return tx->y_parity;
}

/// Computes the effective gas price for an EIP-7702 transaction.
/// effective_gas_price = min(max_fee_per_gas, base_fee + max_priority_fee_per_gas)
static inline uint256_t eip7702_tx_effective_gas_price(const eip7702_tx_t *tx, uint256_t base_fee) {
  uint256_t priority_price = uint256_add(base_fee, tx->max_priority_fee_per_gas);
  if (uint256_lt(tx->max_fee_per_gas, priority_price)) {
    return tx->max_fee_per_gas;
  }
  return priority_price;
}

#endif // DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H
