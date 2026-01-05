#ifndef DIV0_STATE_WORLD_STATE_H
#define DIV0_STATE_WORLD_STATE_H

#include "div0/mem/arena.h"
#include "div0/state/account.h"
#include "div0/state/state_access.h"
#include "div0/trie/mpt.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>

/// World state - manages account state trie and per-account storage.
/// Implements the state_access_t interface.
typedef struct {
  state_access_t base; // vtable (must be first for casting)

  mpt_backend_t *state_backend; // Backend for state trie
  mpt_t state_trie;             // Account state trie: keccak(addr) -> RLP(account)

  // Hash tables for additional account data
  void *storage_tries; // address -> mpt_t* (storage trie)
  void *code_store;    // address -> bytes_t (contract code)

  // EIP-2929 access tracking
  void *warm_addresses; // Set of warm addresses
  void *warm_slots;     // Set of warm (address, slot) pairs

  // EIP-2200 original storage values (for gas calculation)
  void *original_storage; // (address, slot) -> original uint256 value

  // Dirty tracking for efficient state root computation
  void *dirty_storage; // Set of addresses with modified storage

  // All accounts tracking for post-state export
  void *all_accounts; // Set of all addresses in state

  // All storage slots tracking for post-state export
  void *all_storage_slots; // Map of (address, slot) -> slot for all written slots

  // Snapshot support
  uint64_t snapshot_counter; // Per-instance snapshot ID counter

  div0_arena_t *arena; // Arena for all allocations
} world_state_t;

/// Create a new empty world state backed by in-memory MPT.
/// @param arena Arena for all allocations (owned by caller)
/// @return Initialized world state, or nullptr on failure
[[nodiscard]] world_state_t *world_state_create(div0_arena_t *arena);

/// Get state access interface for EVM.
/// @param ws World state
/// @return State access interface (valid while world_state exists)
[[nodiscard]] static inline state_access_t *world_state_access(world_state_t *ws) {
  return &ws->base;
}

/// Get account from state trie.
/// @param ws World state
/// @param addr Address to look up
/// @param out Output account (filled with empty account if not found)
/// @return true if account exists, false if not found
[[nodiscard]] bool world_state_get_account(world_state_t *ws, const address_t *addr,
                                           account_t *out);

/// Set account in state trie.
/// If account is empty (EIP-161), it is deleted from the trie.
/// @param ws World state
/// @param addr Address
/// @param acc Account to store
/// @return true on success
bool world_state_set_account(world_state_t *ws, const address_t *addr, const account_t *acc);

/// Get storage trie for an account.
/// Creates an empty storage trie if it doesn't exist.
/// @param ws World state
/// @param addr Account address
/// @return Storage trie (never nullptr, creates if needed)
[[nodiscard]] mpt_t *world_state_get_storage_trie(world_state_t *ws, const address_t *addr);

/// Compute current state root.
/// This updates all dirty storage roots and recomputes the state trie root.
/// @param ws World state
/// @return State root hash
[[nodiscard]] hash_t world_state_root(world_state_t *ws);

/// Clear all state (reset to empty).
/// @param ws World state
void world_state_clear(world_state_t *ws);

/// Destroy world state and free resources.
/// Note: The arena is NOT destroyed (owned by caller).
/// @param ws World state
void world_state_destroy(world_state_t *ws);

// State snapshot types for post-state export
#include "div0/state/snapshot.h"

/// Export world state to a snapshot.
/// Iterates over all accounts in the state and builds the snapshot structure.
/// @param ws World state
/// @param arena Arena for allocations
/// @param out Output snapshot structure
/// @return true on success
bool world_state_snapshot(world_state_t *ws, div0_arena_t *arena, state_snapshot_t *out);

#endif // DIV0_STATE_WORLD_STATE_H
