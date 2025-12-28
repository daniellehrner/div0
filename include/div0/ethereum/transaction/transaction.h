#ifndef DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H
#define DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H

#include "div0/ethereum/transaction/eip1559_tx.h"
#include "div0/ethereum/transaction/eip2930_tx.h"
#include "div0/ethereum/transaction/eip4844_tx.h"
#include "div0/ethereum/transaction/eip7702_tx.h"
#include "div0/ethereum/transaction/legacy_tx.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Transaction type enumeration (EIP-2718).
typedef enum {
  TX_TYPE_LEGACY = 0,
  TX_TYPE_EIP2930 = 1,
  TX_TYPE_EIP1559 = 2,
  TX_TYPE_EIP4844 = 3,
  TX_TYPE_EIP7702 = 4,
} tx_type_t;

/// Unified transaction type (tagged union).
/// Use transaction_* accessor functions for type-safe access.
typedef struct {
  tx_type_t type;
  union {
    legacy_tx_t legacy;
    eip2930_tx_t eip2930;
    eip1559_tx_t eip1559;
    eip4844_tx_t eip4844;
    eip7702_tx_t eip7702;
  };
} transaction_t;

/// Initializes a transaction as a legacy type with zero values.
static inline void transaction_init(transaction_t *tx) {
  tx->type = TX_TYPE_LEGACY;
  legacy_tx_init(&tx->legacy);
}

/// Returns the nonce of the transaction.
static inline uint64_t transaction_nonce(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.nonce;
  case TX_TYPE_EIP2930:
    return tx->eip2930.nonce;
  case TX_TYPE_EIP1559:
    return tx->eip1559.nonce;
  case TX_TYPE_EIP4844:
    return tx->eip4844.nonce;
  case TX_TYPE_EIP7702:
    return tx->eip7702.nonce;
  }
  return 0;
}

/// Returns the gas limit of the transaction.
static inline uint64_t transaction_gas_limit(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.gas_limit;
  case TX_TYPE_EIP2930:
    return tx->eip2930.gas_limit;
  case TX_TYPE_EIP1559:
    return tx->eip1559.gas_limit;
  case TX_TYPE_EIP4844:
    return tx->eip4844.gas_limit;
  case TX_TYPE_EIP7702:
    return tx->eip7702.gas_limit;
  }
  return 0;
}

/// Returns the value of the transaction.
static inline uint256_t transaction_value(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.value;
  case TX_TYPE_EIP2930:
    return tx->eip2930.value;
  case TX_TYPE_EIP1559:
    return tx->eip1559.value;
  case TX_TYPE_EIP4844:
    return tx->eip4844.value;
  case TX_TYPE_EIP7702:
    return tx->eip7702.value;
  }
  return uint256_zero();
}

/// Returns the recipient address, or nullptr for contract creation.
static inline const address_t *transaction_to(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.to;
  case TX_TYPE_EIP2930:
    return tx->eip2930.to;
  case TX_TYPE_EIP1559:
    return tx->eip1559.to;
  case TX_TYPE_EIP4844:
    return &tx->eip4844.to; // Always present
  case TX_TYPE_EIP7702:
    return &tx->eip7702.to; // Always present
  }
  return nullptr;
}

/// Checks if transaction is a contract creation.
static inline bool transaction_is_create(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return legacy_tx_is_create(&tx->legacy);
  case TX_TYPE_EIP2930:
    return eip2930_tx_is_create(&tx->eip2930);
  case TX_TYPE_EIP1559:
    return eip1559_tx_is_create(&tx->eip1559);
  case TX_TYPE_EIP4844:
  case TX_TYPE_EIP7702:
    return false; // Cannot create contracts
  }
  return false;
}

/// Returns the transaction data.
static inline const bytes_t *transaction_data(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return &tx->legacy.data;
  case TX_TYPE_EIP2930:
    return &tx->eip2930.data;
  case TX_TYPE_EIP1559:
    return &tx->eip1559.data;
  case TX_TYPE_EIP4844:
    return &tx->eip4844.data;
  case TX_TYPE_EIP7702:
    return &tx->eip7702.data;
  }
  return nullptr;
}

/// Returns the chain ID of the transaction.
/// @param tx The transaction
/// @param chain_id Output chain ID
/// @return true if chain ID is present, false otherwise
static inline bool transaction_chain_id(const transaction_t *tx, uint64_t *chain_id) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return legacy_tx_chain_id(&tx->legacy, chain_id);
  case TX_TYPE_EIP2930:
    *chain_id = tx->eip2930.chain_id;
    return true;
  case TX_TYPE_EIP1559:
    *chain_id = tx->eip1559.chain_id;
    return true;
  case TX_TYPE_EIP4844:
    *chain_id = tx->eip4844.chain_id;
    return true;
  case TX_TYPE_EIP7702:
    *chain_id = tx->eip7702.chain_id;
    return true;
  }
  return false;
}

/// Returns the recovery ID for signature recovery.
static inline int transaction_recovery_id(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return legacy_tx_recovery_id(&tx->legacy);
  case TX_TYPE_EIP2930:
    return eip2930_tx_recovery_id(&tx->eip2930);
  case TX_TYPE_EIP1559:
    return eip1559_tx_recovery_id(&tx->eip1559);
  case TX_TYPE_EIP4844:
    return eip4844_tx_recovery_id(&tx->eip4844);
  case TX_TYPE_EIP7702:
    return eip7702_tx_recovery_id(&tx->eip7702);
  }
  return 0;
}

/// Returns the r component of the signature.
static inline uint256_t transaction_r(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.r;
  case TX_TYPE_EIP2930:
    return tx->eip2930.r;
  case TX_TYPE_EIP1559:
    return tx->eip1559.r;
  case TX_TYPE_EIP4844:
    return tx->eip4844.r;
  case TX_TYPE_EIP7702:
    return tx->eip7702.r;
  }
  return uint256_zero();
}

/// Returns the s component of the signature.
static inline uint256_t transaction_s(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.s;
  case TX_TYPE_EIP2930:
    return tx->eip2930.s;
  case TX_TYPE_EIP1559:
    return tx->eip1559.s;
  case TX_TYPE_EIP4844:
    return tx->eip4844.s;
  case TX_TYPE_EIP7702:
    return tx->eip7702.s;
  }
  return uint256_zero();
}

/// Computes the effective gas price for the transaction.
/// @param tx The transaction
/// @param base_fee Current block's base fee (only used for EIP-1559+)
/// @return Effective gas price
static inline uint256_t transaction_effective_gas_price(const transaction_t *tx,
                                                        uint256_t base_fee) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.gas_price;
  case TX_TYPE_EIP2930:
    return tx->eip2930.gas_price;
  case TX_TYPE_EIP1559:
    return eip1559_tx_effective_gas_price(&tx->eip1559, base_fee);
  case TX_TYPE_EIP4844:
    return eip4844_tx_effective_gas_price(&tx->eip4844, base_fee);
  case TX_TYPE_EIP7702:
    return eip7702_tx_effective_gas_price(&tx->eip7702, base_fee);
  }
  return uint256_zero();
}

/// Returns the access list of the transaction, or nullptr if not present.
static inline const access_list_t *transaction_access_list(const transaction_t *tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return nullptr;
  case TX_TYPE_EIP2930:
    return &tx->eip2930.access_list;
  case TX_TYPE_EIP1559:
    return &tx->eip1559.access_list;
  case TX_TYPE_EIP4844:
    return &tx->eip4844.access_list;
  case TX_TYPE_EIP7702:
    return &tx->eip7702.access_list;
  }
  return nullptr;
}

#endif // DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H
