#ifndef DIV0_DB_DATABASE_H
#define DIV0_DB_DATABASE_H

#include <optional>

#include "div0/types/bytes.h"

namespace div0::db {

using types::Bytes;

class Database {
 public:
  Database() = default;
  virtual ~Database() = default;

  // non copyable, non moveable
  Database(const Database& other) = delete;
  Database(Database&& other) noexcept = delete;
  Database& operator=(const Database& other) = delete;
  Database& operator=(Database&& other) noexcept = delete;

  /**
   * @brief Retrieve a value by key.
   *
   * @param key Key to look up
   * @return Value if found, std::nullopt otherwise
   */
  [[nodiscard]] virtual std::optional<Bytes> get(const Bytes& key) const = 0;

  /**
   * @brief Store a key-value pair.
   *
   * @param key Key to store
   * @param value Value to store
   */
  virtual void put(const Bytes& key, const Bytes& value) = 0;

  /**
   * @brief Delete a key-value pair.
   *
   * @param key Key to delete
   */
  virtual void erase(const Bytes& key) = 0;
};

}  // namespace div0::db

#endif  // DIV0_DB_DATABASE_H
