#ifndef DIV0_STATE_STATE_ACCESS_H
#define DIV0_STATE_STATE_ACCESS_H

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Forward declarations
typedef struct state_access state_access_t;

/// State access vtable - interface for EVM state operations.
/// Follows the vtable pattern used by mpt_backend_vtable_t.
typedef struct {
  // ===========================================================================
  // ACCOUNT EXISTENCE
  // ===========================================================================

  /// Check if account exists in state.
  /// @param state State access instance
  /// @param addr Address to check
  /// @return true if account exists
  bool (*account_exists)(state_access_t *state, const address_t *addr);

  /// Check if account is empty (EIP-161: no code, zero nonce, zero balance).
  /// @param state State access instance
  /// @param addr Address to check
  /// @return true if account is empty or non-existent
  bool (*account_is_empty)(state_access_t *state, const address_t *addr);

  /// Create a contract account at address with nonce=1.
  /// Per EIP-161, nonce=1 ensures the account is non-empty.
  /// No-op if account already exists.
  /// @param state State access instance
  /// @param addr Address for new contract account
  void (*create_contract)(state_access_t *state, const address_t *addr);

  /// Delete an account (SELFDESTRUCT).
  /// @param state State access instance
  /// @param addr Address to delete
  void (*delete_account)(state_access_t *state, const address_t *addr);

  // ===========================================================================
  // BALANCE OPERATIONS
  // ===========================================================================

  /// Get account balance. Returns zero for non-existent accounts.
  /// @param state State access instance
  /// @param addr Address to query
  /// @return Balance in wei
  uint256_t (*get_balance)(state_access_t *state, const address_t *addr);

  /// Set account balance. Creates account if it doesn't exist.
  /// @param state State access instance
  /// @param addr Address to update
  /// @param balance New balance
  void (*set_balance)(state_access_t *state, const address_t *addr, uint256_t balance);

  /// Add to account balance.
  /// @param state State access instance
  /// @param addr Address to update
  /// @param amount Amount to add
  /// @return true on success, false on overflow
  bool (*add_balance)(state_access_t *state, const address_t *addr, uint256_t amount);

  /// Subtract from account balance.
  /// @param state State access instance
  /// @param addr Address to update
  /// @param amount Amount to subtract
  /// @return true on success, false if insufficient balance
  bool (*sub_balance)(state_access_t *state, const address_t *addr, uint256_t amount);

  // ===========================================================================
  // NONCE OPERATIONS
  // ===========================================================================

  /// Get account nonce. Returns zero for non-existent accounts.
  /// @param state State access instance
  /// @param addr Address to query
  /// @return Transaction/creation nonce
  uint64_t (*get_nonce)(state_access_t *state, const address_t *addr);

  /// Set account nonce. Creates account if needed.
  /// @param state State access instance
  /// @param addr Address to update
  /// @param nonce New nonce value
  void (*set_nonce)(state_access_t *state, const address_t *addr, uint64_t nonce);

  /// Increment account nonce by 1.
  /// @param state State access instance
  /// @param addr Address to update
  /// @return Old nonce value before increment
  uint64_t (*increment_nonce)(state_access_t *state, const address_t *addr);

  // ===========================================================================
  // CODE OPERATIONS
  // ===========================================================================

  /// Get code for address. Returns empty bytes if no code.
  /// The returned bytes are valid until state is modified.
  /// @param state State access instance
  /// @param addr Address to query
  /// @return Code bytes (may be empty)
  bytes_t (*get_code)(state_access_t *state, const address_t *addr);

  /// Get code size for address.
  /// @param state State access instance
  /// @param addr Address to query
  /// @return Code size in bytes
  size_t (*get_code_size)(state_access_t *state, const address_t *addr);

  /// Get code hash for address. Returns EMPTY_CODE_HASH if no code.
  /// @param state State access instance
  /// @param addr Address to query
  /// @return keccak256 of code
  hash_t (*get_code_hash)(state_access_t *state, const address_t *addr);

  /// Set code for address. Computes and stores code_hash.
  /// @param state State access instance
  /// @param addr Address to update
  /// @param code Code bytes
  /// @param code_len Code length
  void (*set_code)(state_access_t *state, const address_t *addr, const uint8_t *code,
                   size_t code_len);

  // ===========================================================================
  // STORAGE OPERATIONS (SLOAD/SSTORE)
  // ===========================================================================

  /// Load storage slot value. Returns zero for unset slots.
  /// @param state State access instance
  /// @param addr Contract address
  /// @param slot Storage slot key
  /// @return Storage value
  uint256_t (*get_storage)(state_access_t *state, const address_t *addr, uint256_t slot);

  /// Get original storage value (value at start of transaction).
  /// Used for EIP-2200/EIP-3529 gas calculation.
  /// @param state State access instance
  /// @param addr Contract address
  /// @param slot Storage slot key
  /// @return Original storage value before any modifications this tx
  uint256_t (*get_original_storage)(state_access_t *state, const address_t *addr, uint256_t slot);

  /// Store storage slot value.
  /// @param state State access instance
  /// @param addr Contract address
  /// @param slot Storage slot key
  /// @param value Value to store
  void (*set_storage)(state_access_t *state, const address_t *addr, uint256_t slot,
                      uint256_t value);

  // ===========================================================================
  // EIP-2929 WARM/COLD ACCESS (Shanghai+)
  // ===========================================================================

  /// Check if address is warm (accessed this transaction).
  /// @param state State access instance
  /// @param addr Address to check
  /// @return true if address is warm
  bool (*is_address_warm)(state_access_t *state, const address_t *addr);

  /// Mark address as warm.
  /// @param state State access instance
  /// @param addr Address to warm
  /// @return true if address was cold (first access this tx), false if already warm
  bool (*warm_address)(state_access_t *state, const address_t *addr);

  /// Check if storage slot is warm.
  /// @param state State access instance
  /// @param addr Contract address
  /// @param slot Storage slot
  /// @return true if slot is warm
  bool (*is_slot_warm)(state_access_t *state, const address_t *addr, uint256_t slot);

  /// Mark storage slot as warm.
  /// @param state State access instance
  /// @param addr Contract address
  /// @param slot Storage slot
  /// @return true if slot was cold (first access this tx), false if already warm
  bool (*warm_slot)(state_access_t *state, const address_t *addr, uint256_t slot);

  // ===========================================================================
  // TRANSACTION BOUNDARY
  // ===========================================================================

  /// Begin a new transaction - clears warm address/slot sets and original storage.
  /// @param state State access instance
  void (*begin_transaction)(state_access_t *state);

  // ===========================================================================
  // SNAPSHOT/REVERT (for nested calls)
  // ===========================================================================

  /// Take a snapshot of current state for potential revert.
  /// Used to implement CALL/CREATE revert semantics.
  /// @param state State access instance
  /// @return Snapshot ID to pass to revert
  uint64_t (*snapshot)(state_access_t *state);

  /// Revert state to a previous snapshot.
  /// All changes after the snapshot are discarded.
  /// @param state State access instance
  /// @param snapshot_id ID from a previous snapshot call
  void (*revert_to_snapshot)(state_access_t *state, uint64_t snapshot_id);

  /// Commit changes since last snapshot (discard snapshot).
  /// @param state State access instance
  /// @param snapshot_id ID from a previous snapshot call
  void (*commit_snapshot)(state_access_t *state, uint64_t snapshot_id);

  // ===========================================================================
  // STATE ROOT
  // ===========================================================================

  /// Compute current state root hash.
  /// This updates all dirty storage roots and recomputes the state trie root.
  /// @param state State access instance
  /// @return State root hash
  hash_t (*state_root)(state_access_t *state);

  // ===========================================================================
  // LIFECYCLE
  // ===========================================================================

  /// Destroy state access and free resources.
  /// @param state State access instance
  void (*destroy)(state_access_t *state);

} state_access_vtable_t;

/// Base structure for state access.
/// Concrete implementations embed this as first member.
struct state_access {
  const state_access_vtable_t *vtable;
};

// =============================================================================
// CONVENIENCE INLINE FUNCTIONS
// =============================================================================

/// Check if account exists.
[[nodiscard]] static inline bool state_account_exists(state_access_t *state,
                                                      const address_t *addr) {
  return state->vtable->account_exists(state, addr);
}

/// Get account balance.
[[nodiscard]] static inline uint256_t state_get_balance(state_access_t *state,
                                                        const address_t *addr) {
  return state->vtable->get_balance(state, addr);
}

/// Set account balance.
static inline void state_set_balance(state_access_t *state, const address_t *addr,
                                     uint256_t balance) {
  state->vtable->set_balance(state, addr, balance);
}

/// Get account nonce.
[[nodiscard]] static inline uint64_t state_get_nonce(state_access_t *state, const address_t *addr) {
  return state->vtable->get_nonce(state, addr);
}

/// Get storage value.
[[nodiscard]] static inline uint256_t state_get_storage(state_access_t *state,
                                                        const address_t *addr, uint256_t slot) {
  return state->vtable->get_storage(state, addr, slot);
}

/// Get original storage value (for EIP-2200 gas calculation).
[[nodiscard]] static inline uint256_t
state_get_original_storage(state_access_t *state, const address_t *addr, uint256_t slot) {
  return state->vtable->get_original_storage(state, addr, slot);
}

/// Set storage value.
static inline void state_set_storage(state_access_t *state, const address_t *addr, uint256_t slot,
                                     uint256_t value) {
  state->vtable->set_storage(state, addr, slot, value);
}

/// Get code.
[[nodiscard]] static inline bytes_t state_get_code(state_access_t *state, const address_t *addr) {
  return state->vtable->get_code(state, addr);
}

/// Get code hash.
[[nodiscard]] static inline hash_t state_get_code_hash(state_access_t *state,
                                                       const address_t *addr) {
  return state->vtable->get_code_hash(state, addr);
}

/// Compute state root.
[[nodiscard]] static inline hash_t state_root(state_access_t *state) {
  return state->vtable->state_root(state);
}

/// Warm an address for EIP-2929.
[[nodiscard]] static inline bool state_warm_address(state_access_t *state, const address_t *addr) {
  return state->vtable->warm_address(state, addr);
}

/// Warm a storage slot for EIP-2929.
[[nodiscard]] static inline bool state_warm_slot(state_access_t *state, const address_t *addr,
                                                 uint256_t slot) {
  return state->vtable->warm_slot(state, addr, slot);
}

/// Check if account is empty.
[[nodiscard]] static inline bool state_account_is_empty(state_access_t *state,
                                                        const address_t *addr) {
  return state->vtable->account_is_empty(state, addr);
}

/// Create a contract account.
static inline void state_create_contract(state_access_t *state, const address_t *addr) {
  state->vtable->create_contract(state, addr);
}

/// Delete an account.
static inline void state_delete_account(state_access_t *state, const address_t *addr) {
  state->vtable->delete_account(state, addr);
}

/// Add to account balance.
[[nodiscard]] static inline bool state_add_balance(state_access_t *state, const address_t *addr,
                                                   uint256_t amount) {
  return state->vtable->add_balance(state, addr, amount);
}

/// Subtract from account balance.
[[nodiscard]] static inline bool state_sub_balance(state_access_t *state, const address_t *addr,
                                                   uint256_t amount) {
  return state->vtable->sub_balance(state, addr, amount);
}

/// Set account nonce.
static inline void state_set_nonce(state_access_t *state, const address_t *addr, uint64_t nonce) {
  state->vtable->set_nonce(state, addr, nonce);
}

/// Increment account nonce, returns old value.
[[nodiscard]] static inline uint64_t state_increment_nonce(state_access_t *state,
                                                           const address_t *addr) {
  return state->vtable->increment_nonce(state, addr);
}

/// Get code size.
[[nodiscard]] static inline size_t state_get_code_size(state_access_t *state,
                                                       const address_t *addr) {
  return state->vtable->get_code_size(state, addr);
}

/// Set code.
static inline void state_set_code(state_access_t *state, const address_t *addr, const uint8_t *code,
                                  size_t code_len) {
  state->vtable->set_code(state, addr, code, code_len);
}

/// Check if address is warm.
[[nodiscard]] static inline bool state_is_address_warm(state_access_t *state,
                                                       const address_t *addr) {
  return state->vtable->is_address_warm(state, addr);
}

/// Check if storage slot is warm.
[[nodiscard]] static inline bool state_is_slot_warm(state_access_t *state, const address_t *addr,
                                                    uint256_t slot) {
  return state->vtable->is_slot_warm(state, addr, slot);
}

/// Begin a new transaction.
static inline void state_begin_transaction(state_access_t *state) {
  state->vtable->begin_transaction(state);
}

/// Take a snapshot.
[[nodiscard]] static inline uint64_t state_snapshot(state_access_t *state) {
  return state->vtable->snapshot(state);
}

/// Revert to a snapshot.
static inline void state_revert_to_snapshot(state_access_t *state, uint64_t snapshot_id) {
  state->vtable->revert_to_snapshot(state, snapshot_id);
}

/// Commit a snapshot.
static inline void state_commit_snapshot(state_access_t *state, uint64_t snapshot_id) {
  state->vtable->commit_snapshot(state, snapshot_id);
}

/// Destroy state access.
static inline void state_destroy(state_access_t *state) {
  state->vtable->destroy(state);
}

#endif // DIV0_STATE_STATE_ACCESS_H
