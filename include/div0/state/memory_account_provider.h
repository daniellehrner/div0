#ifndef DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H
#define DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H

#include <unordered_set>

#include "div0/state/account_provider.h"

namespace div0::state {

/// In-memory account provider for testing.
/// Tracks warm addresses and provides stub implementations for account state.
class MemoryAccountProvider final : public AccountProvider {
 public:
  MemoryAccountProvider() = default;
  ~MemoryAccountProvider() override = default;

  MemoryAccountProvider(const MemoryAccountProvider& other) = delete;
  MemoryAccountProvider(MemoryAccountProvider&& other) = delete;
  MemoryAccountProvider& operator=(const MemoryAccountProvider& other) = delete;
  MemoryAccountProvider& operator=(MemoryAccountProvider&& other) = delete;

  [[nodiscard]] bool is_warm(const types::Address& address) override {
    return warm_addresses_.contains(address);
  }

  bool warm_address(const types::Address& address) override {
    auto [_, inserted] = warm_addresses_.insert(address);
    return inserted;  // true if was cold (newly inserted)
  }

  [[nodiscard]] bool is_empty(const types::Address& /*address*/) override {
    // For now, all accounts are considered non-empty
    // TODO: Implement proper account state tracking
    return false;
  }

  [[nodiscard]] bool exists(const types::Address& /*address*/) override {
    // For now, all accounts exist
    // TODO: Implement proper account state tracking
    return true;
  }

  void begin_transaction() override { warm_addresses_.clear(); }

 private:
  std::unordered_set<types::Address, types::AddressHash> warm_addresses_;
};

}  // namespace div0::state

#endif  // DIV0_STATE_MEMORY_ACCOUNT_PROVIDER_H
