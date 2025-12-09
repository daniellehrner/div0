// state_builder.h - Build state providers from alloc

#ifndef DIV0_CLI_T8N_STATE_BUILDER_H
#define DIV0_CLI_T8N_STATE_BUILDER_H

#include <memory>

#include "div0/db/memory_database.h"
#include "div0/state/memory_account_provider.h"
#include "div0/state/memory_code_provider.h"
#include "div0/state/memory_storage_provider.h"
#include "div0/state/state_context.h"
#include "t8n/input.h"

namespace div0::cli {

/// Owns all state providers and database for a t8n execution.
/// Provides unified access to accounts, code, and storage.
class T8nState {
 public:
  /// Build state from alloc
  explicit T8nState(const std::map<types::Address, AllocAccount>& alloc);
  ~T8nState() = default;

  // Non-copyable, non-movable (owns references)
  T8nState(const T8nState&) = delete;
  T8nState& operator=(const T8nState&) = delete;
  T8nState(T8nState&&) = delete;
  T8nState& operator=(T8nState&&) = delete;

  /// Get state context for EVM
  [[nodiscard]] state::StateContext& context() { return context_; }

  /// Get account provider for balance/nonce operations
  [[nodiscard]] state::MemoryAccountProvider& accounts() { return accounts_; }
  [[nodiscard]] const state::MemoryAccountProvider& accounts() const { return accounts_; }

  /// Get code provider for contract code
  [[nodiscard]] state::MemoryCodeProvider& code() { return code_; }
  [[nodiscard]] const state::MemoryCodeProvider& code() const { return code_; }

  /// Get storage provider for contract storage
  [[nodiscard]] state::MemoryStorageProvider& storage() { return storage_; }
  [[nodiscard]] const state::MemoryStorageProvider& storage() const { return storage_; }

 private:
  db::MemoryDatabase database_;
  state::MemoryStorageProvider storage_;
  state::MemoryAccountProvider accounts_;
  state::MemoryCodeProvider code_;
  state::StateContext context_;
};

/// Build state from alloc
[[nodiscard]] std::unique_ptr<T8nState> build_state(
    const std::map<types::Address, AllocAccount>& alloc);

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_STATE_BUILDER_H
