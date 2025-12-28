#ifndef DIV0_ETHEREUM_TRANSACTION_ACCESS_LIST_H
#define DIV0_ETHEREUM_TRANSACTION_ACCESS_LIST_H

#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

/// Access list entry: an address with its pre-warmed storage slots.
/// Used in EIP-2930+ transactions for gas discounts.
typedef struct {
  address_t address;
  uint256_t *storage_keys; // Arena-allocated array
  size_t storage_keys_count;
} access_list_entry_t;

/// Access list: array of entries.
typedef struct {
  access_list_entry_t *entries; // Arena-allocated array
  size_t count;
} access_list_t;

/// Initializes an empty access list.
static inline void access_list_init(access_list_t *list) {
  list->entries = nullptr;
  list->count = 0;
}

/// Returns total storage key count across all entries.
static inline size_t access_list_total_keys(const access_list_t *list) {
  size_t total = 0;
  for (size_t i = 0; i < list->count; i++) {
    total += list->entries[i].storage_keys_count;
  }
  return total;
}

/// Allocates space for `count` entries in the access list.
/// @param list Access list to allocate
/// @param count Number of entries
/// @param arena Arena for allocation
/// @return true on success, false on allocation failure
static inline bool access_list_alloc_entries(access_list_t *list, size_t count,
                                             div0_arena_t *arena) {
  if (count == 0) {
    list->entries = nullptr;
    list->count = 0;
    return true;
  }
  list->entries =
      (access_list_entry_t *)div0_arena_alloc(arena, count * sizeof(access_list_entry_t));
  if (!list->entries) {
    return false;
  }
  list->count = count;
  return true;
}

/// Allocates space for storage keys in an entry.
/// @param entry Entry to allocate keys for
/// @param count Number of keys
/// @param arena Arena for allocation
/// @return true on success, false on allocation failure
static inline bool access_list_entry_alloc_keys(access_list_entry_t *entry, size_t count,
                                                div0_arena_t *arena) {
  if (count == 0) {
    entry->storage_keys = nullptr;
    entry->storage_keys_count = 0;
    return true;
  }
  entry->storage_keys = (uint256_t *)div0_arena_alloc(arena, count * sizeof(uint256_t));
  if (!entry->storage_keys) {
    return false;
  }
  entry->storage_keys_count = count;
  return true;
}

#endif // DIV0_ETHEREUM_TRANSACTION_ACCESS_LIST_H
