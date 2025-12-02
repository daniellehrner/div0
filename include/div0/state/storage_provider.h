#ifndef DIV0_STATE_STORAGE_PROVIDER_H
#define DIV0_STATE_STORAGE_PROVIDER_H

#include "div0/types/address.h"
#include "div0/types/storage_slot.h"
#include "div0/types/storage_value.h"

namespace div0::state {
/**
 * @brief Abstract interface for EVM storage operations.
 *
 * Provides SLOAD/SSTORE functionality with address scoping.
 */
class StorageProvider {
 public:
  StorageProvider() = default;
  virtual ~StorageProvider() = default;

  // non copyable, non moveable
  StorageProvider(const StorageProvider& other) = delete;
  StorageProvider(StorageProvider&& other) = delete;
  StorageProvider& operator=(const StorageProvider& other) = delete;
  StorageProvider& operator=(StorageProvider&& other) = delete;

  /**
   * @brief Load a storage slot value (SLOAD).
   *
   * @param address Contract address
   * @param slot Storage slot key
   * @return Value at the slot (0 if not found)
   */
  virtual types::StorageValue load(types::Address address, types::StorageSlot slot) = 0;

  /**
   * @brief Store a value in a storage slot (SSTORE).
   *
   * @param address Contract address
   * @param slot Storage slot key
   * @param value Value to store
   */
  virtual void store(types::Address address, types::StorageSlot slot,
                     types::StorageValue value) = 0;

  virtual bool is_warm(types::Address address, types::StorageSlot slot) = 0;

  virtual void begin_transaction() = 0;
};

}  // namespace div0::state

#endif  // DIV0_STATE_STORAGE_PROVIDER_H
