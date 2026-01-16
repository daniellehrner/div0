#include "div0/state/world_state.h"

#include "div0/crypto/keccak256.h"
#include "div0/mem/stc_allocator.h"

// =============================================================================
// STC Container Definitions
// =============================================================================

// NOLINTBEGIN(readability-identifier-naming) - STC requires specific macro names

// FNV-1a hash constants
#define FNV1A_OFFSET_BASIS 14695981039346656037ULL
#define FNV1A_PRIME 1099511628211ULL

// NOLINTBEGIN(CppDFAUnreachableFunctionCall) - Functions used via STC container macros

/// FNV-1a hash over arbitrary bytes
static uint64_t fnv1a_hash(const uint8_t *const data, const size_t len) {
  uint64_t hash = FNV1A_OFFSET_BASIS;
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= FNV1A_PRIME;
  }
  return hash;
}

/// Continue FNV-1a hash with more data
static uint64_t fnv1a_hash_append(uint64_t hash, const uint8_t *const data, const size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= FNV1A_PRIME;
  }
  return hash;
}

// NOLINTEND(CppDFAUnreachableFunctionCall)

// Hash function for address_t
static uint64_t address_hash(const address_t *const addr) {
  return fnv1a_hash(addr->bytes, ADDRESS_SIZE);
}

// Storage trie map: address -> mpt_t*
#define i_TYPE storage_trie_map, address_t, mpt_t *
#define i_hash(p) address_hash(p)
#define i_eq(a, b) address_equal(a, b)
#include "stc/hmap.h"

// Code store map: address -> bytes_t
#define i_TYPE code_map, address_t, bytes_t
#define i_hash(p) address_hash(p)
#define i_eq(a, b) address_equal(a, b)
#include "stc/hmap.h"

// Warm address set
#define i_TYPE warm_addr_set, address_t
#define i_hash(p) address_hash(p)
#define i_eq(a, b) address_equal(a, b)
#include "stc/hset.h"

// Dirty storage address set (for efficient state root computation)
#define i_TYPE dirty_addr_set, address_t
#define i_hash(p) address_hash(p)
#define i_eq(a, b) address_equal(a, b)
#include "stc/hset.h"

// All accounts set (for post-state export)
#define i_TYPE all_accounts_set, address_t
#define i_hash(p) address_hash(p)
#define i_eq(a, b) address_equal(a, b)
#include "stc/hset.h"

// Warm slot key (address + slot)
typedef struct {
  address_t addr;
  uint256_t slot;
} warm_slot_key_t;

static uint64_t warm_slot_key_hash(const warm_slot_key_t *const key) {
  // FNV-1a hash over address + slot bytes
  const uint64_t hash = fnv1a_hash(key->addr.bytes, ADDRESS_SIZE);

  // Hash slot as 32 big-endian bytes for consistency
  uint8_t slot_bytes[32];
  uint256_to_bytes_be(key->slot, slot_bytes);
  return fnv1a_hash_append(hash, slot_bytes, 32);
}

static bool warm_slot_key_eq(const warm_slot_key_t *const a, const warm_slot_key_t *const b) {
  if (!address_equal(&a->addr, &b->addr)) {
    return false;
  }
  return uint256_eq(a->slot, b->slot);
}

// Warm slot set
#define i_TYPE warm_slot_set, warm_slot_key_t
#define i_hash(p) warm_slot_key_hash(p)
#define i_eq(a, b) warm_slot_key_eq(a, b)
#include "stc/hset.h"

// Original storage map: (address, slot) -> original uint256 value
// Used for EIP-2200/EIP-3529 gas calculation
#define i_TYPE original_storage_map, warm_slot_key_t, uint256_t
#define i_hash(p) warm_slot_key_hash(p)
#define i_eq(a, b) warm_slot_key_eq(a, b)
#include "stc/hmap.h"

// All storage slots set (for post-state export)
#define i_TYPE all_slots_set, warm_slot_key_t
#define i_hash(p) warm_slot_key_hash(p)
#define i_eq(a, b) warm_slot_key_eq(a, b)
#include "stc/hset.h"

// NOLINTEND(readability-identifier-naming)

// Forward declarations
static void erase_slots_for_address(all_slots_set *all_slots, const address_t *addr);

// =============================================================================
// Journal Helpers
// =============================================================================

enum { JOURNAL_INITIAL_CAPACITY = 64 };

/// Append an entry to the journal, growing if needed.
static bool journal_append(world_state_t *const ws, const journal_entry_t entry) {
  if (ws->journal_len >= ws->journal_cap) {
    // Grow journal
    const size_t new_cap = ws->journal_cap == 0 ? JOURNAL_INITIAL_CAPACITY : ws->journal_cap * 2;
    journal_entry_t *const new_journal =
        div0_arena_alloc(ws->arena, new_cap * sizeof(journal_entry_t));
    if (new_journal == nullptr) {
      return false;
    }
    if (ws->journal != nullptr && ws->journal_len > 0) {
      __builtin___memcpy_chk(new_journal, ws->journal, ws->journal_len * sizeof(journal_entry_t),
                             new_cap * sizeof(journal_entry_t));
    }
    ws->journal = new_journal;
    ws->journal_cap = new_cap;
  }
  ws->journal[ws->journal_len++] = entry;
  return true;
}

// =============================================================================
// Helper Functions
// =============================================================================

/// Compute the state trie key for an address (keccak256 of address).
static hash_t address_to_key(const address_t *const addr) {
  return keccak256(addr->bytes, ADDRESS_SIZE);
}

/// Compute the storage trie key for a slot (keccak256 of slot as 32-byte BE).
static hash_t slot_to_key(const uint256_t slot) { // NOLINT(performance-unnecessary-value-param)
  uint8_t slot_bytes[32];
  uint256_to_bytes_be(slot, slot_bytes);
  return keccak256(slot_bytes, 32);
}

// =============================================================================
// Vtable Function Implementations
// =============================================================================

static bool ws_account_exists(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  const hash_t key = address_to_key(addr);
  const bytes_t value = mpt_get(&ws->state_trie, key.bytes, HASH_SIZE);
  return value.data != nullptr;
}

static bool ws_account_is_empty(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return true; // Non-existent is empty
  }
  return account_is_empty(&acc);
}

static void ws_create_contract(state_access_t *const state, const address_t *const addr) {
  const auto ws = (world_state_t *)state;
  const bool existed = ws_account_exists(state, addr);

  // Journal account creation (revert will delete if didn't exist before)
  journal_append(ws, (journal_entry_t){.op = JOURNAL_ACCOUNT_CREATE,
                                       .address = *addr,
                                       .prev = {.existed = existed}});

  if (existed) {
    return; // Already exists
  }
  // Create with nonce=1 per EIP-161 (ensures non-empty account)
  account_t acc = account_empty(); // Modified below
  acc.nonce = 1;
  world_state_set_account(ws, addr, &acc);
}

static void ws_delete_account(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  const hash_t key = address_to_key(addr);
  mpt_delete(&ws->state_trie, key.bytes, HASH_SIZE);

  // Also remove from code store and storage tries
  const auto st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_erase(st_map, *addr);

  const auto c_map = (code_map *)ws->code_store;
  code_map_erase(c_map, *addr);
}

static uint256_t ws_get_balance(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return uint256_zero();
  }
  return acc.balance;
}

/// Internal: set balance without journaling (used by revert)
static void ws_set_balance_internal(world_state_t *const ws, const address_t *const addr,
                                    const uint256_t balance) {
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  acc.balance = balance;
  world_state_set_account(ws, addr, &acc);
}

static void ws_set_balance(state_access_t *state, const address_t *addr, const uint256_t balance) {
  const auto ws = (world_state_t *)state;

  // Journal old balance before modification
  account_t acc;
  const uint256_t old_balance =
      world_state_get_account(ws, addr, &acc) ? acc.balance : uint256_zero();
  journal_append(
      ws,
      (journal_entry_t){.op = JOURNAL_BALANCE, .address = *addr, .prev = {.balance = old_balance}});

  // Now apply the change
  ws_set_balance_internal(ws, addr, balance);
}

static bool ws_add_balance(state_access_t *state, const address_t *addr, const uint256_t amount) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }

  // Check for overflow
  const uint256_t new_balance = uint256_add(acc.balance, amount);
  if (uint256_lt(new_balance, acc.balance)) {
    return false; // Overflow
  }

  // Journal old balance before modification
  journal_append(
      ws,
      (journal_entry_t){.op = JOURNAL_BALANCE, .address = *addr, .prev = {.balance = acc.balance}});

  acc.balance = new_balance;
  world_state_set_account(ws, addr, &acc);
  return true;
}

static bool ws_sub_balance(state_access_t *state, const address_t *addr, const uint256_t amount) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return uint256_is_zero(amount); // Can only subtract 0 from non-existent
  }

  // Check for underflow
  if (uint256_lt(acc.balance, amount)) {
    return false; // Insufficient balance
  }

  // Journal old balance before modification
  journal_append(
      ws,
      (journal_entry_t){.op = JOURNAL_BALANCE, .address = *addr, .prev = {.balance = acc.balance}});

  acc.balance = uint256_sub(acc.balance, amount);
  world_state_set_account(ws, addr, &acc);
  return true;
}

static uint64_t ws_get_nonce(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return 0;
  }
  return acc.nonce;
}

/// Internal: set nonce without journaling (used by revert)
static void ws_set_nonce_internal(world_state_t *const ws, const address_t *const addr,
                                  const uint64_t nonce) {
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  acc.nonce = nonce;
  world_state_set_account(ws, addr, &acc);
}

static void ws_set_nonce(state_access_t *state, const address_t *addr, const uint64_t nonce) {
  const auto ws = (world_state_t *)state;
  account_t acc;

  // Journal old nonce before modification
  const uint64_t old_nonce = world_state_get_account(ws, addr, &acc) ? acc.nonce : 0;
  journal_append(
      ws, (journal_entry_t){.op = JOURNAL_NONCE, .address = *addr, .prev = {.nonce = old_nonce}});

  ws_set_nonce_internal(ws, addr, nonce);
}

static uint64_t ws_increment_nonce(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  const uint64_t old_nonce = acc.nonce;

  // Check for nonce overflow (EIP-2681 limits nonce to 2^64-2)
  if (acc.nonce >= UINT64_MAX - 1) {
    return old_nonce; // Saturate at max, don't increment
  }

  // Journal old nonce before modification
  journal_append(
      ws, (journal_entry_t){.op = JOURNAL_NONCE, .address = *addr, .prev = {.nonce = old_nonce}});

  acc.nonce++;
  world_state_set_account(ws, addr, &acc);
  return old_nonce;
}

static bytes_t ws_get_code(state_access_t *const state, const address_t *const addr) {
  const auto ws = (world_state_t *)state;
  const auto c_map = (code_map *)ws->code_store;

  const code_map_value *const entry = code_map_get(c_map, *addr);
  if (entry != nullptr) {
    return entry->second;
  }

  bytes_t empty;
  bytes_init(&empty);
  return empty;
}

static size_t ws_get_code_size(state_access_t *const state, const address_t *const addr) {
  const bytes_t code = ws_get_code(state, addr);
  return code.size;
}

static hash_t ws_get_code_hash(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return EMPTY_CODE_HASH;
  }
  return acc.code_hash;
}

static void ws_set_code(state_access_t *state, const address_t *addr, const uint8_t *code,
                        const size_t code_len) {
  const auto ws = (world_state_t *)state;

  // Store code in code map
  const auto c_map = (code_map *)ws->code_store;
  bytes_t code_bytes;
  bytes_init_arena(&code_bytes, ws->arena);
  if (code_len > 0) {
    bytes_from_data(&code_bytes, code, code_len);
  }
  code_map_insert(c_map, *addr, code_bytes);

  // Update account code_hash
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }

  if (code_len == 0) {
    acc.code_hash = EMPTY_CODE_HASH;
  } else {
    acc.code_hash = keccak256(code, code_len);
  }

  world_state_set_account(ws, addr, &acc);
}

static uint256_t ws_get_storage(state_access_t *const state, const address_t *const addr,
                                const uint256_t slot) {
  const auto ws = (world_state_t *)state;

  const auto st_map = (storage_trie_map *)ws->storage_tries;
  const storage_trie_map_value *const entry = storage_trie_map_get(st_map, *addr);
  if (entry == nullptr) {
    return uint256_zero();
  }

  const mpt_t *const storage = entry->second;
  const hash_t key = slot_to_key(slot);
  const bytes_t value = mpt_get(storage, key.bytes, HASH_SIZE);
  if (value.data == nullptr || value.size == 0) {
    return uint256_zero();
  }

  // Value is stored as minimal big-endian bytes
  return uint256_from_bytes_be(value.data, value.size);
}

static uint256_t ws_get_original_storage(state_access_t *const state, const address_t *const addr,
                                         const uint256_t slot) {
  const auto ws = (world_state_t *)state;
  const auto orig_map = (original_storage_map *)ws->original_storage;

  const warm_slot_key_t key = {.addr = *addr, .slot = slot};
  const original_storage_map_value *const entry = original_storage_map_get(orig_map, key);

  if (entry != nullptr) {
    return entry->second;
  }

  // Not yet tracked - return current value (no writes yet this tx)
  return ws_get_storage(state, addr, slot);
}

/// Internal: set storage without journaling (used by revert)
static void ws_set_storage_internal(world_state_t *const ws, const address_t *const addr,
                                    const uint256_t slot, const uint256_t value) {
  // Mark address as having dirty storage for efficient state root computation
  const auto dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_insert(dirty, *addr);

  // Track slot for post-state export
  const auto all_slots = (all_slots_set *)ws->all_storage_slots;
  const warm_slot_key_t slot_key = {.addr = *addr, .slot = slot};
  if (uint256_is_zero(value)) {
    all_slots_set_erase(all_slots, slot_key);
  } else {
    all_slots_set_insert(all_slots, slot_key);
  }

  mpt_t *const storage = world_state_get_storage_trie(ws, addr);
  const hash_t key = slot_to_key(slot);

  if (uint256_is_zero(value)) {
    mpt_delete(storage, key.bytes, HASH_SIZE);
  } else {
    uint8_t be_bytes[32];
    uint256_to_bytes_be(value, be_bytes);
    size_t start = 0;
    while (start < 32 && be_bytes[start] == 0) {
      start++;
    }
    mpt_insert(storage, key.bytes, HASH_SIZE, be_bytes + start, 32 - start);
  }
}

static void ws_set_storage(state_access_t *const state, const address_t *const addr,
                           const uint256_t slot, const uint256_t value) {
  const auto ws = (world_state_t *)state;

  // Get old value for journaling
  const uint256_t old_value = ws_get_storage(state, addr, slot);

  // Journal old value before modification
  journal_append(ws, (journal_entry_t){.op = JOURNAL_STORAGE,
                                       .address = *addr,
                                       .prev = {.storage = {.slot = slot, .value = old_value}}});

  // Record original value on first write (for EIP-2200 gas calculation)
  const auto orig_map = (original_storage_map *)ws->original_storage;
  const warm_slot_key_t slot_key = {.addr = *addr, .slot = slot};
  if (!original_storage_map_contains(orig_map, slot_key)) {
    original_storage_map_insert(orig_map, slot_key, old_value);
  }

  ws_set_storage_internal(ws, addr, slot, value);
}

static bool ws_is_address_warm(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  const auto set = (warm_addr_set *)ws->warm_addresses;
  return warm_addr_set_contains(set, *addr);
}

static bool ws_warm_address(state_access_t *state, const address_t *addr) {
  const auto ws = (world_state_t *)state;
  const auto set = (warm_addr_set *)ws->warm_addresses;

  if (warm_addr_set_contains(set, *addr)) {
    return false; // Already warm, not cold
  }

  // Journal that we're warming this address (revert will remove it)
  journal_append(ws, (journal_entry_t){.op = JOURNAL_WARM_ADDRESS, .address = *addr});

  warm_addr_set_insert(set, *addr);
  return true; // Was cold (first access)
}

static bool ws_is_slot_warm(state_access_t *const state, const address_t *const addr,
                            const uint256_t slot) {
  const auto ws = (world_state_t *)state;
  const auto set = (warm_slot_set *)ws->warm_slots;
  const warm_slot_key_t key = {.addr = *addr, .slot = slot};
  return warm_slot_set_contains(set, key);
}

static bool ws_warm_slot(state_access_t *const state, const address_t *const addr,
                         const uint256_t slot) {
  const auto ws = (world_state_t *)state;
  const auto set = (warm_slot_set *)ws->warm_slots;
  const warm_slot_key_t key = {.addr = *addr, .slot = slot};

  if (warm_slot_set_contains(set, key)) {
    return false; // Already warm, not cold
  }

  // Journal that we're warming this slot (revert will remove it)
  journal_append(
      ws, (journal_entry_t){.op = JOURNAL_WARM_SLOT, .address = *addr, .prev = {.slot = slot}});

  warm_slot_set_insert(set, key);
  return true; // Was cold (first access)
}

static void ws_begin_transaction(state_access_t *state) {
  const auto ws = (world_state_t *)state;

  // Clear warm sets
  const auto addr_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_clear(addr_set);

  const auto slot_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_clear(slot_set);

  // Clear original storage tracking
  const auto orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_clear(orig_map);

  // Reset journal for new transaction (keep capacity, just reset length)
  ws->journal_len = 0;
}

// =============================================================================
// Snapshot/Revert Implementation (geth-style journaling)
// =============================================================================

/// Take a snapshot - returns current journal position
static uint64_t ws_snapshot(state_access_t *state) {
  const auto ws = (world_state_t *)state;
  return ws->journal_len;
}

/// Revert to snapshot - walk backwards through journal, restoring previous values
static void ws_revert_to_snapshot(state_access_t *const state, const uint64_t snapshot_id) {
  const auto ws = (world_state_t *)state;
  const size_t target = snapshot_id;

  // Walk backwards from current position to snapshot
  while (ws->journal_len > target) {
    ws->journal_len--;
    const journal_entry_t *const e = &ws->journal[ws->journal_len];

    switch (e->op) {
    case JOURNAL_BALANCE:
      ws_set_balance_internal(ws, &e->address, e->prev.balance);
      break;

    case JOURNAL_NONCE:
      ws_set_nonce_internal(ws, &e->address, e->prev.nonce);
      break;

    case JOURNAL_STORAGE:
      ws_set_storage_internal(ws, &e->address, e->prev.storage.slot, e->prev.storage.value);
      break;

    case JOURNAL_ACCOUNT_CREATE:
      // If account didn't exist before, delete it
      if (!e->prev.existed) {
        ws_delete_account(state, &e->address);
      }
      break;

    case JOURNAL_ACCOUNT_DELETE:
      // Not currently used, but would restore deleted account
      break;

    case JOURNAL_WARM_ADDRESS: {
      // Remove from warm addresses set
      const auto set = (warm_addr_set *)ws->warm_addresses;
      warm_addr_set_erase(set, e->address);
      break;
    }

    case JOURNAL_WARM_SLOT: {
      // Remove from warm slots set
      const auto set = (warm_slot_set *)ws->warm_slots;
      const warm_slot_key_t key = {.addr = e->address, .slot = e->prev.slot};
      warm_slot_set_erase(set, key);
      break;
    }

    case JOURNAL_CODE:
      // Code changes would need code restoration - not commonly reverted
      break;
    }
  }
}

/// Commit snapshot - no-op, entries stay in journal until transaction ends
// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - vtable semantic contract: commit may modify
// state
static void ws_commit_snapshot(state_access_t *const state, const uint64_t snapshot_id) {
  (void)state;
  (void)snapshot_id;
  // In geth-style journaling, commit is a no-op - journal entries
  // become part of the parent snapshot's entries
}

static hash_t ws_state_root(state_access_t *state) {
  const auto ws = (world_state_t *)state;
  return world_state_root(ws);
}

static void ws_destroy(state_access_t *state) {
  const auto ws = (world_state_t *)state;
  world_state_destroy(ws);
}

// =============================================================================
// Vtable Definition
// =============================================================================

static const state_access_vtable_t WORLD_STATE_VTABLE = {
    .account_exists = ws_account_exists,
    .account_is_empty = ws_account_is_empty,
    .create_contract = ws_create_contract,
    .delete_account = ws_delete_account,

    .get_balance = ws_get_balance,
    .set_balance = ws_set_balance,
    .add_balance = ws_add_balance,
    .sub_balance = ws_sub_balance,

    .get_nonce = ws_get_nonce,
    .set_nonce = ws_set_nonce,
    .increment_nonce = ws_increment_nonce,

    .get_code = ws_get_code,
    .get_code_size = ws_get_code_size,
    .get_code_hash = ws_get_code_hash,
    .set_code = ws_set_code,

    .get_storage = ws_get_storage,
    .get_original_storage = ws_get_original_storage,
    .set_storage = ws_set_storage,

    .is_address_warm = ws_is_address_warm,
    .warm_address = ws_warm_address,
    .is_slot_warm = ws_is_slot_warm,
    .warm_slot = ws_warm_slot,

    .begin_transaction = ws_begin_transaction,

    .snapshot = ws_snapshot,
    .revert_to_snapshot = ws_revert_to_snapshot,
    .commit_snapshot = ws_commit_snapshot,

    .state_root = ws_state_root,

    .destroy = ws_destroy,
};

// =============================================================================
// Public API Implementation
// =============================================================================

world_state_t *world_state_create(div0_arena_t *const arena) {
  if (arena == nullptr) {
    return nullptr;
  }

  // Set thread-local arena for STC containers
  div0_stc_arena = arena;

  world_state_t *const ws = div0_arena_alloc(arena, sizeof(world_state_t));
  if (ws == nullptr) {
    return nullptr;
  }

  // Zero-initialize for safety (all pointers start as nullptr)
  __builtin___memset_chk(ws, 0, sizeof(*ws), __builtin_object_size(ws, 0));

  ws->base.vtable = &WORLD_STATE_VTABLE;
  ws->arena = arena;

  // Create state trie backend
  ws->state_backend = mpt_memory_backend_create(arena);
  if (ws->state_backend == nullptr) {
    goto fail;
  }

  // Initialize state trie
  mpt_init(&ws->state_trie, ws->state_backend, arena);

  // Initialize hash tables - on failure, goto fail for cleanup
  // Note: STC _init() doesn't allocate, so partial init is safe
  storage_trie_map *st_map = div0_arena_alloc(arena, sizeof(storage_trie_map));
  if (st_map == nullptr) {
    goto fail;
  }
  *st_map = storage_trie_map_init();
  ws->storage_tries = st_map;

  code_map *c_map = div0_arena_alloc(arena, sizeof(code_map));
  if (c_map == nullptr) {
    goto fail;
  }
  *c_map = code_map_init();
  ws->code_store = c_map;

  warm_addr_set *wa_set = div0_arena_alloc(arena, sizeof(warm_addr_set));
  if (wa_set == nullptr) {
    goto fail;
  }
  *wa_set = warm_addr_set_init();
  ws->warm_addresses = wa_set;

  warm_slot_set *wslot_set = div0_arena_alloc(arena, sizeof(warm_slot_set));
  if (wslot_set == nullptr) {
    goto fail;
  }
  *wslot_set = warm_slot_set_init();
  ws->warm_slots = wslot_set;

  original_storage_map *orig_map = div0_arena_alloc(arena, sizeof(original_storage_map));
  if (orig_map == nullptr) {
    goto fail;
  }
  *orig_map = original_storage_map_init();
  ws->original_storage = orig_map;

  dirty_addr_set *dirty = div0_arena_alloc(arena, sizeof(dirty_addr_set));
  if (dirty == nullptr) {
    goto fail;
  }
  *dirty = dirty_addr_set_init();
  ws->dirty_storage = dirty;

  all_accounts_set *all_accts = div0_arena_alloc(arena, sizeof(all_accounts_set));
  if (all_accts == nullptr) {
    goto fail;
  }
  *all_accts = all_accounts_set_init();
  ws->all_accounts = all_accts;

  all_slots_set *all_slots = div0_arena_alloc(arena, sizeof(all_slots_set));
  if (all_slots == nullptr) {
    goto fail;
  }
  *all_slots = all_slots_set_init();
  ws->all_storage_slots = all_slots;

  // Initialize journal (lazily allocated on first use)
  ws->journal = nullptr;
  ws->journal_len = 0;
  ws->journal_cap = 0;

  return ws;

fail:
  // On failure, clean up any initialized STC containers
  // Note: STC _drop is safe to call on zero-initialized containers
  if (ws->storage_tries != nullptr) {
    storage_trie_map_drop(ws->storage_tries);
  }
  if (ws->code_store != nullptr) {
    code_map_drop(ws->code_store);
  }
  if (ws->warm_addresses != nullptr) {
    warm_addr_set_drop(ws->warm_addresses);
  }
  if (ws->warm_slots != nullptr) {
    warm_slot_set_drop(ws->warm_slots);
  }
  if (ws->original_storage != nullptr) {
    original_storage_map_drop(ws->original_storage);
  }
  if (ws->dirty_storage != nullptr) {
    dirty_addr_set_drop(ws->dirty_storage);
  }
  if (ws->all_accounts != nullptr) {
    all_accounts_set_drop(ws->all_accounts);
  }
  if (ws->all_storage_slots != nullptr) {
    all_slots_set_drop(ws->all_storage_slots);
  }
  // Arena memory is not freed (owned by caller)
  return nullptr;
}

bool world_state_get_account(const world_state_t *const ws, const address_t *const addr,
                             account_t *const out) {
  const hash_t key = address_to_key(addr);
  const bytes_t value = mpt_get(&ws->state_trie, key.bytes, HASH_SIZE);

  if (value.data == nullptr) {
    *out = account_empty();
    return false;
  }

  if (!account_rlp_decode(value.data, value.size, out)) {
    *out = account_empty();
    return false;
  }

  return true;
}

bool world_state_set_account(world_state_t *const ws, const address_t *const addr,
                             const account_t *const acc) {
  const hash_t key = address_to_key(addr);

  // EIP-161: Don't store empty accounts
  if (account_is_empty(acc)) {
    mpt_delete(&ws->state_trie, key.bytes, HASH_SIZE);
    // Remove from all_accounts set when account becomes empty
    const auto all_accts = (all_accounts_set *)ws->all_accounts;
    all_accounts_set_erase(all_accts, *addr);
    // Also remove all storage slots for this account
    const auto all_slots = (all_slots_set *)ws->all_storage_slots;
    erase_slots_for_address(all_slots, addr);
    return true;
  }

  // Track this address in all_accounts set for post-state export
  const auto all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_insert(all_accts, *addr);

  // RLP-encode account
  const bytes_t encoded = account_rlp_encode(acc, ws->arena);
  if (encoded.data == nullptr) {
    return false;
  }

  mpt_insert(&ws->state_trie, key.bytes, HASH_SIZE, encoded.data, encoded.size);
  return true;
}

// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - modifies ws->storage_tries through cast
mpt_t *world_state_get_storage_trie(world_state_t *const ws, const address_t *addr) {
  const auto st_map = (storage_trie_map *)ws->storage_tries;

  // Check if storage trie exists
  const storage_trie_map_value *const entry = storage_trie_map_get(st_map, *addr);
  if (entry != nullptr) {
    return entry->second;
  }

  // Create new storage trie
  mpt_backend_t *const backend = mpt_memory_backend_create(ws->arena);
  if (backend == nullptr) {
    return nullptr;
  }

  mpt_t *const storage = div0_arena_alloc(ws->arena, sizeof(mpt_t));
  if (storage == nullptr) {
    return nullptr;
  }

  mpt_init(storage, backend, ws->arena);
  storage_trie_map_insert(st_map, *addr, storage);

  return storage;
}

hash_t world_state_root(world_state_t *const ws) {
  // Only update storage roots for accounts with dirty storage
  const auto dirty = (dirty_addr_set *)ws->dirty_storage;
  const auto st_map = (storage_trie_map *)ws->storage_tries;

  for (dirty_addr_set_iter it = dirty_addr_set_begin(dirty);
       it.ref != dirty_addr_set_end(dirty).ref; dirty_addr_set_next(&it)) {

    const address_t addr = *it.ref;

    // Get storage trie for this address
    const storage_trie_map_value *const entry = storage_trie_map_get(st_map, addr);
    if (entry == nullptr) {
      continue; // No storage trie (shouldn't happen if dirty)
    }
    const mpt_t *const storage = entry->second;

    // Get current account
    account_t acc;
    if (!world_state_get_account(ws, &addr, &acc)) {
      acc = account_empty();
    }

    // Update storage root
    acc.storage_root = mpt_root_hash(storage);

    // Store updated account
    world_state_set_account(ws, &addr, &acc);
  }

  // Clear dirty set after processing
  dirty_addr_set_clear(dirty);

  // Now compute and return state root
  return mpt_root_hash(&ws->state_trie);
}

void world_state_clear(world_state_t *const ws) {
  mpt_clear(&ws->state_trie);

  const auto st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_clear(st_map);

  const auto c_map = (code_map *)ws->code_store;
  code_map_clear(c_map);

  const auto wa_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_clear(wa_set);

  const auto ws_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_clear(ws_set);

  const auto orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_clear(orig_map);

  const auto dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_clear(dirty);

  const auto all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_clear(all_accts);

  const auto all_slots = (all_slots_set *)ws->all_storage_slots;
  all_slots_set_clear(all_slots);
}

// NOLINTNEXTLINE(CppParameterMayBeConstPtrOrRef) - modifies ws members through casts
void world_state_destroy(world_state_t *const ws) {
  // Clean up hash tables
  const auto st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_drop(st_map);

  const auto c_map = (code_map *)ws->code_store;
  code_map_drop(c_map);

  const auto wa_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_drop(wa_set);

  const auto ws_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_drop(ws_set);

  const auto orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_drop(orig_map);

  const auto dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_drop(dirty);

  const auto all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_drop(all_accts);

  const auto all_slots = (all_slots_set *)ws->all_storage_slots;
  all_slots_set_drop(all_slots);

  // Note: Arena memory is not freed here (owned by caller)
}

// =============================================================================
// Post-State Export
// =============================================================================

/// Erase all storage slots for a specific address.
/// This is used when an account is deleted (becomes empty).
static void erase_slots_for_address(all_slots_set *const all_slots, const address_t *const addr) {
  // We cannot erase during iteration, so collect keys first
  // Use a fixed-size stack buffer for common cases, fall back to slower repeated iteration
  warm_slot_key_t to_erase[64];
  size_t erase_count = 0;
  bool more_to_erase = true;

  while (more_to_erase) {
    erase_count = 0;
    more_to_erase = false;

    for (all_slots_set_iter it = all_slots_set_begin(all_slots);
         it.ref != all_slots_set_end(all_slots).ref; all_slots_set_next(&it)) {
      if (address_equal(&it.ref->addr, addr)) {
        if (erase_count < 64) {
          to_erase[erase_count++] = *it.ref;
        } else {
          more_to_erase = true;
          break;
        }
      }
    }

    // Erase collected keys
    for (size_t i = 0; i < erase_count; i++) {
      all_slots_set_erase(all_slots, to_erase[i]);
    }
  }
}

bool world_state_snapshot(world_state_t *const ws, div0_arena_t *const arena,
                          state_snapshot_t *const out) {
  const auto all_accts = (all_accounts_set *)ws->all_accounts;
  const auto all_slots = (all_slots_set *)ws->all_storage_slots;
  const auto c_map = (code_map *)ws->code_store;

  // Count accounts
  const size_t account_count = (size_t)all_accounts_set_size(all_accts);
  if (account_count == 0) {
    out->accounts = nullptr;
    out->account_count = 0;
    return true;
  }

  // Allocate accounts array
  out->accounts = div0_arena_alloc(arena, account_count * sizeof(account_snapshot_t));
  if (out->accounts == nullptr) {
    return false;
  }

  // Phase 1: Build account snapshots (without storage) and create address->index map
  // We use a simple linear search for the address->index lookup since account counts
  // are typically small (< 100). For larger state, consider a hash map.
  size_t valid_count = 0;
  for (all_accounts_set_iter it = all_accounts_set_begin(all_accts);
       it.ref != all_accounts_set_end(all_accts).ref; all_accounts_set_next(&it)) {

    const address_t addr = *it.ref;
    account_snapshot_t *const snap_acc = &out->accounts[valid_count];

    // Initialize
    __builtin___memset_chk(snap_acc, 0, sizeof(*snap_acc), __builtin_object_size(snap_acc, 0));
    snap_acc->address = addr;

    // Get account data
    account_t acc;
    if (!world_state_get_account(ws, &addr, &acc)) {
      // Account was deleted (empty) - skip it
      continue;
    }

    snap_acc->balance = acc.balance;
    snap_acc->nonce = acc.nonce;

    // Get code
    const code_map_value *const code_entry = code_map_get(c_map, addr);
    if (code_entry != nullptr && code_entry->second.size > 0) {
      snap_acc->code.size = code_entry->second.size;
      snap_acc->code.data = div0_arena_alloc(arena, code_entry->second.size);
      if (snap_acc->code.data == nullptr) {
        return false;
      }
      __builtin___memcpy_chk(snap_acc->code.data, code_entry->second.data, code_entry->second.size,
                             code_entry->second.size);
    }

    valid_count++;
  }
  out->account_count = valid_count;

  if (valid_count == 0) {
    return true;
  }

  // Phase 2: Count slots per account in a single pass through all_slots
  // Allocate a temporary counter array
  size_t *slot_counts = div0_arena_alloc(arena, valid_count * sizeof(size_t));
  if (slot_counts == nullptr) {
    return false;
  }
  __builtin___memset_chk(slot_counts, 0, valid_count * sizeof(size_t),
                         valid_count * sizeof(size_t));

  for (all_slots_set_iter slot_it = all_slots_set_begin(all_slots);
       slot_it.ref != all_slots_set_end(all_slots).ref; all_slots_set_next(&slot_it)) {
    // Find account index for this slot's address
    for (size_t i = 0; i < valid_count; i++) {
      if (address_equal(&slot_it.ref->addr, &out->accounts[i].address)) {
        slot_counts[i]++;
        break;
      }
    }
  }

  // Phase 3: Allocate storage arrays for each account
  for (size_t i = 0; i < valid_count; i++) {
    if (slot_counts[i] > 0) {
      out->accounts[i].storage = div0_arena_alloc(arena, slot_counts[i] * sizeof(storage_entry_t));
      if (out->accounts[i].storage == nullptr) {
        return false;
      }
    }
  }

  // Reset slot_counts to use as insertion indices
  __builtin___memset_chk(slot_counts, 0, valid_count * sizeof(size_t),
                         valid_count * sizeof(size_t));

  // Phase 4: Populate storage entries in a single pass
  for (all_slots_set_iter slot_it = all_slots_set_begin(all_slots);
       slot_it.ref != all_slots_set_end(all_slots).ref; all_slots_set_next(&slot_it)) {
    // Find account index for this slot's address
    for (size_t i = 0; i < valid_count; i++) {
      if (address_equal(&slot_it.ref->addr, &out->accounts[i].address)) {
        const uint256_t slot = slot_it.ref->slot;
        const uint256_t val = ws_get_storage(&ws->base, &out->accounts[i].address, slot);

        // Only include non-zero values
        if (!uint256_is_zero(val)) {
          const size_t idx = slot_counts[i]++;
          out->accounts[i].storage[idx].slot = slot;
          out->accounts[i].storage[idx].value = val;
        }
        break;
      }
    }
  }

  // Set final storage counts
  for (size_t i = 0; i < valid_count; i++) {
    out->accounts[i].storage_count = slot_counts[i];
  }

  return true;
}
