#ifndef DIV0_ETHEREUM_TRANSACTION_AUTHORIZATION_H
#define DIV0_ETHEREUM_TRANSACTION_AUTHORIZATION_H

#include "div0/mem/arena.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Authorization tuple for EIP-7702 SetCode transactions.
/// Allows an EOA to delegate code execution to a contract address.
/// Signing message: keccak256(0x05 || rlp([chain_id, address, nonce]))
typedef struct {
  uint64_t chain_id; // 0 = valid on any chain
  address_t address; // Contract address to delegate to
  uint64_t nonce;    // Authorization nonce (distinct from tx nonce)
  uint8_t y_parity;  // 0 or 1
  uint256_t r;
  uint256_t s;
} authorization_t;

/// Authorization list: array of authorization tuples.
typedef struct {
  authorization_t *entries; // Arena-allocated array
  size_t count;
} authorization_list_t;

/// Initializes an empty authorization list.
static inline void authorization_list_init(authorization_list_t *list) {
  list->entries = nullptr;
  list->count = 0;
}

/// Allocates space for authorizations in the list.
/// @param list Authorization list to allocate
/// @param count Number of authorizations
/// @param arena Arena for allocation
/// @return true on success, false on allocation failure
static inline bool authorization_list_alloc(authorization_list_t *list, size_t count,
                                            div0_arena_t *arena) {
  if (count == 0) {
    list->entries = nullptr;
    list->count = 0;
    return true;
  }
  list->entries = (authorization_t *)div0_arena_alloc(arena, count * sizeof(authorization_t));
  if (!list->entries) {
    return false;
  }
  list->count = count;
  return true;
}

/// Returns the recovery ID (0 or 1) from y_parity.
static inline int authorization_recovery_id(const authorization_t *auth) {
  return auth->y_parity;
}

#endif // DIV0_ETHEREUM_TRANSACTION_AUTHORIZATION_H
