#ifndef DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H

#include "div0/ethereum/transaction/access_list.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Gas per blob (EIP-4844).
static constexpr uint64_t GAS_PER_BLOB = 131072;

/// EIP-4844 transaction (Type 3).
/// Blob-carrying transaction for data availability.
/// Note: Cannot create contracts (to field is required).
/// RLP envelope: 0x03 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
///                            gas_limit, to, value, data, access_list, max_fee_per_blob_gas,
///                            blob_versioned_hashes, y_parity, r, s])
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
  uint256_t max_fee_per_blob_gas;
  hash_t *blob_versioned_hashes; // Arena-allocated, must be non-empty
  size_t blob_hashes_count;
  uint8_t y_parity; // 0 or 1
  uint256_t r;
  uint256_t s;
} eip4844_tx_t;

/// Initializes an EIP-4844 transaction to zero values.
static inline void eip4844_tx_init(eip4844_tx_t *tx) {
  tx->chain_id = 0;
  tx->nonce = 0;
  tx->max_priority_fee_per_gas = uint256_zero();
  tx->max_fee_per_gas = uint256_zero();
  tx->gas_limit = 0;
  tx->to = address_zero();
  tx->value = uint256_zero();
  bytes_init(&tx->data);
  access_list_init(&tx->access_list);
  tx->max_fee_per_blob_gas = uint256_zero();
  tx->blob_versioned_hashes = nullptr;
  tx->blob_hashes_count = 0;
  tx->y_parity = 0;
  tx->r = uint256_zero();
  tx->s = uint256_zero();
}

/// Returns the recovery ID (0 or 1) from y_parity.
static inline int eip4844_tx_recovery_id(const eip4844_tx_t *tx) {
  return tx->y_parity;
}

/// Returns the total blob gas used by this transaction.
static inline uint64_t eip4844_tx_blob_gas(const eip4844_tx_t *tx) {
  return tx->blob_hashes_count * GAS_PER_BLOB;
}

/// Computes the effective gas price for an EIP-4844 transaction.
/// effective_gas_price = min(max_fee_per_gas, base_fee + max_priority_fee_per_gas)
static inline uint256_t eip4844_tx_effective_gas_price(const eip4844_tx_t *tx, uint256_t base_fee) {
  uint256_t priority_price = uint256_add(base_fee, tx->max_priority_fee_per_gas);
  if (uint256_lt(tx->max_fee_per_gas, priority_price)) {
    return tx->max_fee_per_gas;
  }
  return priority_price;
}

/// Allocates space for blob versioned hashes.
/// @param tx Transaction to allocate hashes for
/// @param count Number of blob hashes
/// @param arena Arena for allocation
/// @return true on success, false on allocation failure
static inline bool eip4844_tx_alloc_blob_hashes(eip4844_tx_t *tx, size_t count,
                                                div0_arena_t *arena) {
  if (count == 0) {
    tx->blob_versioned_hashes = nullptr;
    tx->blob_hashes_count = 0;
    return true;
  }
  tx->blob_versioned_hashes = (hash_t *)div0_arena_alloc(arena, count * sizeof(hash_t));
  if (!tx->blob_versioned_hashes) {
    return false;
  }
  tx->blob_hashes_count = count;
  return true;
}

#endif // DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H
