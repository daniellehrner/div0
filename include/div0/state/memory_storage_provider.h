#ifndef DIV0_STATE_MEMORY_STORAGE_PROVIDER_H
#define DIV0_STATE_MEMORY_STORAGE_PROVIDER_H

#include <unordered_set>

#include "div0/db/memory_database.h"
#include "div0/state/storage_provider.h"

namespace div0::state {

class MemoryStorageProvider : public StorageProvider {
 public:
  explicit MemoryStorageProvider(db::Database& database);
  ~MemoryStorageProvider() override = default;

  // non copyable, non moveable
  MemoryStorageProvider(const MemoryStorageProvider& other) = delete;
  MemoryStorageProvider(MemoryStorageProvider&& other) noexcept = delete;
  MemoryStorageProvider& operator=(const MemoryStorageProvider& other) = delete;
  MemoryStorageProvider& operator=(MemoryStorageProvider&& other) noexcept = delete;

  types::StorageValue load(types::Address address, types::StorageSlot slot) override;

  void store(types::Address address, types::StorageSlot slot, types::StorageValue value) override;

  [[nodiscard]] bool is_warm(types::Address address, types::StorageSlot slot) override;

  void begin_transaction() override;

 private:
  // Serialization helpers
  [[nodiscard]] static std::string encode_uint256(const types::Uint256& value);
  [[nodiscard]] static types::Uint256 decode_uint256(std::string_view data);
  [[nodiscard]] static std::string make_storage_key(const types::Address& address,
                                                    const types::StorageSlot& slot);

  db::Database& database_;
  std::unordered_set<std::string> warm_slots_;
};

}  // namespace div0::state

#endif  // DIV0_STATE_MEMORY_STORAGE_PROVIDER_H
