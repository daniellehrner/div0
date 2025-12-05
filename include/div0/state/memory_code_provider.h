#ifndef DIV0_STATE_MEMORY_CODE_PROVIDER_H
#define DIV0_STATE_MEMORY_CODE_PROVIDER_H

#include <unordered_map>

#include "div0/state/code_provider.h"
#include "div0/types/bytes.h"

namespace div0::state {

/**
 * @brief In-memory code provider for testing.
 *
 * Stores code in a hash map keyed by address.
 *
 * IMPORTANT: The spans returned by get_code() point directly into the internal
 * hash map storage. Callers must not:
 * - Call set_code() while any returned span is in use
 * - Modify code_ from another thread during execution
 *
 * This is safe for EVM execution because code is set up before execute() and
 * the map is not mutated during bytecode interpretation.
 */
class MemoryCodeProvider final : public CodeProvider {
 public:
  MemoryCodeProvider() = default;
  ~MemoryCodeProvider() override = default;

  MemoryCodeProvider(const MemoryCodeProvider& other) = delete;
  MemoryCodeProvider(MemoryCodeProvider&& other) = delete;
  MemoryCodeProvider& operator=(const MemoryCodeProvider& other) = delete;
  MemoryCodeProvider& operator=(MemoryCodeProvider&& other) = delete;

  [[nodiscard]] std::span<const uint8_t> get_code(const types::Address& address) override {
    auto it = code_.find(address);
    if (it == code_.end()) {
      return {};
    }
    return it->second;
  }

  [[nodiscard]] size_t get_code_size(const types::Address& address) override {
    auto it = code_.find(address);
    if (it == code_.end()) {
      return 0;
    }
    return it->second.size();
  }

  /// Set code for an address (for test setup)
  void set_code(const types::Address& address, types::Bytes code) {
    code_[address] = std::move(code);
  }

 private:
  std::unordered_map<types::Address, types::Bytes, types::AddressHash> code_;
};

}  // namespace div0::state

#endif  // DIV0_STATE_MEMORY_CODE_PROVIDER_H
