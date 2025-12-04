#include "div0/db/memory_database.h"

namespace div0::db {

std::optional<Bytes> MemoryDatabase::get(const Bytes& key) const {
  const auto it = data_.find(key);
  if (it != data_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void MemoryDatabase::put(const Bytes& key, const Bytes& value) { data_[key] = value; }

void MemoryDatabase::erase(const Bytes& key) { data_.erase(key); }

}  // namespace div0::db
