#ifndef DIV0_STATE_STATE_CONTEXT_H
#define DIV0_STATE_STATE_CONTEXT_H

#include "div0/state/account_provider.h"
#include "div0/state/code_provider.h"
#include "div0/state/storage_provider.h"

namespace div0::state {

/**
 * @brief Container for all state providers.
 *
 * Aggregates references to storage, account, and code providers.
 * Passed to EVM constructor to provide state access.
 */
struct StateContext {
  StateContext(StorageProvider& storage, AccountProvider& accounts, CodeProvider& code)
      : storage(storage), accounts(accounts), code(code) {}
  ~StateContext() = default;

  // non copyable, non moveable
  StateContext(const StateContext& other) = delete;
  StateContext(StateContext&& other) = delete;
  StateContext& operator=(const StateContext& other) = delete;
  StateContext& operator=(StateContext&& other) = delete;

  /// Begin a new transaction - resets warm/cold access tracking in all providers
  void begin_transaction() {
    storage.begin_transaction();
    accounts.begin_transaction();
  }

  StorageProvider& storage;
  AccountProvider& accounts;
  CodeProvider& code;
};

}  // namespace div0::state

#endif  // DIV0_STATE_STATE_CONTEXT_H
