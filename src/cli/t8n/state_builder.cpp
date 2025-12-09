// state_builder.cpp - Build state providers from alloc

#include "t8n/state_builder.h"

#include "div0/crypto/keccak256.h"
#include "div0/ethereum/account.h"
#include "div0/ethereum/storage_slot.h"

namespace div0::cli {

T8nState::T8nState(const std::map<types::Address, AllocAccount>& alloc)
    : storage_(database_), context_(storage_, accounts_, code_) {
  // Populate providers from alloc
  for (const auto& [address, account] : alloc) {
    // Set balance
    accounts_.set_balance(address, account.balance);

    // Set nonce
    accounts_.set_nonce(address, account.nonce);

    // Set code and compute code hash
    if (!account.code.empty()) {
      code_.set_code(address, account.code);
      auto code_hash = crypto::keccak256(account.code);
      accounts_.set_code_hash(address, code_hash);
    } else {
      accounts_.set_code_hash(address, ethereum::EMPTY_CODE_HASH);
    }

    // Set storage slots
    for (const auto& [slot, value] : account.storage) {
      storage_.store(address, ethereum::StorageSlot(slot), ethereum::StorageValue(value));
    }
  }
}

std::unique_ptr<T8nState> build_state(const std::map<types::Address, AllocAccount>& alloc) {
  return std::make_unique<T8nState>(alloc);
}

}  // namespace div0::cli
