#ifndef DIV0_ETHEREUM_TRANSACTION_SIGNER_H
#define DIV0_ETHEREUM_TRANSACTION_SIGNER_H

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/mem/arena.h"
#include "div0/types/hash.h"

/// Computes the signing hash for a legacy transaction.
/// Pre-EIP-155: hash(rlp([nonce, gas_price, gas_limit, to, value, data]))
/// EIP-155:     hash(rlp([nonce, gas_price, gas_limit, to, value, data, chain_id, 0, 0]))
/// @param tx The legacy transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t legacy_tx_signing_hash(const legacy_tx_t *tx, div0_arena_t *arena);

/// Computes the signing hash for an EIP-2930 transaction.
/// hash(0x01 || rlp([chain_id, nonce, gas_price, gas_limit, to, value, data, access_list]))
/// @param tx The EIP-2930 transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t eip2930_tx_signing_hash(const eip2930_tx_t *tx, div0_arena_t *arena);

/// Computes the signing hash for an EIP-1559 transaction.
/// hash(0x02 || rlp([chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data,
/// access_list]))
/// @param tx The EIP-1559 transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t eip1559_tx_signing_hash(const eip1559_tx_t *tx, div0_arena_t *arena);

/// Computes the signing hash for an EIP-4844 transaction.
/// hash(0x03 || rlp([chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data,
///                   access_list, max_fee_per_blob_gas, blob_versioned_hashes]))
/// @param tx The EIP-4844 transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t eip4844_tx_signing_hash(const eip4844_tx_t *tx, div0_arena_t *arena);

/// Computes the signing hash for an EIP-7702 transaction.
/// hash(0x04 || rlp([chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data,
///                   access_list, authorization_list]))
/// @param tx The EIP-7702 transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t eip7702_tx_signing_hash(const eip7702_tx_t *tx, div0_arena_t *arena);

/// Computes the signing hash for any transaction type.
/// @param tx The transaction
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t transaction_signing_hash(const transaction_t *tx, div0_arena_t *arena);

/// Computes the signing hash for an EIP-7702 authorization tuple.
/// hash(0x05 || rlp([chain_id, address, nonce]))
/// @param auth The authorization
/// @param arena Arena for RLP encoding
/// @return Signing hash
hash_t authorization_signing_hash(const authorization_t *auth, div0_arena_t *arena);

/// Recovers the sender address from a transaction signature.
/// @param ctx secp256k1 context
/// @param tx The transaction
/// @param arena Arena for RLP encoding
/// @return Recovery result (check .success before using .address)
ecrecover_result_t transaction_recover_sender(const secp256k1_ctx_t *ctx, const transaction_t *tx,
                                              div0_arena_t *arena);

/// Recovers the authority address from an authorization signature.
/// @param ctx secp256k1 context
/// @param auth The authorization
/// @param arena Arena for RLP encoding
/// @return Recovery result
ecrecover_result_t authorization_recover_authority(const secp256k1_ctx_t *ctx,
                                                   const authorization_t *auth,
                                                   div0_arena_t *arena);

#endif // DIV0_ETHEREUM_TRANSACTION_SIGNER_H
