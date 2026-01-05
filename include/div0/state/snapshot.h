#ifndef DIV0_STATE_SNAPSHOT_H
#define DIV0_STATE_SNAPSHOT_H

/// @file snapshot.h
/// @brief Generic state snapshot types for serializing world state.
///
/// These types represent a flat, serializable view of account state.
/// Used for exporting post-state after execution (e.g., for ZK proving,
/// JSON serialization, or other outputs).

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Types
// ============================================================================

/// Storage slot-value pair.
typedef struct {
  uint256_t slot;  ///< Storage slot key
  uint256_t value; ///< Storage value
} storage_entry_t;

/// Account snapshot with concrete values (not hashes).
typedef struct {
  address_t address;        ///< Account address
  uint256_t balance;        ///< Account balance
  uint64_t nonce;           ///< Account nonce
  bytes_t code;             ///< Contract bytecode (arena-allocated)
  storage_entry_t *storage; ///< Storage entries (arena-allocated array)
  size_t storage_count;     ///< Number of storage entries
} account_snapshot_t;

/// State snapshot containing all accounts.
typedef struct {
  account_snapshot_t *accounts; ///< Account array (arena-allocated)
  size_t account_count;         ///< Number of accounts
} state_snapshot_t;

#endif // DIV0_STATE_SNAPSHOT_H
