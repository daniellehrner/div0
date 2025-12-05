#include <gtest/gtest.h>

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/receipt.h"
#include "div0/ethereum/transaction/rlp.h"

namespace div0::ethereum {
namespace {

// =============================================================================
// LegacyTx RLP Tests
// =============================================================================

TEST(LegacyTxRlp, RoundTrip) {
  LegacyTx tx;
  tx.nonce = 9;
  tx.gas_price = types::Uint256(20000000000ULL);  // 20 gwei
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(0x7890123456789012ULL));
  tx.value = types::Uint256(1000000000000000000ULL);  // 1 ETH
  tx.data = {};
  tx.v = 37;  // EIP-155 with chain_id=1
  tx.r = types::Uint256(0x1234567890abcdefULL);
  tx.s = types::Uint256(0xfedcba0987654321ULL);

  auto encoded = rlp_encode(tx);
  EXPECT_FALSE(encoded.empty());

  auto decoded = rlp_decode_legacy_tx(encoded);
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value.nonce, tx.nonce);
  EXPECT_EQ(decoded.value.gas_price, tx.gas_price);
  EXPECT_EQ(decoded.value.gas_limit, tx.gas_limit);
  ASSERT_TRUE(decoded.value.to.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*decoded.value.to, *tx.to);
  EXPECT_EQ(decoded.value.value, tx.value);
  EXPECT_EQ(decoded.value.data, tx.data);
  EXPECT_EQ(decoded.value.v, tx.v);
  EXPECT_EQ(decoded.value.r, tx.r);
  EXPECT_EQ(decoded.value.s, tx.s);
}

TEST(LegacyTxRlp, RoundTripContractCreation) {
  LegacyTx tx;
  tx.nonce = 0;
  tx.gas_price = types::Uint256(1000000000ULL);
  tx.gas_limit = 100000;
  tx.to = std::nullopt;  // Contract creation
  tx.value = types::Uint256(0);
  tx.data = {0x60, 0x80, 0x60, 0x40, 0x52};  // Some bytecode
  tx.v = 28;
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(2);

  auto encoded = rlp_encode(tx);
  auto decoded = rlp_decode_legacy_tx(encoded);

  ASSERT_TRUE(decoded.ok());
  EXPECT_FALSE(decoded.value.to.has_value());
  EXPECT_EQ(decoded.value.data, tx.data);
}

TEST(LegacyTxRlp, TxHash) {
  LegacyTx tx;
  tx.nonce = 1;
  tx.gas_price = types::Uint256(1);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);
  tx.v = 27;
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  auto hash1 = tx_hash(tx);
  auto hash2 = tx_hash(tx);

  // Hash should be deterministic and cached
  EXPECT_EQ(hash1, hash2);
  EXPECT_TRUE(tx.cached_hash.has_value());
}

TEST(LegacyTxRlp, DecodeEmptyInput) {
  auto result = rlp_decode_legacy_tx({});
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, TxDecodeError::EmptyInput);
}

// =============================================================================
// Eip2930Tx RLP Tests
// =============================================================================

TEST(Eip2930TxRlp, RoundTrip) {
  Eip2930Tx tx;
  tx.chain_id = 1;
  tx.nonce = 5;
  tx.gas_price = types::Uint256(50000000000ULL);  // 50 gwei
  tx.gas_limit = 50000;
  tx.to = types::Address(types::Uint256(0xabcdef));
  tx.value = types::Uint256(100);
  tx.data = {0xaa, 0xbb, 0xcc};
  tx.access_list = {};
  tx.y_parity = 1;
  tx.r = types::Uint256(0x1111);
  tx.s = types::Uint256(0x2222);

  auto encoded = rlp_encode(tx);
  EXPECT_FALSE(encoded.empty());
  EXPECT_EQ(encoded[0], 0x01);  // Type prefix

  auto decoded = rlp_decode_eip2930_tx(std::span<const uint8_t>(encoded).subspan(1));
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value.chain_id, tx.chain_id);
  EXPECT_EQ(decoded.value.nonce, tx.nonce);
  EXPECT_EQ(decoded.value.gas_price, tx.gas_price);
  EXPECT_EQ(decoded.value.gas_limit, tx.gas_limit);
  EXPECT_EQ(decoded.value.value, tx.value);
  EXPECT_EQ(decoded.value.y_parity, tx.y_parity);
}

TEST(Eip2930TxRlp, RoundTripWithAccessList) {
  Eip2930Tx tx;
  tx.chain_id = 1;
  tx.nonce = 0;
  tx.gas_price = types::Uint256(1);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);

  AccessListEntry entry;
  entry.address = types::Address(types::Uint256(0x1234));
  entry.storage_keys.emplace_back(types::Uint256(0));
  entry.storage_keys.emplace_back(types::Uint256(1));
  tx.access_list.push_back(entry);

  tx.y_parity = 0;
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  auto encoded = rlp_encode(tx);
  auto decoded = rlp_decode_eip2930_tx(std::span<const uint8_t>(encoded).subspan(1));

  ASSERT_TRUE(decoded.ok());
  ASSERT_EQ(decoded.value.access_list.size(), 1);
  EXPECT_EQ(decoded.value.access_list[0].address, entry.address);
  ASSERT_EQ(decoded.value.access_list[0].storage_keys.size(), 2);
}

// =============================================================================
// Eip1559Tx RLP Tests
// =============================================================================

TEST(Eip1559TxRlp, RoundTrip) {
  Eip1559Tx tx;
  tx.chain_id = 1;
  tx.nonce = 10;
  tx.max_priority_fee_per_gas = types::Uint256(1000000000ULL);  // 1 gwei
  tx.max_fee_per_gas = types::Uint256(100000000000ULL);         // 100 gwei
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(0xdeadbeef));
  tx.value = types::Uint256(12345);
  tx.data = {0x01, 0x02};
  tx.access_list = {};
  tx.y_parity = 0;
  tx.r = types::Uint256(0xaaaa);
  tx.s = types::Uint256(0xbbbb);

  auto encoded = rlp_encode(tx);
  EXPECT_EQ(encoded[0], 0x02);  // Type prefix

  auto decoded = rlp_decode_eip1559_tx(std::span<const uint8_t>(encoded).subspan(1));
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value.chain_id, tx.chain_id);
  EXPECT_EQ(decoded.value.nonce, tx.nonce);
  EXPECT_EQ(decoded.value.max_priority_fee_per_gas, tx.max_priority_fee_per_gas);
  EXPECT_EQ(decoded.value.max_fee_per_gas, tx.max_fee_per_gas);
  EXPECT_EQ(decoded.value.gas_limit, tx.gas_limit);
  EXPECT_EQ(decoded.value.value, tx.value);
}

// =============================================================================
// Eip4844Tx RLP Tests
// =============================================================================

TEST(Eip4844TxRlp, RoundTrip) {
  Eip4844Tx tx;
  tx.chain_id = 1;
  tx.nonce = 100;
  tx.max_priority_fee_per_gas = types::Uint256(1000000000ULL);
  tx.max_fee_per_gas = types::Uint256(50000000000ULL);
  tx.gas_limit = 50000;
  tx.to = types::Address(types::Uint256(0xcafe));  // Required for blob tx
  tx.value = types::Uint256(0);
  tx.data = {};
  tx.access_list = {};
  tx.max_fee_per_blob_gas = types::Uint256(1000000000ULL);

  // Add a blob versioned hash
  std::array<uint8_t, 32> hash_bytes{};
  hash_bytes[0] = 0x01;  // Version 1
  hash_bytes[31] = 0xaa;
  tx.blob_versioned_hashes.emplace_back(hash_bytes);

  tx.y_parity = 1;
  tx.r = types::Uint256(0x1234);
  tx.s = types::Uint256(0x5678);

  auto encoded = rlp_encode(tx);
  EXPECT_EQ(encoded[0], 0x03);  // Type prefix

  auto decoded = rlp_decode_eip4844_tx(std::span<const uint8_t>(encoded).subspan(1));
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value.chain_id, tx.chain_id);
  EXPECT_EQ(decoded.value.max_fee_per_blob_gas, tx.max_fee_per_blob_gas);
  ASSERT_EQ(decoded.value.blob_versioned_hashes.size(), 1);
  EXPECT_EQ(decoded.value.blob_versioned_hashes[0][0], 0x01);
  EXPECT_EQ(decoded.value.blob_versioned_hashes[0][31], 0xaa);
}

// =============================================================================
// Eip7702Tx RLP Tests
// =============================================================================

TEST(Eip7702TxRlp, RoundTrip) {
  Eip7702Tx tx;
  tx.chain_id = 1;
  tx.nonce = 50;
  tx.max_priority_fee_per_gas = types::Uint256(1000000000ULL);
  tx.max_fee_per_gas = types::Uint256(50000000000ULL);
  tx.gas_limit = 100000;
  tx.to = types::Address(types::Uint256(0xbeef));  // Required
  tx.value = types::Uint256(0);
  tx.data = {};
  tx.access_list = {};

  // Add an authorization
  Authorization auth;
  auth.chain_id = 1;
  auth.address = types::Address(types::Uint256(0x9999));
  auth.nonce = 0;
  auth.y_parity = 0;
  auth.r = types::Uint256(0xaaaa);
  auth.s = types::Uint256(0xbbbb);
  tx.authorization_list.push_back(auth);

  tx.y_parity = 1;
  tx.r = types::Uint256(0x1111);
  tx.s = types::Uint256(0x2222);

  auto encoded = rlp_encode(tx);
  EXPECT_EQ(encoded[0], 0x04);  // Type prefix

  auto decoded = rlp_decode_eip7702_tx(std::span<const uint8_t>(encoded).subspan(1));
  ASSERT_TRUE(decoded.ok());
  EXPECT_EQ(decoded.value.chain_id, tx.chain_id);
  ASSERT_EQ(decoded.value.authorization_list.size(), 1);
  EXPECT_EQ(decoded.value.authorization_list[0].address, auth.address);
  EXPECT_EQ(decoded.value.authorization_list[0].nonce, auth.nonce);
}

// =============================================================================
// Transaction Wrapper Tests (Envelope Detection)
// =============================================================================

TEST(TransactionRlp, DecodeLegacyViaWrapper) {
  LegacyTx legacy;
  legacy.nonce = 1;
  legacy.gas_price = types::Uint256(1);
  legacy.gas_limit = 21000;
  legacy.to = types::Address(types::Uint256(1));
  legacy.value = types::Uint256(0);
  legacy.v = 27;
  legacy.r = types::Uint256(1);
  legacy.s = types::Uint256(1);

  auto encoded = rlp_encode(legacy);
  auto result = rlp_decode_transaction(encoded);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value.type(), TxType::Legacy);
  EXPECT_EQ(result.value.nonce(), legacy.nonce);
}

TEST(TransactionRlp, DecodeTypedViaWrapper) {
  Eip1559Tx eip1559;
  eip1559.chain_id = 1;
  eip1559.nonce = 5;
  eip1559.max_priority_fee_per_gas = types::Uint256(1);
  eip1559.max_fee_per_gas = types::Uint256(10);
  eip1559.gas_limit = 21000;
  eip1559.to = types::Address(types::Uint256(1));
  eip1559.value = types::Uint256(0);
  eip1559.y_parity = 0;
  eip1559.r = types::Uint256(1);
  eip1559.s = types::Uint256(1);

  auto encoded = rlp_encode(eip1559);
  auto result = rlp_decode_transaction(encoded);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value.type(), TxType::DynamicFee);
  EXPECT_EQ(result.value.nonce(), eip1559.nonce);
}

TEST(TransactionRlp, DecodeInvalidType) {
  // Type byte 0x05 is invalid
  std::vector<uint8_t> data = {0x05, 0xc0};  // Invalid type + empty list
  auto result = rlp_decode_transaction(data);

  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, TxDecodeError::InvalidType);
}

TEST(TransactionRlp, TxHashViaWrapper) {
  Eip2930Tx tx;
  tx.chain_id = 1;
  tx.nonce = 0;
  tx.gas_price = types::Uint256(1);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);
  tx.y_parity = 0;
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  const Transaction wrapper(tx);
  auto hash = tx_hash(wrapper);

  // Should be deterministic
  EXPECT_EQ(hash, tx_hash(wrapper));
}

// =============================================================================
// Receipt RLP Tests
// =============================================================================

TEST(ReceiptRlp, EncodeLegacySuccess) {
  Receipt receipt;
  receipt.tx_type = TxType::Legacy;
  receipt.status = 1;
  receipt.cumulative_gas_used = 21000;
  // bloom is zero-initialized
  // logs is empty

  auto encoded = rlp_encode(receipt);
  EXPECT_FALSE(encoded.empty());
  // Legacy receipt starts with RLP list prefix, not type byte
  EXPECT_GE(encoded[0], 0xc0);
}

TEST(ReceiptRlp, EncodeTypedSuccess) {
  Receipt receipt;
  receipt.tx_type = TxType::DynamicFee;  // Type 2
  receipt.status = 1;
  receipt.cumulative_gas_used = 50000;

  auto encoded = rlp_encode(receipt);
  EXPECT_FALSE(encoded.empty());
  // Typed receipt starts with type byte
  EXPECT_EQ(encoded[0], 0x02);
}

TEST(ReceiptRlp, EncodeWithLogs) {
  Receipt receipt;
  receipt.tx_type = TxType::Legacy;
  receipt.status = 1;
  receipt.cumulative_gas_used = 30000;

  evm::Log log;
  log.address = types::Address(types::Uint256(0x1234));
  log.topics.emplace_back(0xabcd);
  log.data = {0x01, 0x02, 0x03};
  receipt.logs.push_back(log);

  receipt.build_bloom();

  auto encoded = rlp_encode(receipt);
  EXPECT_FALSE(encoded.empty());
  // Verify bloom is non-zero (has entries)
  EXPECT_FALSE(receipt.bloom.is_empty());
}

// =============================================================================
// Signing Hash Tests (for sender recovery)
// =============================================================================

TEST(SigningHash, LegacyPreEip155) {
  LegacyTx tx;
  tx.nonce = 0;
  tx.gas_price = types::Uint256(1);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);
  tx.v = 27;  // Pre-EIP-155
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  auto hash = signing_hash(tx);
  EXPECT_FALSE(hash.is_zero());
}

TEST(SigningHash, LegacyEip155) {
  LegacyTx tx;
  tx.nonce = 0;
  tx.gas_price = types::Uint256(1);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);
  tx.v = 37;  // EIP-155 with chain_id=1
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  auto hash = signing_hash(tx);
  EXPECT_FALSE(hash.is_zero());
}

TEST(SigningHash, Eip1559) {
  Eip1559Tx tx;
  tx.chain_id = 1;
  tx.nonce = 0;
  tx.max_priority_fee_per_gas = types::Uint256(1);
  tx.max_fee_per_gas = types::Uint256(10);
  tx.gas_limit = 21000;
  tx.to = types::Address(types::Uint256(1));
  tx.value = types::Uint256(0);
  tx.y_parity = 0;
  tx.r = types::Uint256(1);
  tx.s = types::Uint256(1);

  auto hash = signing_hash(tx);
  EXPECT_FALSE(hash.is_zero());
}

// =============================================================================
// Access List Length Tests
// =============================================================================

TEST(AccessListRlp, EncodedLength) {
  AccessList list;

  // Empty list
  EXPECT_EQ(rlp_encoded_length(list), 1);  // 0xc0

  // Single entry with no storage keys
  AccessListEntry entry;
  entry.address = types::Address(types::Uint256(1));
  list.push_back(entry);

  const size_t len = rlp_encoded_length(list);
  EXPECT_GT(len, 1);

  // Add storage key
  list[0].storage_keys.emplace_back(types::Uint256(0));
  EXPECT_GT(rlp_encoded_length(list), len);
}

TEST(AuthorizationListRlp, EncodedLength) {
  AuthorizationList list;

  // Empty list
  EXPECT_EQ(rlp_encoded_length(list), 1);  // 0xc0

  // Single authorization
  Authorization auth;
  auth.chain_id = 1;
  auth.address = types::Address(types::Uint256(1));
  auth.nonce = 0;
  auth.y_parity = 0;
  auth.r = types::Uint256(1);
  auth.s = types::Uint256(1);
  list.push_back(auth);

  EXPECT_GT(rlp_encoded_length(list), 1);
}

}  // namespace
}  // namespace div0::ethereum
