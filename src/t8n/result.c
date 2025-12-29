#include "div0/t8n/result.h"

#ifndef DIV0_FREESTANDING

#include "div0/util/hex.h"

#include <string.h>
#include <yyjson.h>

// ============================================================================
// Initialization
// ============================================================================

void t8n_result_init(t8n_result_t *result) {
  __builtin___memset_chk(result, 0, sizeof(*result), __builtin_object_size(result, 0));
}

// ============================================================================
// Serialization
// ============================================================================

yyjson_mut_val_t *t8n_write_log(const t8n_log_t *log, json_writer_t *w) {
  yyjson_mut_val_t *obj = json_write_obj(w);
  if (obj == nullptr) {
    return nullptr;
  }

  json_obj_add_hex_address(w, obj, "address", &log->address);

  // Topics array
  yyjson_mut_val_t *topics_arr = json_write_arr(w);
  for (size_t i = 0; i < log->topic_count; i++) {
    yyjson_mut_val_t *topic = json_write_hex_hash(w, &log->topics[i]);
    json_arr_append(w, topics_arr, topic);
  }
  json_obj_add(w, obj, "topics", topics_arr);

  // Data
  json_obj_add_hex_bytes(w, obj, "data", log->data.data, log->data.size);

  return obj;
}

yyjson_mut_val_t *t8n_write_receipt(const t8n_receipt_t *receipt, json_writer_t *w) {
  yyjson_mut_val_t *obj = json_write_obj(w);
  if (obj == nullptr) {
    return nullptr;
  }

  json_obj_add_hex_u64(w, obj, "type", receipt->type);
  json_obj_add_hex_hash(w, obj, "transactionHash", &receipt->tx_hash);
  json_obj_add_hex_u64(w, obj, "transactionIndex", receipt->transaction_index);
  json_obj_add_hex_u64(w, obj, "gasUsed", receipt->gas_used);
  json_obj_add_hex_u64(w, obj, "cumulativeGasUsed", receipt->cumulative_gas);
  json_obj_add_hex_u64(w, obj, "status", receipt->status ? 1 : 0);

  // Bloom filter
  json_obj_add_hex_bytes(w, obj, "logsBloom", receipt->bloom, 256);

  // Logs array
  yyjson_mut_val_t *logs_arr = json_write_arr(w);
  for (size_t i = 0; i < receipt->log_count; i++) {
    yyjson_mut_val_t *log_obj = t8n_write_log(&receipt->logs[i], w);
    if (log_obj == nullptr) {
      return nullptr;
    }
    json_arr_append(w, logs_arr, log_obj);
  }
  json_obj_add(w, obj, "logs", logs_arr);

  // Contract address (if created)
  if (receipt->contract_address != nullptr) {
    json_obj_add_hex_address(w, obj, "contractAddress", receipt->contract_address);
  }

  return obj;
}

yyjson_mut_val_t *t8n_write_result(const t8n_result_t *result, json_writer_t *w) {
  yyjson_mut_val_t *obj = json_write_obj(w);
  if (obj == nullptr) {
    return nullptr;
  }

  // Roots
  json_obj_add_hex_hash(w, obj, "stateRoot", &result->state_root);
  json_obj_add_hex_hash(w, obj, "txRoot", &result->tx_root);
  json_obj_add_hex_hash(w, obj, "receiptsRoot", &result->receipts_root);
  json_obj_add_hex_hash(w, obj, "logsHash", &result->logs_hash);
  json_obj_add_hex_bytes(w, obj, "logsBloom", result->logs_bloom, 256);

  // Gas
  json_obj_add_hex_u64(w, obj, "gasUsed", result->gas_used);

  // Blob gas (optional)
  if (result->has_blob_gas_used) {
    json_obj_add_hex_u64(w, obj, "blobGasUsed", result->blob_gas_used);
  }

  // Receipts array
  yyjson_mut_val_t *receipts_arr = json_write_arr(w);
  for (size_t i = 0; i < result->receipt_count; i++) {
    yyjson_mut_val_t *receipt_obj = t8n_write_receipt(&result->receipts[i], w);
    if (receipt_obj == nullptr) {
      return nullptr;
    }
    json_arr_append(w, receipts_arr, receipt_obj);
  }
  json_obj_add(w, obj, "receipts", receipts_arr);

  // Rejected transactions
  if (result->rejected_count > 0) {
    yyjson_mut_val_t *rejected_arr = json_write_arr(w);
    for (size_t i = 0; i < result->rejected_count; i++) {
      yyjson_mut_val_t *rej_obj = json_write_obj(w);
      json_obj_add_u64(w, rej_obj, "index", result->rejected[i].index);
      json_obj_add_str(w, rej_obj, "error", result->rejected[i].error);
      json_arr_append(w, rejected_arr, rej_obj);
    }
    json_obj_add(w, obj, "rejected", rejected_arr);
  }

  // Optional fork-specific fields
  if (result->has_current_difficulty) {
    json_obj_add_hex_uint256(w, obj, "currentDifficulty", &result->current_difficulty);
  }
  if (result->has_current_base_fee) {
    json_obj_add_hex_uint256(w, obj, "currentBaseFee", &result->current_base_fee);
  }
  if (result->has_withdrawals_root) {
    json_obj_add_hex_hash(w, obj, "withdrawalsRoot", &result->withdrawals_root);
  }
  if (result->has_current_excess_blob_gas) {
    json_obj_add_hex_u64(w, obj, "currentExcessBlobGas", result->current_excess_blob_gas);
  }
  if (result->has_requests_hash) {
    json_obj_add_hex_hash(w, obj, "requestsHash", &result->requests_hash);
  }

  return obj;
}

#endif // DIV0_FREESTANDING
