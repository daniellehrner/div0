#ifndef DIV0_ETHEREUM_TRANSACTION_RLP_H
#define DIV0_ETHEREUM_TRANSACTION_RLP_H

#include "div0/ethereum/transaction/transaction.h"
#include "div0/mem/arena.h"
#include "div0/types/hash.h"

#include <stddef.h>
#include <stdint.h>

/// Transaction decode error codes.
typedef enum {
  TX_DECODE_OK = 0,
  TX_DECODE_EMPTY_INPUT,
  TX_DECODE_INVALID_TYPE,
  TX_DECODE_INVALID_RLP,
  TX_DECODE_INVALID_SIGNATURE,
  TX_DECODE_MISSING_FIELD,
  TX_DECODE_INVALID_FIELD,
  TX_DECODE_ALLOC_FAILED,
} tx_decode_error_t;

/// Result of transaction decoding.
typedef struct {
  tx_decode_error_t error;
  size_t bytes_consumed;
} tx_decode_result_t;

/// Decodes a transaction from RLP data.
/// Handles all transaction types (Legacy, EIP-2930, EIP-1559, EIP-4844, EIP-7702).
///
/// Transaction type detection:
/// - First byte >= 0xc0: Legacy transaction (RLP list prefix)
/// - First byte 0x01-0x04: Typed transaction (EIP-2718)
///
/// @param data Input RLP data
/// @param len Length of input data
/// @param tx Output transaction (caller provides storage)
/// @param arena Arena for dynamic allocations (data, access lists, etc.)
/// @return Decode result with error code
tx_decode_result_t transaction_decode(const uint8_t *data, size_t len, transaction_t *tx,
                                      div0_arena_t *arena);

/// Decodes a legacy transaction from RLP.
/// @param data Input RLP data (should start with list prefix)
/// @param len Length of input data
/// @param tx Output transaction
/// @param arena Arena for allocations
/// @return Decode result
tx_decode_result_t legacy_tx_decode(const uint8_t *data, size_t len, legacy_tx_t *tx,
                                    div0_arena_t *arena);

/// Decodes an EIP-2930 transaction from RLP payload.
/// @param data Input RLP data (after 0x01 type byte)
/// @param len Length of input data
/// @param tx Output transaction
/// @param arena Arena for allocations
/// @return Decode result
tx_decode_result_t eip2930_tx_decode(const uint8_t *data, size_t len, eip2930_tx_t *tx,
                                     div0_arena_t *arena);

/// Decodes an EIP-1559 transaction from RLP payload.
/// @param data Input RLP data (after 0x02 type byte)
/// @param len Length of input data
/// @param tx Output transaction
/// @param arena Arena for allocations
/// @return Decode result
tx_decode_result_t eip1559_tx_decode(const uint8_t *data, size_t len, eip1559_tx_t *tx,
                                     div0_arena_t *arena);

/// Decodes an EIP-4844 transaction from RLP payload.
/// @param data Input RLP data (after 0x03 type byte)
/// @param len Length of input data
/// @param tx Output transaction
/// @param arena Arena for allocations
/// @return Decode result
tx_decode_result_t eip4844_tx_decode(const uint8_t *data, size_t len, eip4844_tx_t *tx,
                                     div0_arena_t *arena);

/// Decodes an EIP-7702 transaction from RLP payload.
/// @param data Input RLP data (after 0x04 type byte)
/// @param len Length of input data
/// @param tx Output transaction
/// @param arena Arena for allocations
/// @return Decode result
tx_decode_result_t eip7702_tx_decode(const uint8_t *data, size_t len, eip7702_tx_t *tx,
                                     div0_arena_t *arena);

/// Returns a human-readable error message.
/// @param error Error code
/// @return Static string describing the error
const char *tx_decode_error_string(tx_decode_error_t error);

// ============================================================================
// RLP Encoding Helpers
// ============================================================================

/// Encodes an optional address (nullptr encodes as empty bytes).
/// @param arena Arena for allocations
/// @param addr Address to encode (may be nullptr)
/// @return RLP-encoded bytes
bytes_t tx_encode_optional_address(div0_arena_t *arena, const address_t *addr);

/// Encodes an access list to RLP.
/// @param output Output buffer to append to
/// @param list Access list to encode
/// @param arena Arena for allocations
void tx_encode_access_list(bytes_t *output, const access_list_t *list, div0_arena_t *arena);

/// Encodes an authorization list to RLP (including signatures).
/// @param output Output buffer to append to
/// @param list Authorization list to encode
/// @param arena Arena for allocations
void tx_encode_authorization_list(bytes_t *output, const authorization_list_t *list,
                                  div0_arena_t *arena);

// ============================================================================
// RLP Encoding
// ============================================================================

/// Encodes a transaction to RLP format.
/// For typed transactions, includes the type byte prefix.
/// @param tx Transaction to encode
/// @param arena Arena for allocations
/// @return Encoded bytes (empty on failure)
bytes_t transaction_encode(const transaction_t *tx, div0_arena_t *arena);

/// Encodes a legacy transaction to RLP.
/// @param tx Transaction to encode
/// @param arena Arena for allocations
/// @return Encoded bytes
bytes_t legacy_tx_encode(const legacy_tx_t *tx, div0_arena_t *arena);

/// Encodes an EIP-2930 transaction to RLP (with 0x01 prefix).
bytes_t eip2930_tx_encode(const eip2930_tx_t *tx, div0_arena_t *arena);

/// Encodes an EIP-1559 transaction to RLP (with 0x02 prefix).
bytes_t eip1559_tx_encode(const eip1559_tx_t *tx, div0_arena_t *arena);

/// Encodes an EIP-4844 transaction to RLP (with 0x03 prefix).
bytes_t eip4844_tx_encode(const eip4844_tx_t *tx, div0_arena_t *arena);

/// Encodes an EIP-7702 transaction to RLP (with 0x04 prefix).
bytes_t eip7702_tx_encode(const eip7702_tx_t *tx, div0_arena_t *arena);

// ============================================================================
// Transaction Hash
// ============================================================================

/// Computes the transaction hash (keccak256 of RLP encoding).
/// This is the transaction ID used on-chain.
/// @param tx Transaction to hash
/// @param arena Arena for temporary allocations
/// @return Transaction hash
hash_t transaction_hash(const transaction_t *tx, div0_arena_t *arena);

#endif // DIV0_ETHEREUM_TRANSACTION_RLP_H
