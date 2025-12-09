#include "div0/state/memory_account_provider.h"

#include "div0/ethereum/account.h"

namespace div0::state {

MemoryAccountProvider::MemoryAccountProvider() = default;

// =============================================================================
// EIP-2929 WARM/COLD TRACKING
// =============================================================================

bool MemoryAccountProvider::is_warm(const types::Address& address) {
  return warm_addresses_.contains(address);
}

bool MemoryAccountProvider::warm_address(const types::Address& address) {
  auto [_, inserted] = warm_addresses_.insert(address);
  return inserted;  // true if was cold (newly inserted)
}

// =============================================================================
// ACCOUNT EXISTENCE CHECKS
// =============================================================================

bool MemoryAccountProvider::is_empty(const types::Address& address) {
  auto it = accounts_.find(address);
  if (it == accounts_.end()) {
    return true;  // Non-existent accounts are empty
  }
  const auto& data = it->second;
  return data.balance.is_zero() && data.nonce == 0 && data.code_hash == ethereum::EMPTY_CODE_HASH;
}

bool MemoryAccountProvider::exists(const types::Address& address) {
  return accounts_.contains(address);
}

// =============================================================================
// TRANSACTION BOUNDARY
// =============================================================================

void MemoryAccountProvider::begin_transaction() { warm_addresses_.clear(); }

// =============================================================================
// BALANCE OPERATIONS
// =============================================================================

types::Uint256 MemoryAccountProvider::get_balance(const types::Address& address) {
  auto it = accounts_.find(address);
  if (it == accounts_.end()) {
    return types::Uint256::zero();
  }
  return it->second.balance;
}

void MemoryAccountProvider::set_balance(const types::Address& address,
                                        const types::Uint256& balance) {
  get_or_create_account(address).balance = balance;
}

bool MemoryAccountProvider::add_balance(const types::Address& address,
                                        const types::Uint256& amount) {
  if (amount.is_zero()) {
    return true;  // Nothing to add
  }

  auto& account = get_or_create_account(address);

  // Check for overflow
  const types::Uint256 new_balance = account.balance + amount;
  if (new_balance < account.balance) {
    return false;  // Overflow
  }

  account.balance = new_balance;
  return true;
}

bool MemoryAccountProvider::subtract_balance(const types::Address& address,
                                             const types::Uint256& amount) {
  if (amount.is_zero()) {
    return true;  // Nothing to subtract
  }

  auto it = accounts_.find(address);
  if (it == accounts_.end()) {
    return false;  // Can't subtract from non-existent account
  }

  if (it->second.balance < amount) {
    return false;  // Insufficient balance
  }

  it->second.balance = it->second.balance - amount;
  return true;
}

// =============================================================================
// NONCE OPERATIONS
// =============================================================================

uint64_t MemoryAccountProvider::get_nonce(const types::Address& address) {
  auto it = accounts_.find(address);
  if (it == accounts_.end()) {
    return 0;
  }
  return it->second.nonce;
}

void MemoryAccountProvider::set_nonce(const types::Address& address, uint64_t nonce) {
  get_or_create_account(address).nonce = nonce;
}

uint64_t MemoryAccountProvider::increment_nonce(const types::Address& address) {
  auto& account = get_or_create_account(address);
  return ++account.nonce;
}

// =============================================================================
// CODE HASH OPERATIONS
// =============================================================================

types::Hash MemoryAccountProvider::get_code_hash(const types::Address& address) {
  auto it = accounts_.find(address);
  if (it == accounts_.end()) {
    return ethereum::EMPTY_CODE_HASH;
  }
  return it->second.code_hash;
}

void MemoryAccountProvider::set_code_hash(const types::Address& address,
                                          const types::Hash& code_hash) {
  get_or_create_account(address).code_hash = code_hash;
}

// =============================================================================
// ACCOUNT LIFECYCLE
// =============================================================================

void MemoryAccountProvider::create_account(const types::Address& address) {
  if (!accounts_.contains(address)) {
    accounts_[address] = AccountData{
        .balance = types::Uint256::zero(),
        .nonce = 0,
        .code_hash = ethereum::EMPTY_CODE_HASH,
    };
  }
}

void MemoryAccountProvider::delete_account(const types::Address& address) {
  accounts_.erase(address);
}

// =============================================================================
// PRIVATE HELPERS
// =============================================================================

AccountData& MemoryAccountProvider::get_or_create_account(const types::Address& address) {
  auto it = accounts_.find(address);
  if (it != accounts_.end()) {
    return it->second;
  }

  // Create new account with defaults
  auto [inserted_it, _] = accounts_.emplace(address, AccountData{
                                                         .balance = types::Uint256::zero(),
                                                         .nonce = 0,
                                                         .code_hash = ethereum::EMPTY_CODE_HASH,
                                                     });
  return inserted_it->second;
}

}  // namespace div0::state
