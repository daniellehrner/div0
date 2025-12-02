#ifndef DIV0_DB_DATABASE_H
#define DIV0_DB_DATABASE_H

#include <optional>
#include <string>
#include <string_view>

namespace div0::db {

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
  [[nodiscard]] virtual std::optional<std::string> get(std::string_view key) const = 0;

  /**
   * @brief Store a key-value pair.
   *
   * @param key Key to store
   * @param value Value to store
   */
  virtual void put(std::string_view key, std::string_view value) = 0;

  /**
   * @brief Delete a key-value pair.
   *
   * @param key Key to delete
   */
  virtual void erase(std::string_view key) = 0;
};

}  // namespace div0::db

#endif  // DIV0_DB_DATABASE_H
