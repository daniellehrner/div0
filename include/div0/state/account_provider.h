#ifndef DIV0_STATE_ACCOUNT_PROVIDER_H
#define DIV0_STATE_ACCOUNT_PROVIDER_H

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::state {

/**
 * @brief Abstract interface for account state operations.
 *
 * Provides account balance, nonce, code hash access, and EIP-2929 address access tracking.
 */
class AccountProvider {
 public:
  AccountProvider() = default;
  virtual ~AccountProvider() = default;

  AccountProvider(const AccountProvider& other) = delete;
  AccountProvider(AccountProvider&& other) = delete;
  AccountProvider& operator=(const AccountProvider& other) = delete;
  AccountProvider& operator=(AccountProvider&& other) = delete;

  /// Check if address has been accessed this transaction (EIP-2929 warm/cold)
  [[nodiscard]] virtual bool is_warm(const types::Address& address) = 0;

  /// Mark address as accessed (EIP-2929). Returns true if it was cold (first access).
  virtual bool warm_address(const types::Address& address) = 0;

  /// Check if account is empty (no code, zero nonce, zero balance) - for CALL gas calculation
  [[nodiscard]] virtual bool is_empty(const types::Address& address) = 0;

  /// Check if account exists in state (has been created)
  [[nodiscard]] virtual bool exists(const types::Address& address) = 0;

  /// Begin a new transaction - clears access lists
  virtual void begin_transaction() = 0;

  // TODO: Implement when needed
  // virtual types::Uint256 get_balance(const types::Address& address) = 0;
  // virtual uint64_t get_nonce(const types::Address& address) = 0;
  // virtual types::Uint256 get_code_hash(const types::Address& address) = 0;
};

}  // namespace div0::state

#endif  // DIV0_STATE_ACCOUNT_PROVIDER_H
