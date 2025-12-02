#ifndef DIV0_DB_MEMORY_DATABASE_H
#define DIV0_DB_MEMORY_DATABASE_H

#include <string>
#include <unordered_map>

#include "div0/db/database.h"

namespace div0::db {

/**
 * @brief In-memory key-value database for testing.
 *
 * Simple std::unordered_map-based implementation.
 * Not thread-safe. Not persistent.
 */
class MemoryDatabase : public Database {
 public:
  MemoryDatabase() = default;
  ~MemoryDatabase() override = default;

  // non copyable, non moveable
  MemoryDatabase(const MemoryDatabase& other) = delete;
  MemoryDatabase(MemoryDatabase&& other) noexcept = delete;
  MemoryDatabase& operator=(const MemoryDatabase& other) = delete;
  MemoryDatabase& operator=(MemoryDatabase&& other) noexcept = delete;

  [[nodiscard]] std::optional<std::string> get(std::string_view key) const override;

  void put(std::string_view key, std::string_view value) override;

  void erase(std::string_view key) override;

 private:
  std::unordered_map<std::string, std::string> data_;
};

}  // namespace div0::db

#endif  // DIV0_DB_MEMORY_DATABASE_H
