# Merkle Patricia Trie

The div0 Merkle Patricia Trie (MPT) is a cryptographically authenticated key-value store used by Ethereum for state, storage, and transaction/receipt tries. It combines a radix trie for efficient key lookups with Merkle hashing for cryptographic proofs.

## Overview

### Why Merkle Patricia Trie?

Ethereum requires a data structure that provides:
- **Efficient lookups**: O(log n) key-value operations
- **Cryptographic commitment**: A single 32-byte root hash authenticates the entire state
- **Proof generation**: Compact proofs that a key-value exists (or doesn't)
- **Deterministic ordering**: Same data always produces same root hash

The MPT achieves this by:
- **Path compression**: Extension nodes eliminate long chains of single-child branches
- **Nibble-based keys**: Keys are split into 4-bit nibbles for 16-way branching
- **RLP encoding**: Canonical serialization for deterministic hashing
- **Hash caching**: Avoids recomputing unchanged subtree hashes

### Node Types

```
┌──────────────────────────────────────────────────────────────────┐
│                       MPT Node Types                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  LEAF NODE                                                       │
│  ┌────────────────────────────────────────────────────┐          │
│  │ path: remaining nibbles  │  value: stored data     │          │
│  └────────────────────────────────────────────────────┘          │
│  RLP: [hex_prefix(path, is_leaf=true), value]                    │
│                                                                  │
│  EXTENSION NODE                                                  │
│  ┌────────────────────────────────────────────────────┐          │
│  │ path: shared prefix      │  child: node reference  │          │
│  └────────────────────────────────────────────────────┘          │
│  RLP: [hex_prefix(path, is_leaf=false), child_ref]               │
│                                                                  │
│  BRANCH NODE                                                     │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │ [0] [1] [2] ... [15] │  value (optional)                 │    │
│  └──────────────────────────────────────────────────────────┘    │
│  RLP: [child0, child1, ..., child15, value]                      │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### Architecture

```
┌───────────────────────────────────────────────────────────────────┐
│                            mpt_t                                  │
│  ┌──────────────┐     ┌──────────────────────────────────────┐    │
│  │   backend    │────▶│         mpt_backend_t                │    │
│  ├──────────────┤     │  ┌────────────────────────────────┐  │    │
│  │  work_arena  │     │  │  vtable (get_root, set_root,   │  │    │
│  └──────────────┘     │  │  alloc_node, store_node, ...)  │  │    │
│                       │  └────────────────────────────────┘  │    │
│                       └──────────────────────────────────────┘    │
│                                      │                            │
│                                      ▼                            │
│                       ┌───────────────────────────────────────┐   │
│                       │           mpt_node_t                  │   │
│                       │  ┌──────────────────────────────────┐ │   │
│                       │  │ type: LEAF | EXTENSION | BRANCH  │ │   │
│                       │  │ cached_hash, hash_valid          │ │   │
│                       │  │ union { leaf, extension, branch }│ │   │
│                       │  └──────────────────────────────────┘ │   │
│                       └───────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────────┘
```

## API Reference

### Types

```c
/// Node types in the Merkle Patricia Trie.
typedef enum {
  MPT_NODE_EMPTY,     // Empty node (null)
  MPT_NODE_LEAF,      // Leaf node: terminates path with value
  MPT_NODE_EXTENSION, // Extension node: shared path prefix
  MPT_NODE_BRANCH     // Branch node: 16-way branch + optional value
} mpt_node_type_t;

/// Reference to a child node.
/// Can be either embedded bytes (RLP < 32 bytes) or hash (RLP >= 32 bytes).
typedef struct {
  union {
    bytes_t embedded; // RLP-encoded node (when < 32 bytes)
    hash_t hash;      // keccak256 of RLP-encoded node
  };
  mpt_node_t *node;   // Direct pointer (for in-memory backends)
  bool is_hash;       // True if this is a hash reference
} node_ref_t;

/// Merkle Patricia Trie handle.
struct mpt {
  mpt_backend_t *backend;   // Storage backend
  div0_arena_t *work_arena; // Working arena for temporary allocations
};

/// Empty root hash constant (keccak256 of RLP-encoded empty string: 0x80).
extern const hash_t MPT_EMPTY_ROOT;
// = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
```

### Core Functions

#### `mpt_init`

```c
void mpt_init(mpt_t *mpt, mpt_backend_t *backend, div0_arena_t *work_arena);
```

Initialize an MPT with a storage backend.

- **mpt**: MPT structure to initialize
- **backend**: Storage backend (e.g., from `mpt_memory_backend_create`)
- **work_arena**: Arena for temporary allocations during operations

#### `mpt_insert`

```c
[[nodiscard]] bool mpt_insert(mpt_t *mpt, const uint8_t *key, size_t key_len,
                               const uint8_t *value, size_t value_len);
```

Insert or update a key-value pair.

- Existing keys are overwritten
- Empty values are allowed (but see `mpt_delete` for removal)
- **Returns**: `true` on success, `false` on error

#### `mpt_get`

```c
[[nodiscard]] bytes_t mpt_get(mpt_t *mpt, const uint8_t *key, size_t key_len);
```

Get value for a key.

- **Returns**: Value bytes (empty if not found), arena-backed
- Returned bytes are valid until arena is reset

#### `mpt_contains`

```c
[[nodiscard]] bool mpt_contains(mpt_t *mpt, const uint8_t *key, size_t key_len);
```

Check if a key exists.

- More efficient than `mpt_get` when value isn't needed
- **Returns**: `true` if key exists

#### `mpt_delete`

```c
bool mpt_delete(mpt_t *mpt, const uint8_t *key, size_t key_len);
```

Delete a key from the trie.

- Properly collapses branch nodes when children are removed
- **Returns**: `true` if key was deleted, `false` if not found

#### `mpt_root_hash`

```c
[[nodiscard]] hash_t mpt_root_hash(mpt_t *mpt);
```

Compute the root hash of the trie.

- Uses cached hashes where possible (incremental computation)
- **Returns**: `MPT_EMPTY_ROOT` if trie is empty

#### `mpt_is_empty`

```c
[[nodiscard]] bool mpt_is_empty(mpt_t *mpt);
```

Check if the trie is empty.

#### `mpt_clear`

```c
void mpt_clear(mpt_t *mpt);
```

Clear all entries, resetting to an empty trie.

#### `mpt_destroy`

```c
void mpt_destroy(mpt_t *mpt);
```

Destroy the MPT and its backend.

### Backend Functions

#### `mpt_memory_backend_create`

```c
[[nodiscard]] mpt_backend_t *mpt_memory_backend_create(div0_arena_t *arena);
```

Create an in-memory storage backend.

- All nodes are stored directly in the arena
- Fast direct pointer access (no hash-based lookups needed)
- Suitable for transaction processing (not persistent)

## Usage Examples

### Basic Usage

```c
#include "div0/mem/arena.h"
#include "div0/trie/mpt.h"

// Initialize arena
div0_arena_t arena;
div0_arena_init(&arena);

// Create in-memory backend and MPT
mpt_backend_t *backend = mpt_memory_backend_create(&arena);
mpt_t trie;
mpt_init(&trie, backend, &arena);

// Insert key-value pairs
mpt_insert(&trie, (uint8_t *)"dog", 3, (uint8_t *)"puppy", 5);
mpt_insert(&trie, (uint8_t *)"cat", 3, (uint8_t *)"kitten", 6);

// Retrieve values
bytes_t value = mpt_get(&trie, (uint8_t *)"dog", 3);
// value.data = "puppy", value.size = 5

// Get root hash for verification
hash_t root = mpt_root_hash(&trie);

// Cleanup
mpt_destroy(&trie);
div0_arena_destroy(&arena);
```

### State Trie Usage

For Ethereum state storage where keys are hashed addresses:

```c
#include "div0/crypto/keccak256.h"
#include "div0/trie/mpt.h"

// Key is keccak256(address)
address_t addr = { /* 20-byte address */ };
hash_t key = keccak256(addr.bytes, ADDRESS_SIZE);

// Value is RLP-encoded account
bytes_t account_rlp = account_rlp_encode(&account, &arena);

// Insert into state trie
mpt_insert(&state_trie, key.bytes, HASH_SIZE,
           account_rlp.data, account_rlp.size);
```

### Storage Trie Usage

For contract storage where keys and values are uint256:

```c
// Storage key is keccak256(slot)
uint256_t slot = uint256_from_u64(1);
uint8_t slot_bytes[32];
uint256_to_bytes_be(&slot, slot_bytes);
hash_t key = keccak256(slot_bytes, 32);

// Value is RLP-encoded uint256 (no leading zeros)
uint256_t value = uint256_from_u64(42);
bytes_t value_rlp = rlp_encode_uint256(&arena, &value);

mpt_insert(&storage_trie, key.bytes, HASH_SIZE,
           value_rlp.data, value_rlp.size);
```

## Key Concepts

### Nibbles and Hex-Prefix Encoding

Keys are converted to nibbles (4-bit values, 0-15) for tree traversal:

```
Key bytes:    [0xCA, 0xFE]
Nibbles:      [12, 10, 15, 14]  (C, A, F, E)
```

Hex-prefix encoding stores paths with a flag byte:

| Prefix | Meaning |
|--------|---------|
| 0x00.. | Extension, even nibbles |
| 0x1.   | Extension, odd nibbles (first nibble in prefix) |
| 0x20.. | Leaf, even nibbles |
| 0x3.   | Leaf, odd nibbles |

### Node References

Child nodes are referenced by:
- **Embedded**: If RLP encoding < 32 bytes, stored inline
- **Hash**: If RLP encoding >= 32 bytes, stored as keccak256 hash

This optimization reduces storage for small nodes while maintaining security.

### Hash Caching

Each node caches its hash after computation:
- Modified nodes have `hash_valid = false`
- `mpt_root_hash` recomputes only changed paths
- Enables efficient incremental updates

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `insert` | O(key_length) | Path-based traversal |
| `get` | O(key_length) | Path-based traversal |
| `delete` | O(key_length) | May restructure nodes |
| `root_hash` | O(modified nodes) | Incremental with caching |

### Memory Usage

- Leaf: ~80 bytes + path + value
- Extension: ~80 bytes + path
- Branch: ~600 bytes (16 child refs + value)
- Hash cache: 32 bytes per node

## Backend Interface

The MPT uses a vtable pattern for storage abstraction:

```c
typedef struct {
  mpt_node_t *(*get_root)(mpt_backend_t *backend);
  void (*set_root)(mpt_backend_t *backend, mpt_node_t *root);
  mpt_node_t *(*alloc_node)(mpt_backend_t *backend);
  mpt_node_t *(*get_node_by_hash)(mpt_backend_t *backend, const hash_t *hash);
  hash_t (*store_node)(mpt_backend_t *backend, mpt_node_t *node);
  void (*begin_batch)(mpt_backend_t *backend);
  void (*commit_batch)(mpt_backend_t *backend);
  void (*rollback_batch)(mpt_backend_t *backend);
  void (*clear)(mpt_backend_t *backend);
  void (*destroy)(mpt_backend_t *backend);
} mpt_backend_vtable_t;
```

This allows different backends (in-memory, LevelDB, etc.) without changing MPT logic.

## Thread Safety

The MPT is **not thread-safe**. Each thread should use its own MPT instance, or external synchronization must be used.

## Files

| File | Description |
|------|-------------|
| `include/div0/trie/mpt.h` | MPT handle and operations |
| `include/div0/trie/node.h` | Node types and functions |
| `include/div0/trie/nibbles.h` | Nibble array utilities |
| `include/div0/trie/hex_prefix.h` | Hex-prefix encoding |
| `src/trie/mpt.c` | MPT implementation |
| `src/trie/mpt_memory.c` | In-memory backend |
| `src/trie/node.c` | Node RLP encoding/hashing |
| `src/trie/nibbles.c` | Nibble operations |
| `src/trie/hex_prefix.c` | Hex-prefix codec |