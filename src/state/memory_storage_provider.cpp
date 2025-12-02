#include "div0/state/memory_storage_provider.h"

#include <array>
#include <cassert>
#include <cstring>

#include "div0/utils/bytes.h"

namespace div0::state {

MemoryStorageProvider::MemoryStorageProvider(db::Database& database) : database_(database) {}

types::StorageValue MemoryStorageProvider::load(types::Address address, types::StorageSlot slot) {
  const auto key = make_storage_key(address, slot);
  const auto result = database_.get(key);

  warm_slots_.insert(key);

  if (!result.has_value()) {
    return types::StorageValue{types::Uint256::zero()};
  }

  return types::StorageValue{decode_uint256(result.value())};
}

void MemoryStorageProvider::store(types::Address address, types::StorageSlot slot,
                                  types::StorageValue value) {
  const auto key = make_storage_key(address, slot);
  warm_slots_.insert(key);

  database_.put(key, encode_uint256(value.get()));
}

bool MemoryStorageProvider::is_warm(types::Address address, types::StorageSlot slot) {
  return warm_slots_.contains(make_storage_key(address, slot));
}

void MemoryStorageProvider::begin_transaction() { warm_slots_.clear(); }

std::string MemoryStorageProvider::encode_uint256(const types::Uint256& value) {
  std::string result(32, '\0');

  // Convert each limb to big-endian and write 8 bytes
  const std::array swapped = utils::swap_endian_256(value.data_unsafe());

  // Copy bytes using memcpy (type-safe, no pointer arithmetic)
  std::memcpy(result.data(), swapped.data(), 32);

  return result;
}

types::Uint256 MemoryStorageProvider::decode_uint256(const std::string_view data) {
  assert(data.length() == 32 && "data provided by the database must always be 32 bytes");

  // Read 4 uint64_t values from the raw bytes
  std::array<uint64_t, 4> raw = {};
  std::memcpy(raw.data(), data.data(), 32);

  return types::Uint256(utils::swap_endian_256(raw));
}

std::string MemoryStorageProvider::make_storage_key(const types::Address& address,
                                                    const types::StorageSlot& slot) {
  return encode_uint256(address.to_uint256()) + encode_uint256(slot.get());
}

}  // namespace div0::state
