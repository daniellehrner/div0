#ifndef DIV0_STATE_ACCOUNT_PROVIDER_H
#define DIV0_STATE_ACCOUNT_PROVIDER_H

#include "div0/types/address.h"
#include "div0/types/hash.h"
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

  // ═══════════════════════════════════════════════════════════════════════════
  // BALANCE OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get account balance. Returns zero for non-existent accounts.
  [[nodiscard]] virtual types::Uint256 get_balance(const types::Address& address) = 0;

  /// Set account balance. Creates account if it doesn't exist.
  virtual void set_balance(const types::Address& address, const types::Uint256& balance) = 0;

  /// Add to account balance. Creates account if needed. Returns false on overflow.
  [[nodiscard]] virtual bool add_balance(const types::Address& address,
                                         const types::Uint256& amount) = 0;

  /// Subtract from account balance. Returns false if insufficient balance.
  [[nodiscard]] virtual bool subtract_balance(const types::Address& address,
                                              const types::Uint256& amount) = 0;

  // ═══════════════════════════════════════════════════════════════════════════
  // NONCE OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get account nonce. Returns zero for non-existent accounts.
  [[nodiscard]] virtual uint64_t get_nonce(const types::Address& address) = 0;

  /// Set account nonce. Creates account if it doesn't exist.
  virtual void set_nonce(const types::Address& address, uint64_t nonce) = 0;

  /// Increment nonce and return the NEW nonce value.
  virtual uint64_t increment_nonce(const types::Address& address) = 0;

  // ═══════════════════════════════════════════════════════════════════════════
  // CODE HASH OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get code hash for an account. Returns EMPTY_CODE_HASH for accounts without code.
  [[nodiscard]] virtual types::Hash get_code_hash(const types::Address& address) = 0;

  /// Set code hash for an account. Creates account if it doesn't exist.
  virtual void set_code_hash(const types::Address& address, const types::Hash& code_hash) = 0;

  // ═══════════════════════════════════════════════════════════════════════════
  // ACCOUNT LIFECYCLE
  // ═══════════════════════════════════════════════════════════════════════════

  /// Create an empty account at address (zero balance, zero nonce, empty code hash).
  /// Does nothing if account already exists.
  virtual void create_account(const types::Address& address) = 0;

  /// Delete an account (for SELFDESTRUCT). Account will no longer exist.
  virtual void delete_account(const types::Address& address) = 0;
};

}  // namespace div0::state

#endif  // DIV0_STATE_ACCOUNT_PROVIDER_H
