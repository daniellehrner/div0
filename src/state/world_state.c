#include "div0/state/world_state.h"

#include "div0/crypto/keccak256.h"
#include "div0/mem/stc_allocator.h"
#include "div0/rlp/encode.h"

#include <string.h>

// =============================================================================
// STC Container Definitions
// =============================================================================

// NOLINTBEGIN(readability-identifier-naming) - STC requires specific macro names

// FNV-1a hash constants
#define FNV1A_OFFSET_BASIS 14695981039346656037ULL
#define FNV1A_PRIME 1099511628211ULL

// NOLINTBEGIN(CppDFAUnreachableFunctionCall) - Functions used via STC container macros

/// FNV-1a hash over arbitrary bytes
static uint64_t fnv1a_hash(const uint8_t *data, size_t len) {
  uint64_t hash = FNV1A_OFFSET_BASIS;
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= FNV1A_PRIME;
  }
  return hash;
}

/// Continue FNV-1a hash with more data
static uint64_t fnv1a_hash_append(uint64_t hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= FNV1A_PRIME;
  }
  return hash;
}

// NOLINTEND(CppDFAUnreachableFunctionCall)

// Hash function for address_t
static uint64_t address_hash(const address_t *addr) {
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

static uint64_t warm_slot_key_hash(const warm_slot_key_t *key) {
  // FNV-1a hash over address + slot bytes
  uint64_t hash = fnv1a_hash(key->addr.bytes, ADDRESS_SIZE);

  // Hash slot as 32 big-endian bytes for consistency
  uint8_t slot_bytes[32];
  uint256_to_bytes_be(key->slot, slot_bytes);
  return fnv1a_hash_append(hash, slot_bytes, 32);
}

static bool warm_slot_key_eq(const warm_slot_key_t *a, const warm_slot_key_t *b) {
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

// =============================================================================
// Helper Functions
// =============================================================================

/// Compute the state trie key for an address (keccak256 of address).
static hash_t address_to_key(const address_t *addr) {
  return keccak256(addr->bytes, ADDRESS_SIZE);
}

/// Compute the storage trie key for a slot (keccak256 of slot as 32-byte BE).
static hash_t slot_to_key(uint256_t slot) {
  uint8_t slot_bytes[32];
  uint256_to_bytes_be(slot, slot_bytes);
  return keccak256(slot_bytes, 32);
}

// =============================================================================
// Vtable Function Implementations
// =============================================================================

static bool ws_account_exists(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  hash_t key = address_to_key(addr);
  bytes_t value = mpt_get(&ws->state_trie, key.bytes, HASH_SIZE);
  return value.data != nullptr;
}

static bool ws_account_is_empty(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return true; // Non-existent is empty
  }
  return account_is_empty(&acc);
}

static void ws_create_contract(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  if (ws_account_exists(state, addr)) {
    return; // Already exists
  }
  // Create with nonce=1 per EIP-161 (ensures non-empty account)
  account_t acc = account_empty();
  acc.nonce = 1;
  world_state_set_account(ws, addr, &acc);
}

static void ws_delete_account(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  hash_t key = address_to_key(addr);
  mpt_delete(&ws->state_trie, key.bytes, HASH_SIZE);

  // Also remove from code store and storage tries
  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_erase(st_map, *addr);

  code_map *c_map = (code_map *)ws->code_store;
  code_map_erase(c_map, *addr);
}

static uint256_t ws_get_balance(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return uint256_zero();
  }
  return acc.balance;
}

static void ws_set_balance(state_access_t *state, const address_t *addr, uint256_t balance) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  acc.balance = balance;
  world_state_set_account(ws, addr, &acc);
}

static bool ws_add_balance(state_access_t *state, const address_t *addr, uint256_t amount) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }

  // Check for overflow
  uint256_t new_balance = uint256_add(acc.balance, amount);
  if (uint256_lt(new_balance, acc.balance)) {
    return false; // Overflow
  }

  acc.balance = new_balance;
  world_state_set_account(ws, addr, &acc);
  return true;
}

static bool ws_sub_balance(state_access_t *state, const address_t *addr, uint256_t amount) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return uint256_is_zero(amount); // Can only subtract 0 from non-existent
  }

  // Check for underflow
  if (uint256_lt(acc.balance, amount)) {
    return false; // Insufficient balance
  }

  acc.balance = uint256_sub(acc.balance, amount);
  world_state_set_account(ws, addr, &acc);
  return true;
}

static uint64_t ws_get_nonce(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return 0;
  }
  return acc.nonce;
}

static void ws_set_nonce(state_access_t *state, const address_t *addr, uint64_t nonce) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  acc.nonce = nonce;
  world_state_set_account(ws, addr, &acc);
}

static uint64_t ws_increment_nonce(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    acc = account_empty();
  }
  uint64_t old_nonce = acc.nonce;

  // Check for nonce overflow (EIP-2681 limits nonce to 2^64-2)
  if (acc.nonce >= UINT64_MAX - 1) {
    return old_nonce; // Saturate at max, don't increment
  }

  acc.nonce++;
  world_state_set_account(ws, addr, &acc);
  return old_nonce;
}

static bytes_t ws_get_code(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  code_map *c_map = (code_map *)ws->code_store;

  const code_map_value *entry = code_map_get(c_map, *addr);
  if (entry != nullptr) {
    return entry->second;
  }

  bytes_t empty;
  bytes_init(&empty);
  return empty;
}

static size_t ws_get_code_size(state_access_t *state, const address_t *addr) {
  bytes_t code = ws_get_code(state, addr);
  return code.size;
}

static hash_t ws_get_code_hash(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  account_t acc;
  if (!world_state_get_account(ws, addr, &acc)) {
    return EMPTY_CODE_HASH;
  }
  return acc.code_hash;
}

static void ws_set_code(state_access_t *state, const address_t *addr, const uint8_t *code,
                        size_t code_len) {
  world_state_t *ws = (world_state_t *)state;

  // Store code in code map
  code_map *c_map = (code_map *)ws->code_store;
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

static uint256_t ws_get_storage(state_access_t *state, const address_t *addr, uint256_t slot) {
  world_state_t *ws = (world_state_t *)state;

  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;
  const storage_trie_map_value *entry = storage_trie_map_get(st_map, *addr);
  if (entry == nullptr) {
    return uint256_zero();
  }

  mpt_t *storage = entry->second;
  hash_t key = slot_to_key(slot);
  bytes_t value = mpt_get(storage, key.bytes, HASH_SIZE);
  if (value.data == nullptr || value.size == 0) {
    return uint256_zero();
  }

  // Value is stored as minimal big-endian bytes
  return uint256_from_bytes_be(value.data, value.size);
}

static uint256_t ws_get_original_storage(state_access_t *state, const address_t *addr,
                                         uint256_t slot) {
  world_state_t *ws = (world_state_t *)state;
  original_storage_map *orig_map = (original_storage_map *)ws->original_storage;

  warm_slot_key_t key = {.addr = *addr, .slot = slot};
  const original_storage_map_value *entry = original_storage_map_get(orig_map, key);

  if (entry != nullptr) {
    return entry->second;
  }

  // Not yet tracked - return current value (no writes yet this tx)
  return ws_get_storage(state, addr, slot);
}

static void ws_set_storage(state_access_t *state, const address_t *addr, uint256_t slot,
                           uint256_t value) {
  world_state_t *ws = (world_state_t *)state;

  // Record original value on first write (for EIP-2200 gas calculation)
  original_storage_map *orig_map = (original_storage_map *)ws->original_storage;
  warm_slot_key_t slot_key = {.addr = *addr, .slot = slot};
  if (!original_storage_map_contains(orig_map, slot_key)) {
    uint256_t original = ws_get_storage(state, addr, slot);
    original_storage_map_insert(orig_map, slot_key, original);
  }

  // Mark address as having dirty storage for efficient state root computation
  dirty_addr_set *dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_insert(dirty, *addr);

  // Track slot for post-state export
  all_slots_set *all_slots = (all_slots_set *)ws->all_storage_slots;
  if (uint256_is_zero(value)) {
    // Remove slot from tracking on deletion
    all_slots_set_erase(all_slots, slot_key);
  } else {
    // Track non-zero slots
    all_slots_set_insert(all_slots, slot_key);
  }

  mpt_t *storage = world_state_get_storage_trie(ws, addr);
  hash_t key = slot_to_key(slot);

  if (uint256_is_zero(value)) {
    // Delete the slot
    mpt_delete(storage, key.bytes, HASH_SIZE);
  } else {
    // Encode value as minimal bytes (strip leading zeros)
    uint8_t be_bytes[32];
    uint256_to_bytes_be(value, be_bytes);

    size_t start = 0;
    while (start < 32 && be_bytes[start] == 0) {
      start++;
    }

    mpt_insert(storage, key.bytes, HASH_SIZE, be_bytes + start, 32 - start);
  }
}

static bool ws_is_address_warm(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  warm_addr_set *set = (warm_addr_set *)ws->warm_addresses;
  return warm_addr_set_contains(set, *addr);
}

static bool ws_warm_address(state_access_t *state, const address_t *addr) {
  world_state_t *ws = (world_state_t *)state;
  warm_addr_set *set = (warm_addr_set *)ws->warm_addresses;

  if (warm_addr_set_contains(set, *addr)) {
    return false; // Already warm, not cold
  }

  warm_addr_set_insert(set, *addr);
  return true; // Was cold (first access)
}

static bool ws_is_slot_warm(state_access_t *state, const address_t *addr, uint256_t slot) {
  world_state_t *ws = (world_state_t *)state;
  warm_slot_set *set = (warm_slot_set *)ws->warm_slots;
  warm_slot_key_t key = {.addr = *addr, .slot = slot};
  return warm_slot_set_contains(set, key);
}

static bool ws_warm_slot(state_access_t *state, const address_t *addr, uint256_t slot) {
  world_state_t *ws = (world_state_t *)state;
  warm_slot_set *set = (warm_slot_set *)ws->warm_slots;
  warm_slot_key_t key = {.addr = *addr, .slot = slot};

  if (warm_slot_set_contains(set, key)) {
    return false; // Already warm, not cold
  }

  warm_slot_set_insert(set, key);
  return true; // Was cold (first access)
}

static void ws_begin_transaction(state_access_t *state) {
  world_state_t *ws = (world_state_t *)state;

  // Clear warm sets
  warm_addr_set *addr_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_clear(addr_set);

  warm_slot_set *slot_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_clear(slot_set);

  // Clear original storage tracking
  original_storage_map *orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_clear(orig_map);
}

// FIXME: Snapshot/revert is not yet implemented. These are stubs that return
// valid IDs but do NOT actually track or revert state changes. Full journaling
// support is required for proper CALL/CREATE revert semantics.
// See: https://github.com/daniellehrner/div0/issues/XX

static uint64_t ws_snapshot(state_access_t *state) {
  world_state_t *ws = (world_state_t *)state;
  // Returns incrementing ID for API compatibility, but no journaling occurs
  return ++ws->snapshot_counter;
}

static void ws_revert_to_snapshot(state_access_t *state, uint64_t snapshot_id) {
  (void)state;
  (void)snapshot_id;
  // FIXME: No-op - requires journaling all state changes since snapshot
}

static void ws_commit_snapshot(state_access_t *state, uint64_t snapshot_id) {
  (void)state;
  (void)snapshot_id;
  // FIXME: No-op - would discard journal entries for the snapshot
}

static hash_t ws_state_root(state_access_t *state) {
  world_state_t *ws = (world_state_t *)state;
  return world_state_root(ws);
}

static void ws_destroy(state_access_t *state) {
  world_state_t *ws = (world_state_t *)state;
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

world_state_t *world_state_create(div0_arena_t *arena) {
  if (arena == nullptr) {
    return nullptr;
  }

  // Set thread-local arena for STC containers
  div0_stc_arena = arena;

  world_state_t *ws = div0_arena_alloc(arena, sizeof(world_state_t));
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

  // Initialize snapshot counter
  ws->snapshot_counter = 0;

  return ws;

fail:
  // On failure, clean up any initialized STC containers
  // Note: STC _drop is safe to call on zero-initialized containers
  if (ws->storage_tries != nullptr) {
    storage_trie_map_drop((storage_trie_map *)ws->storage_tries);
  }
  if (ws->code_store != nullptr) {
    code_map_drop((code_map *)ws->code_store);
  }
  if (ws->warm_addresses != nullptr) {
    warm_addr_set_drop((warm_addr_set *)ws->warm_addresses);
  }
  if (ws->warm_slots != nullptr) {
    warm_slot_set_drop((warm_slot_set *)ws->warm_slots);
  }
  if (ws->original_storage != nullptr) {
    original_storage_map_drop((original_storage_map *)ws->original_storage);
  }
  if (ws->dirty_storage != nullptr) {
    dirty_addr_set_drop((dirty_addr_set *)ws->dirty_storage);
  }
  if (ws->all_accounts != nullptr) {
    all_accounts_set_drop((all_accounts_set *)ws->all_accounts);
  }
  if (ws->all_storage_slots != nullptr) {
    all_slots_set_drop((all_slots_set *)ws->all_storage_slots);
  }
  // Arena memory is not freed (owned by caller)
  return nullptr;
}

bool world_state_get_account(world_state_t *ws, const address_t *addr, account_t *out) {
  hash_t key = address_to_key(addr);
  bytes_t value = mpt_get(&ws->state_trie, key.bytes, HASH_SIZE);

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

bool world_state_set_account(world_state_t *ws, const address_t *addr, const account_t *acc) {
  hash_t key = address_to_key(addr);

  // EIP-161: Don't store empty accounts
  if (account_is_empty(acc)) {
    mpt_delete(&ws->state_trie, key.bytes, HASH_SIZE);
    // Remove from all_accounts set when account becomes empty
    all_accounts_set *all_accts = (all_accounts_set *)ws->all_accounts;
    all_accounts_set_erase(all_accts, *addr);
    return true;
  }

  // Track this address in all_accounts set for post-state export
  all_accounts_set *all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_insert(all_accts, *addr);

  // RLP-encode account
  bytes_t encoded = account_rlp_encode(acc, ws->arena);
  if (encoded.data == nullptr) {
    return false;
  }

  mpt_insert(&ws->state_trie, key.bytes, HASH_SIZE, encoded.data, encoded.size);
  return true;
}

mpt_t *world_state_get_storage_trie(world_state_t *ws, const address_t *addr) {
  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;

  // Check if storage trie exists
  const storage_trie_map_value *entry = storage_trie_map_get(st_map, *addr);
  if (entry != nullptr) {
    return entry->second;
  }

  // Create new storage trie
  mpt_backend_t *backend = mpt_memory_backend_create(ws->arena);
  if (backend == nullptr) {
    return nullptr;
  }

  mpt_t *storage = div0_arena_alloc(ws->arena, sizeof(mpt_t));
  if (storage == nullptr) {
    return nullptr;
  }

  mpt_init(storage, backend, ws->arena);
  storage_trie_map_insert(st_map, *addr, storage);

  return storage;
}

hash_t world_state_root(world_state_t *ws) {
  // Only update storage roots for accounts with dirty storage
  dirty_addr_set *dirty = (dirty_addr_set *)ws->dirty_storage;
  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;

  for (dirty_addr_set_iter it = dirty_addr_set_begin(dirty);
       it.ref != dirty_addr_set_end(dirty).ref; dirty_addr_set_next(&it)) {

    address_t addr = *it.ref;

    // Get storage trie for this address
    const storage_trie_map_value *entry = storage_trie_map_get(st_map, addr);
    if (entry == nullptr) {
      continue; // No storage trie (shouldn't happen if dirty)
    }
    mpt_t *storage = entry->second;

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

void world_state_clear(world_state_t *ws) {
  mpt_clear(&ws->state_trie);

  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_clear(st_map);

  code_map *c_map = (code_map *)ws->code_store;
  code_map_clear(c_map);

  warm_addr_set *wa_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_clear(wa_set);

  warm_slot_set *ws_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_clear(ws_set);

  original_storage_map *orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_clear(orig_map);

  dirty_addr_set *dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_clear(dirty);

  all_accounts_set *all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_clear(all_accts);

  all_slots_set *all_slots = (all_slots_set *)ws->all_storage_slots;
  all_slots_set_clear(all_slots);
}

void world_state_destroy(world_state_t *ws) {
  // Clean up hash tables
  storage_trie_map *st_map = (storage_trie_map *)ws->storage_tries;
  storage_trie_map_drop(st_map);

  code_map *c_map = (code_map *)ws->code_store;
  code_map_drop(c_map);

  warm_addr_set *wa_set = (warm_addr_set *)ws->warm_addresses;
  warm_addr_set_drop(wa_set);

  warm_slot_set *ws_set = (warm_slot_set *)ws->warm_slots;
  warm_slot_set_drop(ws_set);

  original_storage_map *orig_map = (original_storage_map *)ws->original_storage;
  original_storage_map_drop(orig_map);

  dirty_addr_set *dirty = (dirty_addr_set *)ws->dirty_storage;
  dirty_addr_set_drop(dirty);

  all_accounts_set *all_accts = (all_accounts_set *)ws->all_accounts;
  all_accounts_set_drop(all_accts);

  all_slots_set *all_slots = (all_slots_set *)ws->all_storage_slots;
  all_slots_set_drop(all_slots);

  // Note: Arena memory is not freed here (owned by caller)
}

// =============================================================================
// Post-State Export
// =============================================================================

/// Count storage slots for a specific address.
static size_t count_slots_for_address(all_slots_set *all_slots, const address_t *addr) {
  size_t count = 0;
  for (all_slots_set_iter it = all_slots_set_begin(all_slots);
       it.ref != all_slots_set_end(all_slots).ref; all_slots_set_next(&it)) {
    if (address_equal(&it.ref->addr, addr)) {
      count++;
    }
  }
  return count;
}

bool world_state_snapshot(world_state_t *ws, div0_arena_t *arena, state_snapshot_t *out) {
  all_accounts_set *all_accts = (all_accounts_set *)ws->all_accounts;
  all_slots_set *all_slots = (all_slots_set *)ws->all_storage_slots;
  code_map *c_map = (code_map *)ws->code_store;

  // Count accounts
  size_t count = (size_t)all_accounts_set_size(all_accts);
  if (count == 0) {
    out->accounts = nullptr;
    out->account_count = 0;
    return true;
  }

  // Allocate accounts array
  out->accounts = div0_arena_alloc(arena, count * sizeof(account_snapshot_t));
  if (out->accounts == nullptr) {
    return false;
  }
  out->account_count = count;

  // Iterate over all accounts
  size_t idx = 0;
  for (all_accounts_set_iter it = all_accounts_set_begin(all_accts);
       it.ref != all_accounts_set_end(all_accts).ref; all_accounts_set_next(&it)) {

    address_t addr = *it.ref;
    account_snapshot_t *snap_acc = &out->accounts[idx];

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
    const code_map_value *code_entry = code_map_get(c_map, addr);
    if (code_entry != nullptr && code_entry->second.size > 0) {
      // Copy code to arena
      snap_acc->code.size = code_entry->second.size;
      snap_acc->code.data = div0_arena_alloc(arena, code_entry->second.size);
      if (snap_acc->code.data == nullptr) {
        return false;
      }
      __builtin___memcpy_chk(snap_acc->code.data, code_entry->second.data, code_entry->second.size,
                             code_entry->second.size);
    }

    // Get storage slots for this account
    size_t slot_count = count_slots_for_address(all_slots, &addr);
    if (slot_count > 0) {
      snap_acc->storage = div0_arena_alloc(arena, slot_count * sizeof(storage_entry_t));
      if (snap_acc->storage == nullptr) {
        return false;
      }

      // Populate storage entries
      size_t slot_idx = 0;
      for (all_slots_set_iter slot_it = all_slots_set_begin(all_slots);
           slot_it.ref != all_slots_set_end(all_slots).ref; all_slots_set_next(&slot_it)) {
        if (address_equal(&slot_it.ref->addr, &addr)) {
          uint256_t slot = slot_it.ref->slot;
          uint256_t value = ws_get_storage(&ws->base, &addr, slot);

          // Only include non-zero values
          if (!uint256_is_zero(value)) {
            snap_acc->storage[slot_idx].slot = slot;
            snap_acc->storage[slot_idx].value = value;
            slot_idx++;
          }
        }
      }
      snap_acc->storage_count = slot_idx;
    }

    idx++;
  }

  // Adjust count if any accounts were skipped
  out->account_count = idx;

  return true;
}
