#ifndef DIV0_DB_MEMORY_DATABASE_H
#define DIV0_DB_MEMORY_DATABASE_H

#include <map>

#include "div0/db/database.h"

namespace div0::db {

/**
 * @brief In-memory key-value database for testing.
 *
 * Simple std::map-based implementation.
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

  [[nodiscard]] std::optional<Bytes> get(const Bytes& key) const override;

  void put(const Bytes& key, const Bytes& value) override;

  void erase(const Bytes& key) override;

 private:
  std::map<Bytes, Bytes> data_;
};

}  // namespace div0::db

#endif  // DIV0_DB_MEMORY_DATABASE_H
