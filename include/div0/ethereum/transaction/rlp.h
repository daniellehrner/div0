#ifndef DIV0_ETHEREUM_TRANSACTION_RLP_H
#define DIV0_ETHEREUM_TRANSACTION_RLP_H

#include <cstdint>
#include <optional>
#include <span>

#include "div0/ethereum/transaction/eip1559_tx.h"
#include "div0/ethereum/transaction/eip2930_tx.h"
#include "div0/ethereum/transaction/eip4844_tx.h"
#include "div0/ethereum/transaction/eip7702_tx.h"
#include "div0/ethereum/transaction/legacy_tx.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

namespace div0::crypto {
class Secp256k1Context;
}

namespace div0::ethereum {

// =============================================================================
// Error Types
// =============================================================================

/**
 * @brief Transaction decode error codes.
 */
enum class TxDecodeError : uint8_t {
  Success,
  EmptyInput,
  InvalidType,
  InvalidRlp,
  InvalidSignature,
  MissingField,
  InvalidFieldCount,
};

/**
 * @brief Result of a transaction decode operation.
 */
template <typename T>
struct TxDecodeResult {
  T value{};
  TxDecodeError error{TxDecodeError::Success};

  [[nodiscard]] bool ok() const noexcept { return error == TxDecodeError::Success; }
  explicit operator bool() const noexcept { return ok(); }
};

// =============================================================================
// Encoding - Full signed transaction (for tx hash computation)
// =============================================================================

[[nodiscard]] types::Bytes rlp_encode(const LegacyTx& tx);
[[nodiscard]] types::Bytes rlp_encode(const Eip2930Tx& tx);
[[nodiscard]] types::Bytes rlp_encode(const Eip1559Tx& tx);
[[nodiscard]] types::Bytes rlp_encode(const Eip4844Tx& tx);
[[nodiscard]] types::Bytes rlp_encode(const Eip7702Tx& tx);
[[nodiscard]] types::Bytes rlp_encode(const Transaction& tx);

// =============================================================================
// Decoding - From RLP bytes
// =============================================================================

/**
 * @brief Decode a legacy transaction from RLP bytes.
 * @param data Full RLP-encoded transaction (starting with list header)
 */
[[nodiscard]] TxDecodeResult<LegacyTx> rlp_decode_legacy_tx(std::span<const uint8_t> data);

/**
 * @brief Decode an EIP-2930 transaction from RLP bytes.
 * @param data RLP payload (after 0x01 type byte has been stripped)
 */
[[nodiscard]] TxDecodeResult<Eip2930Tx> rlp_decode_eip2930_tx(std::span<const uint8_t> data);

/**
 * @brief Decode an EIP-1559 transaction from RLP bytes.
 * @param data RLP payload (after 0x02 type byte has been stripped)
 */
[[nodiscard]] TxDecodeResult<Eip1559Tx> rlp_decode_eip1559_tx(std::span<const uint8_t> data);

/**
 * @brief Decode an EIP-4844 transaction from RLP bytes.
 * @param data RLP payload (after 0x03 type byte has been stripped)
 */
[[nodiscard]] TxDecodeResult<Eip4844Tx> rlp_decode_eip4844_tx(std::span<const uint8_t> data);

/**
 * @brief Decode an EIP-7702 transaction from RLP bytes.
 * @param data RLP payload (after 0x04 type byte has been stripped)
 */
[[nodiscard]] TxDecodeResult<Eip7702Tx> rlp_decode_eip7702_tx(std::span<const uint8_t> data);

/**
 * @brief Decode any transaction type with automatic envelope detection.
 *
 * For legacy transactions (type 0): data starts with RLP list prefix (>= 0xc0)
 * For typed transactions (types 1-4): data starts with type byte (0x01-0x04)
 */
[[nodiscard]] TxDecodeResult<Transaction> rlp_decode_transaction(std::span<const uint8_t> data);

// =============================================================================
// Transaction Hash (keccak256 of RLP-encoded tx)
// =============================================================================

[[nodiscard]] types::Hash tx_hash(const LegacyTx& tx);
[[nodiscard]] types::Hash tx_hash(const Eip2930Tx& tx);
[[nodiscard]] types::Hash tx_hash(const Eip1559Tx& tx);
[[nodiscard]] types::Hash tx_hash(const Eip4844Tx& tx);
[[nodiscard]] types::Hash tx_hash(const Eip7702Tx& tx);
[[nodiscard]] types::Hash tx_hash(const Transaction& tx);

// =============================================================================
// Signing Hash (for sender recovery)
// =============================================================================

/**
 * @brief Compute the signing hash for a legacy transaction.
 *
 * For pre-EIP-155: keccak256(rlp([nonce, gas_price, gas_limit, to, value, data]))
 * For EIP-155: keccak256(rlp([nonce, gas_price, gas_limit, to, value, data, chain_id, 0, 0]))
 */
[[nodiscard]] types::Hash signing_hash(const LegacyTx& tx);

/**
 * @brief Compute the signing hash for an EIP-2930 transaction.
 * keccak256(0x01 || rlp([chain_id, nonce, gas_price, gas_limit, to, value, data, access_list]))
 */
[[nodiscard]] types::Hash signing_hash(const Eip2930Tx& tx);

/**
 * @brief Compute the signing hash for an EIP-1559 transaction.
 */
[[nodiscard]] types::Hash signing_hash(const Eip1559Tx& tx);

/**
 * @brief Compute the signing hash for an EIP-4844 transaction.
 */
[[nodiscard]] types::Hash signing_hash(const Eip4844Tx& tx);

/**
 * @brief Compute the signing hash for an EIP-7702 transaction.
 */
[[nodiscard]] types::Hash signing_hash(const Eip7702Tx& tx);

[[nodiscard]] types::Hash signing_hash(const Transaction& tx);

// =============================================================================
// Sender Recovery
// =============================================================================

/**
 * @brief Recover the sender address from a signed transaction.
 *
 * Uses the signature (v/y_parity, r, s) and signing hash to recover the
 * public key via ECDSA recovery, then derives the address.
 *
 * @return The sender address, or nullopt if recovery fails
 */
[[nodiscard]] std::optional<types::Address> recover_sender(const LegacyTx& tx,
                                                           crypto::Secp256k1Context& ctx);
[[nodiscard]] std::optional<types::Address> recover_sender(const Eip2930Tx& tx,
                                                           crypto::Secp256k1Context& ctx);
[[nodiscard]] std::optional<types::Address> recover_sender(const Eip1559Tx& tx,
                                                           crypto::Secp256k1Context& ctx);
[[nodiscard]] std::optional<types::Address> recover_sender(const Eip4844Tx& tx,
                                                           crypto::Secp256k1Context& ctx);
[[nodiscard]] std::optional<types::Address> recover_sender(const Eip7702Tx& tx,
                                                           crypto::Secp256k1Context& ctx);
[[nodiscard]] std::optional<types::Address> recover_sender(const Transaction& tx,
                                                           crypto::Secp256k1Context& ctx);

// =============================================================================
// AccessList / AuthorizationList RLP Helpers
// =============================================================================

/**
 * @brief Compute RLP encoded length for an access list.
 * access_list: [[address, [storage_key, ...]], ...]
 */
[[nodiscard]] size_t rlp_encoded_length(const AccessList& list);

/**
 * @brief Compute RLP encoded length for an authorization list.
 * authorization_list: [[chain_id, address, nonce, y_parity, r, s], ...]
 */
[[nodiscard]] size_t rlp_encoded_length(const AuthorizationList& list);

}  // namespace div0::ethereum

// Forward declarations for RLP encoder/decoder (in div0::rlp namespace)
namespace div0::rlp {
class RlpEncoder;
class RlpDecoder;
}  // namespace div0::rlp

namespace div0::ethereum {

/**
 * @brief Encode an access list using an existing RlpEncoder.
 * Used by typed transaction encode functions.
 */
void rlp_encode_access_list(rlp::RlpEncoder& enc, const AccessList& list);

/**
 * @brief Encode an authorization list using an existing RlpEncoder.
 * Used by EIP-7702 transaction encode functions.
 */
void rlp_encode_authorization_list(rlp::RlpEncoder& enc, const AuthorizationList& list);

/**
 * @brief Decode an access list using an existing RlpDecoder.
 * @return TxDecodeError::Success on success, error code otherwise
 */
[[nodiscard]] TxDecodeError rlp_decode_access_list(rlp::RlpDecoder& dec, AccessList& list);

/**
 * @brief Decode an authorization list using an existing RlpDecoder.
 * @return TxDecodeError::Success on success, error code otherwise
 */
[[nodiscard]] TxDecodeError rlp_decode_authorization_list(rlp::RlpDecoder& dec,
                                                          AuthorizationList& list);

/**
 * @brief Decode an optional 'to' address from RLP bytes.
 * Empty bytes = contract creation (nullopt), 20 bytes = address.
 * @return nullopt on invalid input (not empty and not 20 bytes)
 */
[[nodiscard]] std::optional<std::optional<types::Address>> rlp_decode_optional_address(
    rlp::RlpDecoder& dec);

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_RLP_H
