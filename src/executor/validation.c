#include "div0/executor/block_executor.h"
#include "div0/types/uint256.h"

const char *tx_validation_error_str(const tx_validation_error_t err) {
  switch (err) {
  case TX_VALID:
    return "valid";
  case TX_ERR_INVALID_SIGNATURE:
    return "invalid signature";
  case TX_ERR_NONCE_TOO_LOW:
    return "nonce too low";
  case TX_ERR_NONCE_TOO_HIGH:
    return "nonce too high";
  case TX_ERR_INSUFFICIENT_BALANCE:
    return "insufficient balance for gas * price + value";
  case TX_ERR_INTRINSIC_GAS:
    return "intrinsic gas too low";
  case TX_ERR_GAS_LIMIT_EXCEEDED:
    return "gas limit exceeds block gas limit";
  case TX_ERR_MAX_FEE_TOO_LOW:
    return "max fee per gas less than block base fee";
  case TX_ERR_CHAIN_ID_MISMATCH:
    return "chain ID mismatch";
  }
  return "unknown error";
}

/// Get max_fee_per_gas for EIP-1559+ transactions.
/// Returns gas_price for legacy/EIP-2930 transactions.
static uint256_t transaction_max_fee_per_gas(const transaction_t *const tx) {
  switch (tx->type) {
  case TX_TYPE_LEGACY:
    return tx->legacy.gas_price;
  case TX_TYPE_EIP2930:
    return tx->eip2930.gas_price;
  case TX_TYPE_EIP1559:
    return tx->eip1559.max_fee_per_gas;
  case TX_TYPE_EIP4844:
    return tx->eip4844.max_fee_per_gas;
  case TX_TYPE_EIP7702:
    return tx->eip7702.max_fee_per_gas;
  }
  return uint256_zero();
}

tx_validation_error_t block_executor_validate_tx(const block_executor_t *const exec,
                                                 const block_tx_t *const tx,
                                                 const uint64_t cumulative_gas) {
  const transaction_t *const transaction = tx->tx;

  // 1. Chain ID check (if present)
  uint64_t tx_chain_id;
  if (transaction_chain_id(transaction, &tx_chain_id)) {
    if (tx_chain_id != exec->chain_id) {
      return TX_ERR_CHAIN_ID_MISMATCH;
    }
  }

  // 2. Nonce check
  const uint64_t sender_nonce = state_get_nonce(exec->state, &tx->sender);
  const uint64_t tx_nonce = transaction_nonce(transaction);
  if (tx_nonce < sender_nonce) {
    return TX_ERR_NONCE_TOO_LOW;
  }
  if (tx_nonce > sender_nonce) {
    return TX_ERR_NONCE_TOO_HIGH;
  }

  // 3. Intrinsic gas check
  const uint64_t intrinsic = tx_intrinsic_gas(transaction);
  const uint64_t gas_limit = transaction_gas_limit(transaction);
  if (gas_limit < intrinsic) {
    return TX_ERR_INTRINSIC_GAS;
  }

  // 4. Block gas limit check (overflow-safe)
  const uint64_t block_gas_limit = exec->block->gas_limit;
  if (gas_limit > block_gas_limit || cumulative_gas > block_gas_limit - gas_limit) {
    return TX_ERR_GAS_LIMIT_EXCEEDED;
  }

  // 5. Max fee check (EIP-1559+)
  const uint256_t max_fee = transaction_max_fee_per_gas(transaction);
  if (uint256_lt(max_fee, exec->block->base_fee)) {
    return TX_ERR_MAX_FEE_TOO_LOW;
  }

  // 6. Balance check: sender_balance >= value + gas_limit * effective_gas_price
  const uint256_t effective_gas_price =
      transaction_effective_gas_price(transaction, exec->block->base_fee);
  const uint256_t gas_cost = uint256_mul(effective_gas_price, uint256_from_u64(gas_limit));
  const uint256_t tx_value = transaction_value(transaction);
  const uint256_t total_cost = uint256_add(gas_cost, tx_value);

  // Check for overflow in total_cost calculation
  if (uint256_lt(total_cost, tx_value)) {
    return TX_ERR_INSUFFICIENT_BALANCE; // Overflow means impossibly large cost
  }

  const uint256_t sender_balance = state_get_balance(exec->state, &tx->sender);
  if (uint256_lt(sender_balance, total_cost)) {
    return TX_ERR_INSUFFICIENT_BALANCE;
  }

  return TX_VALID;
}
