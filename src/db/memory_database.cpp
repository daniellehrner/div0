#include "div0/db/memory_database.h"

namespace div0::db {

std::optional<std::string> MemoryDatabase::get(const std::string_view key) const {
  const auto it = data_.find(std::string(key));
  if (it != data_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void MemoryDatabase::put(const std::string_view key, const std::string_view value) {
  data_[std::string(key)] = std::string(value);
}

void MemoryDatabase::erase(const std::string_view key) { data_.erase(std::string(key)); }

}  // namespace div0::db
