#ifndef DIV0_STATE_MEMORY_STORAGE_PROVIDER_H
#define DIV0_STATE_MEMORY_STORAGE_PROVIDER_H

#include <map>
#include <unordered_set>

#include "div0/db/memory_database.h"
#include "div0/state/storage_provider.h"
#include "div0/types/bytes.h"

namespace div0::state {

/// Storage entry: address -> slot -> value
using StorageMap =
    std::map<types::Address, std::map<ethereum::StorageSlot, ethereum::StorageValue>>;

class MemoryStorageProvider : public StorageProvider {
 public:
  explicit MemoryStorageProvider(db::Database& database);
  ~MemoryStorageProvider() override = default;

  // non copyable, non moveable
  MemoryStorageProvider(const MemoryStorageProvider& other) = delete;
  MemoryStorageProvider(MemoryStorageProvider&& other) noexcept = delete;
  MemoryStorageProvider& operator=(const MemoryStorageProvider& other) = delete;
  MemoryStorageProvider& operator=(MemoryStorageProvider&& other) noexcept = delete;

  ethereum::StorageValue load(types::Address address, ethereum::StorageSlot slot) override;

  void store(types::Address address, ethereum::StorageSlot slot,
             ethereum::StorageValue value) override;

  [[nodiscard]] bool is_warm(types::Address address, ethereum::StorageSlot slot) override;

  void begin_transaction() override;

  /// Get all stored slots
  /// Returns a map of address -> (slot -> value) for all non-zero storage
  [[nodiscard]] const StorageMap& storage() const { return storage_; }

 private:
  // Serialization helpers
  [[nodiscard]] static types::Bytes encode_uint256(const types::Uint256& value);
  [[nodiscard]] static types::Uint256 decode_uint256(const types::Bytes& data);
  [[nodiscard]] static types::Bytes make_storage_key(const types::Address& address,
                                                     const ethereum::StorageSlot& slot);

  db::Database& database_;
  std::unordered_set<types::Bytes> warm_slots_;
  StorageMap storage_;  // In-memory copy for iteration
};

}  // namespace div0::state

#endif  // DIV0_STATE_MEMORY_STORAGE_PROVIDER_H
