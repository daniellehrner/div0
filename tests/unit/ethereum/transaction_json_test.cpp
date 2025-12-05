#include <gtest/gtest.h>

#include "div0/ethereum/transaction/json.h"

namespace div0::ethereum {
namespace {

using types::Address;
using types::Hash;
using types::Uint256;

// =============================================================================
// Hex Encoding Tests
// =============================================================================

TEST(HexEncoding, Uint64Zero) { EXPECT_EQ(hex::encode_uint64(0), "0x0"); }

TEST(HexEncoding, Uint64Simple) {
  EXPECT_EQ(hex::encode_uint64(1), "0x1");
  EXPECT_EQ(hex::encode_uint64(255), "0xff");
  EXPECT_EQ(hex::encode_uint64(256), "0x100");
}

TEST(HexEncoding, Uint64Max) {
  EXPECT_EQ(hex::encode_uint64(0xFFFFFFFFFFFFFFFF), "0xffffffffffffffff");
}

TEST(HexEncoding, Uint256Zero) { EXPECT_EQ(hex::encode_uint256(Uint256::zero()), "0x0"); }

TEST(HexEncoding, Uint256Simple) {
  EXPECT_EQ(hex::encode_uint256(Uint256(1)), "0x1");
  EXPECT_EQ(hex::encode_uint256(Uint256(0x1234)), "0x1234");
}

TEST(HexEncoding, Uint256LargeValue) {
  // Value spanning multiple limbs
  // Uint256 limbs are little-endian: limb[0] is LSB
  // So Uint256(0xDEADBEEF, 0x12345678, 0, 0) means:
  //   limb[0] = 0xDEADBEEF (bits 0-63)
  //   limb[1] = 0x12345678 (bits 64-127)
  // In big-endian hex: 0x1234567800000000deadbeef
  const Uint256 value(0xDEADBEEF, 0x12345678, 0, 0);
  EXPECT_EQ(hex::encode_uint256(value), "0x1234567800000000deadbeef");
}

TEST(HexEncoding, Address) {
  std::array<uint8_t, 20> bytes{};
  bytes[0] = 0xDE;
  bytes[1] = 0xAD;
  bytes[19] = 0xEF;
  const Address addr(bytes);
  EXPECT_EQ(hex::encode_address(addr), "0xdead0000000000000000000000000000000000ef");
}

TEST(HexEncoding, Hash) {
  std::array<uint8_t, 32> bytes{};
  bytes[0] = 0xAB;
  bytes[31] = 0xCD;
  const Hash hash(bytes);
  EXPECT_EQ(hex::encode_hash(hash),
            "0xab000000000000000000000000000000000000000000000000000000000000cd");
}

TEST(HexEncoding, StorageSlot) {
  const StorageSlot slot(Uint256(0x42));
  auto encoded = hex::encode_storage_slot(slot);
  EXPECT_EQ(encoded, "0x0000000000000000000000000000000000000000000000000000000000000042");
}

TEST(HexEncoding, Bytes) {
  std::vector<uint8_t> data{0xDE, 0xAD, 0xBE, 0xEF};
  EXPECT_EQ(hex::encode_bytes(data), "0xdeadbeef");
}

TEST(HexEncoding, EmptyBytes) { EXPECT_EQ(hex::encode_bytes({}), "0x"); }

// =============================================================================
// Hex Decoding Tests
// =============================================================================

TEST(HexDecoding, Uint64) {
  const auto result = hex::decode_uint64("0x42");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*result, 0x42ULL);
}

TEST(HexDecoding, Uint64Zero) {
  const auto result = hex::decode_uint64("0x0");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*result, 0ULL);
}

TEST(HexDecoding, Uint64Max) {
  const auto result = hex::decode_uint64("0xffffffffffffffff");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*result, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(HexDecoding, Uint64Invalid) {
  EXPECT_FALSE(hex::decode_uint64("42").has_value());                   // Missing 0x
  EXPECT_FALSE(hex::decode_uint64("0xGG").has_value());                 // Invalid chars
  EXPECT_FALSE(hex::decode_uint64("0x").has_value());                   // Empty
  EXPECT_FALSE(hex::decode_uint64("0x12345678901234567").has_value());  // Too long
}

TEST(HexDecoding, Uint256) {
  const auto result = hex::decode_uint256("0x1234");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*result, Uint256(0x1234));
}

TEST(HexDecoding, Uint256Large) {
  // Decoding 0x1234567800000000deadbeef
  // In little-endian limbs: limb[0] = 0xDEADBEEF, limb[1] = 0x12345678
  const auto result = hex::decode_uint256("0x1234567800000000deadbeef");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(*result, Uint256(0xDEADBEEF, 0x12345678, 0, 0));
}

TEST(HexDecoding, Address) {
  const auto result = hex::decode_address("0xdead000000000000000000000000000000000001");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto& addr = *result;
  EXPECT_EQ(addr[0], 0xDE);
  EXPECT_EQ(addr[1], 0xAD);
  EXPECT_EQ(addr[19], 0x01);
}

TEST(HexDecoding, AddressInvalid) {
  EXPECT_FALSE(hex::decode_address("0x1234").has_value());  // Too short
  EXPECT_FALSE(hex::decode_address("0xdead0000000000000000000000000000000000000001")
                   .has_value());  // Too long
}

TEST(HexDecoding, Hash) {
  const auto result =
      hex::decode_hash("0xab000000000000000000000000000000000000000000000000000000000000cd");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto& hash = *result;
  EXPECT_EQ(hash[0], 0xAB);
  EXPECT_EQ(hash[31], 0xCD);
}

TEST(HexDecoding, StorageSlot) {
  const auto result = hex::decode_storage_slot("0x42");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(result->get(), Uint256(0x42));
}

TEST(HexDecoding, Bytes) {
  const auto result = hex::decode_bytes("0xdeadbeef");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto& bytes = *result;
  EXPECT_EQ(bytes.size(), 4UL);
  EXPECT_EQ(bytes[0], 0xDE);
  EXPECT_EQ(bytes[3], 0xEF);
}

TEST(HexDecoding, EmptyBytes) {
  const auto result = hex::decode_bytes("0x");
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto& bytes = *result;
  EXPECT_TRUE(bytes.empty());
}

// =============================================================================
// Legacy Transaction JSON Tests
// =============================================================================

TEST(LegacyTxJson, EncodeBasic) {
  LegacyTx tx;
  tx.nonce = 1;
  tx.gas_price = Uint256(20000000000ULL);  // 20 gwei
  tx.gas_limit = 21000;
  tx.to = Address(Uint256(0x1234));
  tx.value = Uint256(1000000000000000000ULL);  // 1 ETH
  tx.v = 28;
  tx.r = Uint256(0xABCD);
  tx.s = Uint256(0xEF01);

  const std::string json = json_encode(tx);

  EXPECT_NE(json.find("\"type\":\"0x0\""), std::string::npos);
  EXPECT_NE(json.find("\"nonce\":\"0x1\""), std::string::npos);
  EXPECT_NE(json.find("\"gas\":\"0x5208\""), std::string::npos);  // 21000 = 0x5208
  EXPECT_NE(json.find("\"v\":\"0x1c\""), std::string::npos);      // 28 = 0x1c
}

TEST(LegacyTxJson, EncodeContractCreation) {
  LegacyTx tx;
  tx.nonce = 0;
  tx.gas_price = Uint256(1);
  tx.gas_limit = 100000;
  tx.to = std::nullopt;  // Contract creation
  tx.value = Uint256::zero();
  tx.data = {0x60, 0x80};
  tx.v = 27;
  tx.r = Uint256(1);
  tx.s = Uint256(2);

  const std::string json = json_encode(tx);

  EXPECT_NE(json.find("\"to\":null"), std::string::npos);
  EXPECT_NE(json.find("\"input\":\"0x6080\""), std::string::npos);
}

TEST(LegacyTxJson, DecodeBasic) {
  const char* json = R"({
    "type": "0x0",
    "nonce": "0x1",
    "gasPrice": "0x4a817c800",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0xde0b6b3a7640000",
    "input": "0x",
    "v": "0x1c",
    "r": "0xabcd",
    "s": "0xef01"
  })";

  auto result = json_decode_legacy_tx(json);
  ASSERT_TRUE(result.ok()) << result.error_detail;

  EXPECT_EQ(result.value.nonce, 1UL);
  EXPECT_EQ(result.value.gas_price, Uint256(20000000000ULL));
  EXPECT_EQ(result.value.gas_limit, 21000UL);
  EXPECT_TRUE(result.value.to.has_value());
  EXPECT_EQ(result.value.value, Uint256(1000000000000000000ULL));
  EXPECT_EQ(result.value.v, 28UL);
  EXPECT_EQ(result.value.r, Uint256(0xABCD));
  EXPECT_EQ(result.value.s, Uint256(0xEF01));
}

TEST(LegacyTxJson, RoundTrip) {
  LegacyTx original;
  original.nonce = 42;
  original.gas_price = Uint256(100);
  original.gas_limit = 50000;
  original.to = Address(Uint256(0xDEADBEEF));
  original.value = Uint256(12345);
  original.data = {0x01, 0x02, 0x03};
  original.v = 37;
  original.r = Uint256(0x111);
  original.s = Uint256(0x222);

  const std::string json = json_encode(original);
  auto result = json_decode_legacy_tx(json);

  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.nonce, original.nonce);
  EXPECT_EQ(result.value.gas_price, original.gas_price);
  EXPECT_EQ(result.value.gas_limit, original.gas_limit);
  EXPECT_EQ(result.value.value, original.value);
  EXPECT_EQ(result.value.data, original.data);
  EXPECT_EQ(result.value.v, original.v);
  EXPECT_EQ(result.value.r, original.r);
  EXPECT_EQ(result.value.s, original.s);
}

// =============================================================================
// EIP-1559 Transaction JSON Tests
// =============================================================================

TEST(Eip1559TxJson, EncodeBasic) {
  Eip1559Tx tx;
  tx.chain_id = 1;
  tx.nonce = 5;
  tx.max_priority_fee_per_gas = Uint256(2000000000ULL);
  tx.max_fee_per_gas = Uint256(100000000000ULL);
  tx.gas_limit = 21000;
  tx.to = Address(Uint256(0x1234));
  tx.value = Uint256(1000);
  tx.y_parity = 1;
  tx.r = Uint256(0xABC);
  tx.s = Uint256(0xDEF);

  const std::string json = json_encode(tx);

  EXPECT_NE(json.find("\"type\":\"0x2\""), std::string::npos);
  EXPECT_NE(json.find("\"chainId\":\"0x1\""), std::string::npos);
  EXPECT_NE(json.find("\"maxPriorityFeePerGas\":"), std::string::npos);
  EXPECT_NE(json.find("\"maxFeePerGas\":"), std::string::npos);
  EXPECT_NE(json.find("\"yParity\":\"0x1\""), std::string::npos);
}

TEST(Eip1559TxJson, RoundTrip) {
  Eip1559Tx original;
  original.chain_id = 1;
  original.nonce = 10;
  original.max_priority_fee_per_gas = Uint256(1000000000ULL);
  original.max_fee_per_gas = Uint256(50000000000ULL);
  original.gas_limit = 100000;
  original.to = Address(Uint256(0xCAFE));
  original.value = Uint256(5000);
  original.data = {0xAA, 0xBB};
  original.y_parity = 0;
  original.r = Uint256(0x123);
  original.s = Uint256(0x456);

  const std::string json = json_encode(original);
  auto result = json_decode_eip1559_tx(json);

  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.chain_id, original.chain_id);
  EXPECT_EQ(result.value.nonce, original.nonce);
  EXPECT_EQ(result.value.max_priority_fee_per_gas, original.max_priority_fee_per_gas);
  EXPECT_EQ(result.value.max_fee_per_gas, original.max_fee_per_gas);
  EXPECT_EQ(result.value.gas_limit, original.gas_limit);
  EXPECT_EQ(result.value.value, original.value);
  EXPECT_EQ(result.value.y_parity, original.y_parity);
}

// =============================================================================
// Transaction Type Detection Tests
// =============================================================================

TEST(TransactionJson, DetectLegacy) {
  const char* json = R"({
    "type": "0x0",
    "nonce": "0x0",
    "gasPrice": "0x1",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "v": "0x1b",
    "r": "0x1",
    "s": "0x2"
  })";

  auto result = json_decode_transaction(json);
  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.type(), TxType::Legacy);
}

TEST(TransactionJson, DetectEip1559) {
  const char* json = R"({
    "type": "0x2",
    "chainId": "0x1",
    "nonce": "0x0",
    "maxPriorityFeePerGas": "0x1",
    "maxFeePerGas": "0x2",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "accessList": [],
    "yParity": "0x0",
    "r": "0x1",
    "s": "0x2"
  })";

  auto result = json_decode_transaction(json);
  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.type(), TxType::DynamicFee);
}

TEST(TransactionJson, MissingTypeDefaultsToLegacy) {
  const char* json = R"({
    "nonce": "0x0",
    "gasPrice": "0x1",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "v": "0x1b",
    "r": "0x1",
    "s": "0x2"
  })";

  auto result = json_decode_transaction(json);
  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.type(), TxType::Legacy);
}

// =============================================================================
// Access List Tests
// =============================================================================

TEST(Eip2930TxJson, WithAccessList) {
  Eip2930Tx tx;
  tx.chain_id = 1;
  tx.nonce = 0;
  tx.gas_price = Uint256(1);
  tx.gas_limit = 21000;
  tx.to = Address(Uint256(0x1234));
  tx.value = Uint256::zero();

  AccessListEntry entry;
  entry.address = Address(Uint256(0xABCD));
  entry.storage_keys.emplace_back(Uint256(0x1));
  entry.storage_keys.emplace_back(Uint256(0x2));
  tx.access_list.push_back(entry);

  tx.y_parity = 0;
  tx.r = Uint256(1);
  tx.s = Uint256(2);

  const std::string json = json_encode(tx);

  EXPECT_NE(json.find("\"accessList\":["), std::string::npos);
  EXPECT_NE(json.find("\"storageKeys\":["), std::string::npos);
}

TEST(Eip2930TxJson, RoundTripWithAccessList) {
  Eip2930Tx original;
  original.chain_id = 1;
  original.nonce = 5;
  original.gas_price = Uint256(100);
  original.gas_limit = 50000;
  original.to = Address(Uint256(0xCAFE));
  original.value = Uint256(1000);

  AccessListEntry entry1;
  entry1.address = Address(Uint256(0x1111));
  entry1.storage_keys.emplace_back(Uint256(0x10));
  entry1.storage_keys.emplace_back(Uint256(0x20));
  original.access_list.push_back(entry1);

  AccessListEntry entry2;
  entry2.address = Address(Uint256(0x2222));
  original.access_list.push_back(entry2);

  original.y_parity = 1;
  original.r = Uint256(0x123);
  original.s = Uint256(0x456);

  const std::string json = json_encode(original);
  auto result = json_decode_eip2930_tx(json);

  ASSERT_TRUE(result.ok()) << result.error_detail;
  EXPECT_EQ(result.value.access_list.size(), 2UL);
  EXPECT_EQ(result.value.access_list[0].storage_keys.size(), 2UL);
  EXPECT_EQ(result.value.access_list[1].storage_keys.size(), 0UL);
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST(TransactionJson, InvalidJson) {
  auto result = json_decode_transaction("not json");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, JsonDecodeError::ParseError);
}

TEST(TransactionJson, MissingRequiredField) {
  const char* json = R"({
    "type": "0x0",
    "gasPrice": "0x1",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "v": "0x1b",
    "r": "0x1",
    "s": "0x2"
  })";  // Missing "nonce"

  auto result = json_decode_legacy_tx(json);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, JsonDecodeError::MissingField);
  EXPECT_EQ(result.error_detail, "nonce");
}

TEST(TransactionJson, InvalidHex) {
  const char* json = R"({
    "type": "0x0",
    "nonce": "not_hex",
    "gasPrice": "0x1",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "v": "0x1b",
    "r": "0x1",
    "s": "0x2"
  })";

  auto result = json_decode_legacy_tx(json);
  EXPECT_FALSE(result.ok());
}

TEST(TransactionJson, InvalidType) {
  const char* json = R"({
    "type": "0xff",
    "nonce": "0x0",
    "gasPrice": "0x1",
    "gas": "0x5208",
    "to": "0x0000000000000000000000000000000000001234",
    "value": "0x0",
    "input": "0x",
    "v": "0x1b",
    "r": "0x1",
    "s": "0x2"
  })";

  auto result = json_decode_transaction(json);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, JsonDecodeError::InvalidType);
}

}  // namespace
}  // namespace div0::ethereum
