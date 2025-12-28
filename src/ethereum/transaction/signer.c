#include "div0/ethereum/transaction/signer.h"

#include "div0/crypto/keccak256.h"
#include "div0/ethereum/transaction/rlp.h"
#include "div0/rlp/encode.h"

hash_t legacy_tx_signing_hash(const legacy_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 512);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [nonce, gas_price, gas_limit, to, value, data]
  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t gas_price_rlp = rlp_encode_uint256(arena, &tx->gas_price);
  rlp_list_append(&output, &gas_price_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  // EIP-155: append [chain_id, 0, 0] if v > 28
  uint64_t chain_id = 0;
  if (legacy_tx_chain_id(tx, &chain_id)) {
    bytes_t chain_id_rlp = rlp_encode_u64(arena, chain_id);
    rlp_list_append(&output, &chain_id_rlp);

    uint256_t zero = uint256_zero();
    bytes_t zero_rlp = rlp_encode_uint256(arena, &zero);
    rlp_list_append(&output, &zero_rlp);
    rlp_list_append(&output, &zero_rlp);
  }

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

hash_t eip2930_tx_signing_hash(const eip2930_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 512);

  // Type byte prefix
  bytes_append_byte(&output, 0x01);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [chain_id, nonce, gas_price, gas_limit, to, value, data, access_list]
  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t gas_price_rlp = rlp_encode_uint256(arena, &tx->gas_price);
  rlp_list_append(&output, &gas_price_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

hash_t eip1559_tx_signing_hash(const eip1559_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 512);

  // Type byte prefix
  bytes_append_byte(&output, 0x02);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data, access_list]
  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = tx_encode_optional_address(arena, tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

hash_t eip4844_tx_signing_hash(const eip4844_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 1024);

  // Type byte prefix
  bytes_append_byte(&output, 0x03);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data,
  //  access_list, max_fee_per_blob_gas, blob_versioned_hashes]
  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = rlp_encode_address(arena, &tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);

  bytes_t max_blob_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_blob_gas);
  rlp_list_append(&output, &max_blob_fee_rlp);

  // Blob versioned hashes list
  rlp_list_builder_t hashes_builder;
  rlp_list_start(&hashes_builder, &output);
  for (size_t i = 0; i < tx->blob_hashes_count; i++) {
    bytes_t hash_rlp = rlp_encode_bytes(arena, tx->blob_versioned_hashes[i].bytes, HASH_SIZE);
    rlp_list_append(&output, &hash_rlp);
  }
  rlp_list_end(&hashes_builder);

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

hash_t eip7702_tx_signing_hash(const eip7702_tx_t *tx, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 1024);

  // Type byte prefix
  bytes_append_byte(&output, 0x04);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [chain_id, nonce, max_priority_fee, max_fee, gas_limit, to, value, data,
  //  access_list, authorization_list]
  bytes_t chain_id_rlp = rlp_encode_u64(arena, tx->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, tx->nonce);
  rlp_list_append(&output, &nonce_rlp);

  bytes_t max_priority_rlp = rlp_encode_uint256(arena, &tx->max_priority_fee_per_gas);
  rlp_list_append(&output, &max_priority_rlp);

  bytes_t max_fee_rlp = rlp_encode_uint256(arena, &tx->max_fee_per_gas);
  rlp_list_append(&output, &max_fee_rlp);

  bytes_t gas_limit_rlp = rlp_encode_u64(arena, tx->gas_limit);
  rlp_list_append(&output, &gas_limit_rlp);

  bytes_t to_rlp = rlp_encode_address(arena, &tx->to);
  rlp_list_append(&output, &to_rlp);

  bytes_t value_rlp = rlp_encode_uint256(arena, &tx->value);
  rlp_list_append(&output, &value_rlp);

  bytes_t data_rlp = rlp_encode_bytes(arena, tx->data.data, tx->data.size);
  rlp_list_append(&output, &data_rlp);

  tx_encode_access_list(&output, &tx->access_list, arena);
  tx_encode_authorization_list(&output, &tx->authorization_list, arena);

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

hash_t transaction_signing_hash(const transaction_t *tx, div0_arena_t *arena) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return legacy_tx_signing_hash(&tx->legacy, arena);
  case TX_TYPE_EIP2930:
    return eip2930_tx_signing_hash(&tx->eip2930, arena);
  case TX_TYPE_EIP1559:
    return eip1559_tx_signing_hash(&tx->eip1559, arena);
  case TX_TYPE_EIP4844:
    return eip4844_tx_signing_hash(&tx->eip4844, arena);
  case TX_TYPE_EIP7702:
    return eip7702_tx_signing_hash(&tx->eip7702, arena);
  default:
    return hash_zero();
  }
}

hash_t authorization_signing_hash(const authorization_t *auth, div0_arena_t *arena) {
  bytes_t output;
  bytes_init_arena(&output, arena);
  bytes_reserve(&output, 64);

  // Magic prefix 0x05
  bytes_append_byte(&output, 0x05);

  rlp_list_builder_t builder;
  rlp_list_start(&builder, &output);

  // [chain_id, address, nonce]
  bytes_t chain_id_rlp = rlp_encode_u64(arena, auth->chain_id);
  rlp_list_append(&output, &chain_id_rlp);

  bytes_t addr_rlp = rlp_encode_address(arena, &auth->address);
  rlp_list_append(&output, &addr_rlp);

  bytes_t nonce_rlp = rlp_encode_u64(arena, auth->nonce);
  rlp_list_append(&output, &nonce_rlp);

  rlp_list_end(&builder);

  return keccak256(output.data, output.size);
}

ecrecover_result_t transaction_recover_sender(const secp256k1_ctx_t *ctx, const transaction_t *tx,
                                              div0_arena_t *arena) {
  hash_t signing_hash = transaction_signing_hash(tx, arena);
  uint256_t msg_hash = hash_to_uint256(&signing_hash);

  uint64_t v = 0;
  uint256_t r = uint256_zero();
  uint256_t s = uint256_zero();
  uint64_t chain_id = 0;

  switch (tx->type) {
  case TX_TYPE_LEGACY:
    v = tx->legacy.v;
    r = tx->legacy.r;
    s = tx->legacy.s;
    legacy_tx_chain_id(&tx->legacy, &chain_id);
    break;
  case TX_TYPE_EIP2930:
    // Typed txs use y_parity (0 or 1)
    v = tx->eip2930.y_parity;
    r = tx->eip2930.r;
    s = tx->eip2930.s;
    chain_id = tx->eip2930.chain_id;
    break;
  case TX_TYPE_EIP1559:
    v = tx->eip1559.y_parity;
    r = tx->eip1559.r;
    s = tx->eip1559.s;
    chain_id = tx->eip1559.chain_id;
    break;
  case TX_TYPE_EIP4844:
    v = tx->eip4844.y_parity;
    r = tx->eip4844.r;
    s = tx->eip4844.s;
    chain_id = tx->eip4844.chain_id;
    break;
  case TX_TYPE_EIP7702:
    v = tx->eip7702.y_parity;
    r = tx->eip7702.r;
    s = tx->eip7702.s;
    chain_id = tx->eip7702.chain_id;
    break;
  default: {
    ecrecover_result_t failed = {.success = false};
    return failed;
  }
  }

  return secp256k1_ecrecover(ctx, &msg_hash, v, &r, &s, chain_id);
}

ecrecover_result_t authorization_recover_authority(const secp256k1_ctx_t *ctx,
                                                   const authorization_t *auth,
                                                   div0_arena_t *arena) {
  hash_t signing_hash = authorization_signing_hash(auth, arena);
  uint256_t msg_hash = hash_to_uint256(&signing_hash);

  // Authorization uses y_parity directly as v
  return secp256k1_ecrecover(ctx, &msg_hash, auth->y_parity, &auth->r, &auth->s, auth->chain_id);
}
