#ifndef DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H

#include "div0/ethereum/transaction/access_list.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// EIP-1559 transaction (Type 2).
/// Dynamic fee transaction with separate base/priority fees.
/// RLP envelope: 0x02 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
///                            gas_limit, to, value, data, access_list, y_parity, r, s])
typedef struct {
  uint64_t chain_id;
  uint64_t nonce;
  uint256_t max_priority_fee_per_gas;
  uint256_t max_fee_per_gas;
  uint64_t gas_limit;
  address_t *to; // nullptr = contract creation
  uint256_t value;
  bytes_t data;
  access_list_t access_list;
  uint8_t y_parity; // 0 or 1
  uint256_t r;
  uint256_t s;
} eip1559_tx_t;

/// Initializes an EIP-1559 transaction to zero values.
static inline void eip1559_tx_init(eip1559_tx_t *tx) {
  tx->chain_id = 0;
  tx->nonce = 0;
  tx->max_priority_fee_per_gas = uint256_zero();
  tx->max_fee_per_gas = uint256_zero();
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
static inline int eip1559_tx_recovery_id(const eip1559_tx_t *tx) {
  return tx->y_parity;
}

/// Checks if transaction is a contract creation.
static inline bool eip1559_tx_is_create(const eip1559_tx_t *tx) {
  return tx->to == nullptr;
}

/// Computes the effective gas price for an EIP-1559 transaction.
/// effective_gas_price = min(max_fee_per_gas, base_fee + max_priority_fee_per_gas)
/// @param tx The transaction
/// @param base_fee Current block's base fee
/// @return Effective gas price
static inline uint256_t eip1559_tx_effective_gas_price(const eip1559_tx_t *tx, uint256_t base_fee) {
  // priority_price = base_fee + max_priority_fee_per_gas
  uint256_t priority_price = uint256_add(base_fee, tx->max_priority_fee_per_gas);

  // Return min(max_fee_per_gas, priority_price)
  if (uint256_lt(tx->max_fee_per_gas, priority_price)) {
    return tx->max_fee_per_gas;
  }
  return priority_price;
}

#endif // DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H
