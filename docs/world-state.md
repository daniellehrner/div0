# World State

The div0 world state manages Ethereum account state using Merkle Patricia Tries. It provides the complete state management layer for EVM execution, including account balances, nonces, contract code, storage, and EIP-2929 access tracking.

## Overview

### What is World State?

Ethereum's world state is a mapping from addresses to accounts. Each account contains:
- **Nonce**: Transaction count (EOA) or contract creation count
- **Balance**: Wei held by the account
- **Code Hash**: keccak256 of contract bytecode (empty for EOAs)
- **Storage Root**: Root hash of the account's storage trie

The world state provides:
- **Authenticated state**: Single 32-byte root commits to all accounts
- **Per-account storage**: Each contract has its own storage trie
- **Gas-accurate access tracking**: EIP-2929 warm/cold semantics
- **Transaction isolation**: Original storage values for gas calculation

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           world_state_t                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                      State Trie (MPT)                            │   │
│  │  Key: keccak256(address)                                         │   │
│  │  Value: RLP([nonce, balance, storage_root, code_hash])           │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌───────────────────────┐  ┌────────────────────────────────────────┐  │
│  │    storage_tries      │  │           code_store                   │  │
│  │  addr -> MPT          │  │  addr -> bytes_t (bytecode)            │  │
│  │  ┌────────────────┐   │  └────────────────────────────────────────┘  │
│  │  │ Contract A MPT │   │                                              │
│  │  │ slot -> value  │   │  ┌────────────────────────────────────────┐  │
│  │  └────────────────┘   │  │         EIP-2929 Tracking              │  │
│  │  ┌────────────────┐   │  │  warm_addresses: Set<address>          │  │
│  │  │ Contract B MPT │   │  │  warm_slots: Set<(address, slot)>      │  │
│  │  │ slot -> value  │   │  └────────────────────────────────────────┘  │
│  │  └────────────────┘   │                                              │
│  └───────────────────────┘  ┌────────────────────────────────────────┐  │
│                             │       EIP-2200 Original Storage        │  │
│                             │  (addr, slot) -> original_value        │  │
│                             └────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                    state_access_t (vtable)                       │   │
│  │  Interface for EVM: get_balance, set_storage, warm_address, ...  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## API Reference

### Types

```c
/// Account structure
typedef struct {
  uint64_t nonce;
  uint256_t balance;
  hash_t storage_root;  // Root of storage trie
  hash_t code_hash;     // keccak256(code)
} account_t;

/// Empty code hash: keccak256("")
extern const hash_t EMPTY_CODE_HASH;
// = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470

/// World state structure
typedef struct {
  state_access_t base;        // vtable interface
  mpt_t state_trie;           // Account state trie
  void *storage_tries;        // address -> mpt_t*
  void *code_store;           // address -> bytes_t
  void *warm_addresses;       // EIP-2929 warm addresses
  void *warm_slots;           // EIP-2929 warm slots
  void *original_storage;     // EIP-2200 original values
  void *dirty_storage;        // Addresses with modified storage
  uint64_t snapshot_counter;  // For nested call snapshots
  div0_arena_t *arena;        // All allocations
} world_state_t;
```

### World State Functions

#### `world_state_create`

```c
[[nodiscard]] world_state_t *world_state_create(div0_arena_t *arena);
```

Create a new empty world state.

- **arena**: Arena for all allocations (caller retains ownership)
- **Returns**: Initialized world state, or `nullptr` on failure

#### `world_state_access`

```c
[[nodiscard]] state_access_t *world_state_access(world_state_t *ws);
```

Get the state access interface for EVM integration.

- Returns a vtable-based interface with all state operations
- Valid while the world state exists

#### `world_state_get_account`

```c
[[nodiscard]] bool world_state_get_account(world_state_t *ws, const address_t *addr,
                                           account_t *out);
```

Get account from state trie.

- **Returns**: `true` if account exists
- **out**: Filled with empty account if not found

#### `world_state_set_account`

```c
bool world_state_set_account(world_state_t *ws, const address_t *addr,
                             const account_t *acc);
```

Set account in state trie.

- Empty accounts (EIP-161) are automatically deleted
- **Returns**: `true` on success

#### `world_state_get_storage_trie`

```c
[[nodiscard]] mpt_t *world_state_get_storage_trie(world_state_t *ws,
                                                   const address_t *addr);
```

Get or create storage trie for an account.

- Creates empty trie if it doesn't exist
- **Returns**: Storage trie (never `nullptr`)

#### `world_state_root`

```c
[[nodiscard]] hash_t world_state_root(world_state_t *ws);
```

Compute current state root hash.

- Updates storage roots for dirty accounts
- Recomputes state trie root
- Uses dirty tracking for efficiency

#### `world_state_clear`

```c
void world_state_clear(world_state_t *ws);
```

Reset all state to empty.

- Clears state trie, storage tries, code, warm sets
- State root becomes `MPT_EMPTY_ROOT`

#### `world_state_destroy`

```c
void world_state_destroy(world_state_t *ws);
```

Destroy world state and free resources.

- Arena is NOT destroyed (caller owns it)

### State Access Interface

The `state_access_t` vtable provides the EVM interface:

```c
typedef struct {
  // Account existence
  bool (*account_exists)(state_access_t *state, const address_t *addr);
  bool (*account_is_empty)(state_access_t *state, const address_t *addr);
  void (*create_contract)(state_access_t *state, const address_t *addr);
  void (*delete_account)(state_access_t *state, const address_t *addr);

  // Balance operations
  uint256_t (*get_balance)(state_access_t *state, const address_t *addr);
  void (*set_balance)(state_access_t *state, const address_t *addr, uint256_t bal);
  bool (*add_balance)(state_access_t *state, const address_t *addr, uint256_t amt);
  bool (*sub_balance)(state_access_t *state, const address_t *addr, uint256_t amt);

  // Nonce operations
  uint64_t (*get_nonce)(state_access_t *state, const address_t *addr);
  void (*set_nonce)(state_access_t *state, const address_t *addr, uint64_t nonce);
  uint64_t (*increment_nonce)(state_access_t *state, const address_t *addr);

  // Code operations
  bytes_t (*get_code)(state_access_t *state, const address_t *addr);
  size_t (*get_code_size)(state_access_t *state, const address_t *addr);
  hash_t (*get_code_hash)(state_access_t *state, const address_t *addr);
  void (*set_code)(state_access_t *state, const address_t *addr,
                   const uint8_t *code, size_t len);

  // Storage operations (SLOAD/SSTORE)
  uint256_t (*get_storage)(state_access_t *state, const address_t *addr, uint256_t slot);
  uint256_t (*get_original_storage)(state_access_t *state, const address_t *addr,
                                     uint256_t slot);
  void (*set_storage)(state_access_t *state, const address_t *addr,
                      uint256_t slot, uint256_t value);

  // EIP-2929 warm/cold tracking
  bool (*is_address_warm)(state_access_t *state, const address_t *addr);
  bool (*warm_address)(state_access_t *state, const address_t *addr);
  bool (*is_slot_warm)(state_access_t *state, const address_t *addr, uint256_t slot);
  bool (*warm_slot)(state_access_t *state, const address_t *addr, uint256_t slot);

  // Transaction boundary
  void (*begin_transaction)(state_access_t *state);

  // Snapshot/revert
  uint64_t (*snapshot)(state_access_t *state);
  void (*revert_to_snapshot)(state_access_t *state, uint64_t snapshot_id);
  void (*commit_snapshot)(state_access_t *state, uint64_t snapshot_id);

  // State root
  hash_t (*state_root)(state_access_t *state);

  // Lifecycle
  void (*destroy)(state_access_t *state);
} state_access_vtable_t;
```

### Convenience Wrappers

For cleaner code, use the inline wrappers in `state_access.h`:

```c
// Instead of:
uint256_t bal = state->vtable->get_balance(state, &addr);

// Use:
uint256_t bal = state_get_balance(state, &addr);
```

Available wrappers: `state_account_exists`, `state_get_balance`, `state_set_balance`, `state_get_nonce`, `state_get_storage`, `state_set_storage`, `state_get_code`, `state_get_code_hash`, `state_warm_address`, `state_warm_slot`, etc.

## Usage Examples

### Basic Usage

```c
#include "div0/mem/arena.h"
#include "div0/state/world_state.h"

// Initialize arena
div0_arena_t arena;
div0_arena_init(&arena);

// Create world state
world_state_t *ws = world_state_create(&arena);
state_access_t *state = world_state_access(ws);

// Set account balance
address_t addr = { /* 20-byte address */ };
state_set_balance(state, &addr, uint256_from_u64(1000000));

// Get account balance
uint256_t balance = state_get_balance(state, &addr);

// Compute state root
hash_t root = world_state_root(ws);

// Cleanup
world_state_destroy(ws);
div0_arena_destroy(&arena);
```

### Contract Storage

```c
// Set storage slot
uint256_t slot = uint256_from_u64(0);
uint256_t value = uint256_from_u64(42);
state_set_storage(state, &contract_addr, slot, value);

// Get storage slot
uint256_t stored = state_get_storage(state, &contract_addr, slot);

// Get original value (for gas calculation)
uint256_t original = state_get_original_storage(state, &contract_addr, slot);
```

### Contract Deployment

```c
// Create contract account (nonce = 1 per EIP-161)
state_create_contract(state, &new_contract_addr);

// Set contract code
uint8_t bytecode[] = { 0x60, 0x00, 0x60, 0x00, 0xf3 };  // PUSH 0, PUSH 0, RETURN
state_set_code(state, &new_contract_addr, bytecode, sizeof(bytecode));

// Code hash is computed automatically
hash_t code_hash = state_get_code_hash(state, &new_contract_addr);
```

### EIP-2929 Gas Calculation

```c
// At start of transaction, addresses/slots are cold
state_begin_transaction(state);

// Warm an address (returns true if was cold)
bool was_cold = state_warm_address(state, &addr);
// First access: was_cold = true, charge 2600 gas
// Subsequent: was_cold = false, charge 100 gas

// Warm a storage slot
bool slot_was_cold = state_warm_slot(state, &addr, slot);
// First access: slot_was_cold = true, charge 2100 gas
// Subsequent: slot_was_cold = false, charge 100 gas
```

### Nested Call Handling

```c
// Take snapshot before CALL
uint64_t snapshot = state_snapshot(state);

// Execute subcall...
bool call_success = execute_call(...);

if (!call_success) {
  // Revert all state changes
  state_revert_to_snapshot(state, snapshot);
} else {
  // Commit changes
  state_commit_snapshot(state, snapshot);
}
```

## EIP Compliance

### EIP-161: Empty Account Deletion

Accounts with nonce=0, balance=0, and empty code are automatically deleted:

```c
// Setting balance to zero on an empty account deletes it
state_set_balance(state, &addr, uint256_zero());
// account_exists(addr) now returns false
```

### EIP-2929: Gas Cost for State Access

The world state tracks warm/cold access for accurate gas metering:

| Operation | Cold Cost | Warm Cost |
|-----------|-----------|-----------|
| Address access | 2600 | 100 |
| Storage read | 2100 | 100 |
| Storage write | See EIP-2200 | |

### EIP-2200: SSTORE Gas Metering

Original storage values are tracked for gas calculation:

```c
// Get original value (value at start of transaction)
uint256_t original = state_get_original_storage(state, &addr, slot);

// Current value
uint256_t current = state_get_storage(state, &addr, slot);

// Gas cost depends on: original -> current -> new transitions
```

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| Account lookup | O(log n) | MPT traversal |
| Storage access | O(log m) | Per-account trie |
| Warm check | O(1) | Hash set lookup |
| State root | O(dirty accounts) | Incremental updates |

### Memory Usage

- Per account: ~200 bytes (account data + trie node)
- Per storage slot: ~100 bytes (trie node + value)
- Warm tracking: ~40 bytes per warm address/slot

## Thread Safety

The world state is **not thread-safe**. Each thread should have its own world state instance, or external synchronization must be used.

## Files

| File | Description |
|------|-------------|
| `include/div0/state/account.h` | Account type and RLP encoding |
| `include/div0/state/state_access.h` | State access vtable interface |
| `include/div0/state/world_state.h` | World state manager |
| `src/state/account.c` | Account RLP encode/decode |
| `src/state/world_state.c` | World state implementation |

## Dependencies

The world state builds on:
- **div0_mem**: Arena allocator for all allocations
- **div0_types**: uint256, address, hash, bytes
- **div0_trie**: Merkle Patricia Trie for state/storage
- **div0_rlp**: RLP encoding for account serialization
- **div0_crypto**: keccak256 for hashing
- **STC**: Hash maps/sets for warm tracking and storage