#include "div0/t8n/env.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

// ============================================================================
// Initialization
// ============================================================================

void t8n_env_init(t8n_env_t *env) {
  __builtin___memset_chk(env, 0, sizeof(*env), __builtin_object_size(env, 0));
}

// ============================================================================
// Parsing Helpers
// ============================================================================

static json_result_t parse_block_hashes(yyjson_val_t *const obj, div0_arena_t *const arena,
                                        t8n_block_hash_t **const out, size_t *const out_count) {
  yyjson_val_t *const hashes = json_obj_get(obj, "blockHashes");
  if (hashes == nullptr || !json_is_obj(hashes)) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  const size_t count = json_obj_size(hashes);
  if (count == 0) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  *out = div0_arena_alloc(arena, count * sizeof(t8n_block_hash_t));
  if (*out == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate block hashes");
  }

  json_obj_iter_t iter = json_obj_iter(hashes);
  const char *num_str;
  yyjson_val_t *hash_val;
  size_t idx = 0;

  while (json_obj_iter_next(&iter, &num_str, &hash_val)) {
    t8n_block_hash_t *const entry = &(*out)[idx];

    // Parse block number from key (can be decimal or hex)
    if (!hex_decode_u64(num_str, &entry->number)) {
      // Try decimal
      char *end;
      entry->number = strtoull(num_str, &end, 10);
      if (*end != '\0') {
        return json_err(JSON_ERR_INVALID_HEX, "invalid block number key");
      }
    }

    if (!json_val_hex_hash(hash_val, &entry->hash)) {
      return json_err(JSON_ERR_INVALID_HEX, "invalid block hash value");
    }

    idx++;
  }

  *out_count = idx;
  return json_ok();
}

static json_result_t parse_withdrawals(yyjson_val_t *const obj, div0_arena_t *const arena,
                                       t8n_withdrawal_t **const out, size_t *const out_count) {
  yyjson_val_t *const withdrawals = json_obj_get(obj, "withdrawals");
  if (withdrawals == nullptr || !json_is_arr(withdrawals)) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  const size_t count = json_arr_len(withdrawals);
  if (count == 0) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  *out = div0_arena_alloc(arena, count * sizeof(t8n_withdrawal_t));
  if (*out == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate withdrawals");
  }

  json_arr_iter_t iter = json_arr_iter(withdrawals);
  yyjson_val_t *w_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &w_val)) {
    if (!json_is_obj(w_val)) {
      return json_err(JSON_ERR_INVALID_TYPE, "withdrawal must be an object");
    }

    t8n_withdrawal_t *const w = &(*out)[idx];

    if (!json_get_hex_u64(w_val, "index", &w->index)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing withdrawal index");
    }
    if (!json_get_hex_u64(w_val, "validatorIndex", &w->validator_index)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing withdrawal validatorIndex");
    }
    if (!json_get_hex_address(w_val, "address", &w->address)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing withdrawal address");
    }
    if (!json_get_hex_u64(w_val, "amount", &w->amount)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing withdrawal amount");
    }

    idx++;
  }

  *out_count = idx;
  return json_ok();
}

static json_result_t parse_ommers(yyjson_val_t *const obj, div0_arena_t *const arena,
                                  t8n_ommer_t **const out, size_t *const out_count) {
  yyjson_val_t *const ommers = json_obj_get(obj, "ommers");
  if (ommers == nullptr || !json_is_arr(ommers)) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  const size_t count = json_arr_len(ommers);
  if (count == 0) {
    *out = nullptr;
    *out_count = 0;
    return json_ok();
  }

  *out = div0_arena_alloc(arena, count * sizeof(t8n_ommer_t));
  if (*out == nullptr) {
    return json_err(JSON_ERR_ALLOC, "failed to allocate ommers");
  }

  json_arr_iter_t iter = json_arr_iter(ommers);
  yyjson_val_t *o_val;
  size_t idx = 0;

  while (json_arr_iter_next(&iter, &o_val)) {
    if (!json_is_obj(o_val)) {
      return json_err(JSON_ERR_INVALID_TYPE, "ommer must be an object");
    }

    t8n_ommer_t *const o = &(*out)[idx];

    if (!json_get_hex_address(o_val, "address", &o->coinbase)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing ommer address");
    }
    if (!json_get_hex_u64(o_val, "delta", &o->delta)) {
      return json_err(JSON_ERR_MISSING_FIELD, "missing ommer delta");
    }

    idx++;
  }

  *out_count = idx;
  return json_ok();
}

// ============================================================================
// Parsing Implementation
// ============================================================================

json_result_t t8n_parse_env_value(yyjson_val_t *const root, div0_arena_t *const arena,
                                  t8n_env_t *const out) {
  if (root == nullptr || !json_is_obj(root)) {
    return json_err(JSON_ERR_INVALID_TYPE, "env must be an object");
  }

  t8n_env_init(out);

  // Required fields
  if (!json_get_hex_address(root, "currentCoinbase", &out->coinbase)) {
    return json_err(JSON_ERR_MISSING_FIELD, "missing currentCoinbase");
  }
  if (!json_get_hex_u64(root, "currentGasLimit", &out->gas_limit)) {
    return json_err(JSON_ERR_MISSING_FIELD, "missing currentGasLimit");
  }
  if (!json_get_hex_u64(root, "currentNumber", &out->number)) {
    return json_err(JSON_ERR_MISSING_FIELD, "missing currentNumber");
  }
  if (!json_get_hex_u64(root, "currentTimestamp", &out->timestamp)) {
    return json_err(JSON_ERR_MISSING_FIELD, "missing currentTimestamp");
  }

  // Optional fields - difficulty/randao
  out->has_difficulty = json_get_hex_uint256(root, "currentDifficulty", &out->difficulty);
  out->has_prev_randao = json_get_hex_uint256(root, "currentRandom", &out->prev_randao);
  if (!out->has_prev_randao) {
    out->has_prev_randao = json_get_hex_uint256(root, "prevRandao", &out->prev_randao);
  }

  // Optional fields - EIP-1559
  out->has_base_fee = json_get_hex_uint256(root, "currentBaseFee", &out->base_fee);

  // Optional fields - EIP-4844
  out->has_excess_blob_gas = json_get_hex_u64(root, "currentExcessBlobGas", &out->excess_blob_gas);
  out->has_blob_gas_used = json_get_hex_u64(root, "currentBlobGasUsed", &out->blob_gas_used);

  // Optional fields - EIP-4788
  out->has_parent_beacon_root =
      json_get_hex_hash(root, "parentBeaconBlockRoot", &out->parent_beacon_root);

  // Parent fields for base fee calculation
  out->has_parent_base_fee = json_get_hex_uint256(root, "parentBaseFee", &out->parent_base_fee);
  out->has_parent_gas_used = json_get_hex_u64(root, "parentGasUsed", &out->parent_gas_used);
  out->has_parent_gas_limit = json_get_hex_u64(root, "parentGasLimit", &out->parent_gas_limit);
  out->has_parent_excess_blob_gas =
      json_get_hex_u64(root, "parentExcessBlobGas", &out->parent_excess_blob_gas);
  out->has_parent_blob_gas_used =
      json_get_hex_u64(root, "parentBlobGasUsed", &out->parent_blob_gas_used);

  // Arrays
  json_result_t result =
      parse_block_hashes(root, arena, &out->block_hashes, &out->block_hash_count);
  if (json_is_err(result)) {
    return result;
  }

  result = parse_withdrawals(root, arena, &out->withdrawals, &out->withdrawal_count);
  if (json_is_err(result)) {
    return result;
  }

  result = parse_ommers(root, arena, &out->ommers, &out->ommer_count);
  if (json_is_err(result)) {
    return result;
  }

  return json_ok();
}

json_result_t t8n_parse_env(const char *const json, const size_t len, div0_arena_t *const arena,
                            t8n_env_t *const out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse(json, len, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *const root = json_doc_root(&doc);
  result = t8n_parse_env_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

json_result_t t8n_parse_env_file(const char *const path, div0_arena_t *const arena,
                                 t8n_env_t *const out) {
  json_doc_t doc = {.doc = nullptr};
  json_result_t result = json_parse_file(path, &doc);
  if (json_is_err(result)) {
    return result;
  }

  yyjson_val_t *const root = json_doc_root(&doc);
  result = t8n_parse_env_value(root, arena, out);

  json_doc_free(&doc);
  return result;
}

#endif // DIV0_FREESTANDING
