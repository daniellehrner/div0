#include "div0/executor/block_executor.h"

#include "div0/ethereum/transaction/access_list.h"
#include "div0/ethereum/transaction/rlp.h"
#include "div0/evm/execution_env.h"
#include "div0/types/address.h"

#include <stdint.h>

void block_executor_init(block_executor_t *exec, state_access_t *state,
                         const block_context_t *block, evm_t *evm, div0_arena_t *arena,
                         uint64_t chain_id) {
  exec->state = state;
  exec->block = block;
  exec->evm = evm;
  exec->arena = arena;
  exec->chain_id = chain_id;
  exec->skip_signature_validation = false;
}

/// Warm access list addresses and storage slots (EIP-2930).
static void warm_access_list(state_access_t *state, const access_list_t *access_list) {
  if (!access_list) {
    return;
  }
  for (size_t i = 0; i < access_list->count; i++) {
    const access_list_entry_t *entry = &access_list->entries[i];
    (void)state_warm_address(state, &entry->address);
    for (size_t j = 0; j < entry->storage_keys_count; j++) {
      (void)state_warm_slot(state, &entry->address, entry->storage_keys[j]);
    }
  }
}

/// Get blob hashes for EIP-4844 transactions.
static void get_blob_hashes(const transaction_t *tx, const hash_t **hashes, size_t *count) {
  if (tx->type == TX_TYPE_EIP4844) {
    *hashes = tx->eip4844.blob_versioned_hashes;
    *count = tx->eip4844.blob_hashes_count;
  } else {
    *hashes = nullptr;
    *count = 0;
  }
}

/// Calculate blob gas for EIP-4844 transactions.
static uint64_t get_blob_gas(const transaction_t *tx) {
  if (tx->type == TX_TYPE_EIP4844) {
    return eip4844_tx_blob_gas(&tx->eip4844);
  }
  return 0;
}

/// Execute a single transaction after validation passes.
/// Returns true if execution completed (check receipt.success for EVM result).
static bool execute_transaction(block_executor_t *exec, const block_tx_t *btx,
                                uint64_t *cumulative_gas, exec_receipt_t *receipt) {
  const transaction_t *tx = btx->tx;

  // Set transaction metadata in receipt
  receipt->tx_hash = transaction_hash(tx, exec->arena);
  receipt->tx_type = (uint8_t)tx->type;

  // Get transaction parameters
  uint64_t gas_limit = transaction_gas_limit(tx);
  uint256_t value = transaction_value(tx);
  uint256_t effective_gas_price = transaction_effective_gas_price(tx, exec->block->base_fee);
  const bytes_t *data = transaction_data(tx);
  const address_t *to = transaction_to(tx);
  bool is_create = transaction_is_create(tx);

  // 1. Deduct max gas cost from sender
  uint256_t max_gas_cost = uint256_mul(effective_gas_price, uint256_from_u64(gas_limit));
  if (!state_sub_balance(exec->state, &btx->sender, max_gas_cost)) {
    return false; // Should not happen - validated earlier
  }

  // 2. Increment sender nonce (get nonce before increment for CREATE address)
  uint64_t sender_nonce = state_get_nonce(exec->state, &btx->sender);
  (void)state_increment_nonce(exec->state, &btx->sender);

  // 3. Compute contract address for CREATE transactions
  address_t contract_address = address_zero();
  if (is_create) {
    contract_address = compute_create_address(&btx->sender, sender_nonce);
  }

  // 4. Warm sender and recipient/contract (EIP-2929)
  (void)state_warm_address(exec->state, &btx->sender);
  if (is_create) {
    (void)state_warm_address(exec->state, &contract_address);
  } else if (to) {
    (void)state_warm_address(exec->state, to);
  }

  // 5. Warm access list addresses/slots (EIP-2930)
  warm_access_list(exec->state, transaction_access_list(tx));

  // 6. Take snapshot for potential revert
  uint64_t snapshot = state_snapshot(exec->state);

  // 7. Transfer value from sender to recipient/contract
  if (!uint256_is_zero(value)) {
    if (!state_sub_balance(exec->state, &btx->sender, value)) {
      // Insufficient balance for value transfer
      state_revert_to_snapshot(exec->state, snapshot);
      receipt->success = false;
      receipt->gas_used = gas_limit; // All gas consumed on failure
      goto finalize;
    }
    const address_t *recipient = is_create ? &contract_address : to;
    (void)state_add_balance(exec->state, recipient, value);
  }

  // 8. Build execution environment
  execution_env_t env;
  execution_env_init(&env);
  env.block = exec->block;

  // Transaction context
  env.tx.origin = btx->sender;
  env.tx.gas_price = effective_gas_price;
  get_blob_hashes(tx, &env.tx.blob_hashes, &env.tx.blob_hashes_count);

  // Call parameters
  uint64_t intrinsic = tx_intrinsic_gas(tx);
  env.call.gas = gas_limit - intrinsic;
  env.call.value = value;
  env.call.caller = btx->sender;
  env.call.is_static = false;

  if (is_create) {
    // Contract creation
    env.call.address = contract_address;
    env.call.code = data ? data->data : nullptr;
    env.call.code_size = data ? data->size : 0;
    env.call.input = nullptr;
    env.call.input_size = 0;

    // Create contract account with nonce=1 (EIP-161)
    state_create_contract(exec->state, &contract_address);
  } else {
    // Message call
    env.call.address = *to;
    bytes_t code = state_get_code(exec->state, to);
    env.call.code = code.data;
    env.call.code_size = code.size;
    env.call.input = data ? data->data : nullptr;
    env.call.input_size = data ? data->size : 0;
  }

  // 9. Execute EVM
  evm_reset(exec->evm);
  evm_set_block_context(exec->evm, exec->block);
  evm_set_tx_context(exec->evm, &env.tx);

  evm_execution_result_t result = evm_execute_env(exec->evm, &env);

  // 10. Handle execution result
  if (result.result == EVM_RESULT_STOP) {
    // Success - commit state changes
    state_commit_snapshot(exec->state, snapshot);
    receipt->success = true;

    // Calculate gas used with refund (capped at gas_used / 5)
    uint64_t gas_used = intrinsic + result.gas_used;
    uint64_t max_refund = gas_used / 5;
    uint64_t refund = result.gas_refund < max_refund ? result.gas_refund : max_refund;
    receipt->gas_used = gas_used - refund;

    // For CREATE, set the contract address and store the code
    if (is_create) {
      receipt->created_address = div0_arena_alloc(exec->arena, sizeof(address_t));
      if (receipt->created_address) {
        *receipt->created_address = contract_address;
      }
      // Store deployed code (the return data from CREATE is the runtime code)
      if (result.output_size > 0) {
        state_set_code(exec->state, &contract_address, result.output, result.output_size);
      }
    } else {
      // Copy output data for regular calls
      if (result.output_size > 0) {
        receipt->output = div0_arena_alloc(exec->arena, result.output_size);
        if (receipt->output) {
          __builtin___memcpy_chk(receipt->output, result.output, result.output_size,
                                 result.output_size);
          receipt->output_size = result.output_size;
        }
      }
    }
  } else {
    // Error - revert state changes
    state_revert_to_snapshot(exec->state, snapshot);
    receipt->success = false;
    receipt->gas_used = gas_limit; // All gas consumed on error
  }

finalize:
  // 11. Refund unused gas to sender
  uint64_t gas_remaining = gas_limit - receipt->gas_used;
  if (gas_remaining > 0) {
    uint256_t refund_amount = uint256_mul(effective_gas_price, uint256_from_u64(gas_remaining));
    (void)state_add_balance(exec->state, &btx->sender, refund_amount);
  }

  // 12. Pay coinbase (only priority fee, base fee is burned per EIP-1559)
  uint256_t priority_fee = uint256_sub(effective_gas_price, exec->block->base_fee);
  uint256_t coinbase_payment = uint256_mul(priority_fee, uint256_from_u64(receipt->gas_used));
  (void)state_add_balance(exec->state, &exec->block->coinbase, coinbase_payment);

  // 13. Update cumulative gas
  *cumulative_gas += receipt->gas_used;
  receipt->cumulative_gas = *cumulative_gas;

  // Logs are not captured yet (LOG opcodes not implemented)
  receipt->logs = nullptr;
  receipt->log_count = 0;

  return true;
}

bool block_executor_run(block_executor_t *exec, const block_tx_t *txs, size_t tx_count,
                        block_exec_result_t *result) {
  // Initialize result
  __builtin___memset_chk(result, 0, sizeof(*result), sizeof(*result));

  if (tx_count == 0) {
    result->state_root = state_root(exec->state);
    return true;
  }

  // Allocate arrays (worst case: all receipts or all rejected)
  // Check for overflow before multiplication
  if (tx_count > SIZE_MAX / sizeof(exec_receipt_t) ||
      tx_count > SIZE_MAX / sizeof(exec_rejected_t)) {
    return false;
  }
  result->receipts = div0_arena_alloc(exec->arena, tx_count * sizeof(exec_receipt_t));
  result->rejected = div0_arena_alloc(exec->arena, tx_count * sizeof(exec_rejected_t));
  if (!result->receipts || !result->rejected) {
    return false;
  }

  uint64_t cumulative_gas = 0;
  uint64_t blob_gas_used = 0;

  for (size_t i = 0; i < tx_count; i++) {
    const block_tx_t *btx = &txs[i];

    // Begin new transaction in state (clears warm sets)
    state_begin_transaction(exec->state);

    // Validate transaction
    tx_validation_error_t err = block_executor_validate_tx(exec, btx, cumulative_gas);
    if (err != TX_VALID) {
      // Add to rejected list
      exec_rejected_t *rej = &result->rejected[result->rejected_count++];
      rej->index = btx->original_index;
      rej->error = err;
      rej->error_message = tx_validation_error_str(err);
      continue;
    }

    // Execute transaction
    exec_receipt_t *receipt = &result->receipts[result->receipt_count];
    __builtin___memset_chk(receipt, 0, sizeof(*receipt), sizeof(*receipt));

    if (!execute_transaction(exec, btx, &cumulative_gas, receipt)) {
      // Fatal error during execution (balance deduction failed unexpectedly)
      exec_rejected_t *rej = &result->rejected[result->rejected_count++];
      rej->index = btx->original_index;
      rej->error = TX_ERR_INSUFFICIENT_BALANCE;
      rej->error_message = "gas cost deduction failed unexpectedly";
      continue;
    }

    result->receipt_count++;

    // Track blob gas (saturating add to prevent overflow)
    uint64_t tx_blob_gas = get_blob_gas(btx->tx);
    if (blob_gas_used <= UINT64_MAX - tx_blob_gas) {
      blob_gas_used += tx_blob_gas;
    } else {
      blob_gas_used = UINT64_MAX; // Saturate on overflow
    }
  }

  // Compute final state root
  result->gas_used = cumulative_gas;
  result->blob_gas_used = blob_gas_used;
  result->state_root = state_root(exec->state);

  return true;
}
