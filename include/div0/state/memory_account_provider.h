#ifndef DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H
#define DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H

#include <unordered_map>
#include <unordered_set>

#include "div0/state/account_provider.h"
#include "div0/types/hash.h"

namespace div0::state {

/// Account data stored in memory.
struct AccountData {
  types::Uint256 balance;
  uint64_t nonce{0};
  types::Hash code_hash;  // EMPTY_CODE_HASH if no code
};

/// In-memory account provider with full account state support.
/// Tracks warm addresses, balances, nonces, and code hashes.
class MemoryAccountProvider final : public AccountProvider {
 public:
  MemoryAccountProvider();
  ~MemoryAccountProvider() override = default;

  MemoryAccountProvider(const MemoryAccountProvider& other) = delete;
  MemoryAccountProvider(MemoryAccountProvider&& other) = delete;
  MemoryAccountProvider& operator=(const MemoryAccountProvider& other) = delete;
  MemoryAccountProvider& operator=(MemoryAccountProvider&& other) = delete;

  // ═══════════════════════════════════════════════════════════════════════════
  // EIP-2929 WARM/COLD TRACKING
  // ═══════════════════════════════════════════════════════════════════════════

  [[nodiscard]] bool is_warm(const types::Address& address) override;
  bool warm_address(const types::Address& address) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // ACCOUNT EXISTENCE CHECKS
  // ═══════════════════════════════════════════════════════════════════════════

  [[nodiscard]] bool is_empty(const types::Address& address) override;
  [[nodiscard]] bool exists(const types::Address& address) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // TRANSACTION BOUNDARY
  // ═══════════════════════════════════════════════════════════════════════════

  void begin_transaction() override;

  // ═══════════════════════════════════════════════════════════════════════════
  // BALANCE OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  [[nodiscard]] types::Uint256 get_balance(const types::Address& address) override;
  void set_balance(const types::Address& address, const types::Uint256& balance) override;
  [[nodiscard]] bool add_balance(const types::Address& address,
                                 const types::Uint256& amount) override;
  [[nodiscard]] bool subtract_balance(const types::Address& address,
                                      const types::Uint256& amount) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // NONCE OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  [[nodiscard]] uint64_t get_nonce(const types::Address& address) override;
  void set_nonce(const types::Address& address, uint64_t nonce) override;
  uint64_t increment_nonce(const types::Address& address) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // CODE HASH OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  [[nodiscard]] types::Hash get_code_hash(const types::Address& address) override;
  void set_code_hash(const types::Address& address, const types::Hash& code_hash) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // ACCOUNT LIFECYCLE
  // ═══════════════════════════════════════════════════════════════════════════

  void create_account(const types::Address& address) override;
  void delete_account(const types::Address& address) override;

  // ═══════════════════════════════════════════════════════════════════════════
  // STATE ACCESS (for state root computation)
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get all accounts for state root computation.
  [[nodiscard]] const std::unordered_map<types::Address, AccountData, types::AddressHash>&
  accounts() const {
    return accounts_;
  }

 private:
  /// Get or create account, returning reference to account data.
  AccountData& get_or_create_account(const types::Address& address);

  std::unordered_map<types::Address, AccountData, types::AddressHash> accounts_;
  std::unordered_set<types::Address, types::AddressHash> warm_addresses_;
};

}  // namespace div0::state

#endif  // DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H
