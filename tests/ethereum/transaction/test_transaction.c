#include "test_transaction.h"

#include "div0/ethereum/transaction/rlp.h"
#include "div0/ethereum/transaction/signer.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/util/hex.h"

#include "unity.h"

#include <string.h>

// Helper macro to calculate hex string byte length (handles 0x prefix)
#define HEX_LEN(s) ((strlen(s) - ((s)[0] == '0' && ((s)[1] == 'x' || (s)[1] == 'X') ? 2 : 0)) / 2)

extern div0_arena_t test_arena;

// ============================================================================
// Transaction Type Tests
// ============================================================================

void test_transaction_type_enum(void) {
  TEST_ASSERT_EQUAL_INT(0, TX_TYPE_LEGACY);
  TEST_ASSERT_EQUAL_INT(1, TX_TYPE_EIP2930);
  TEST_ASSERT_EQUAL_INT(2, TX_TYPE_EIP1559);
  TEST_ASSERT_EQUAL_INT(3, TX_TYPE_EIP4844);
  TEST_ASSERT_EQUAL_INT(4, TX_TYPE_EIP7702);
}

void test_transaction_init_default(void) {
  transaction_t tx;
  transaction_init(&tx);

  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, tx.type);
  TEST_ASSERT_EQUAL_UINT64(0, tx.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(0, tx.legacy.gas_limit);
  TEST_ASSERT_TRUE(uint256_is_zero(tx.legacy.gas_price));
  TEST_ASSERT_TRUE(uint256_is_zero(tx.legacy.value));
  TEST_ASSERT_NULL(tx.legacy.to);
}

// ============================================================================
// Legacy Transaction Decoding Tests
// ============================================================================

void test_legacy_tx_decode_basic(void) {
  // Simple legacy transaction (EIP-155, chain_id=1):
  // RLP([nonce=0, gasPrice=0x3b9aca00 (1 gwei), gasLimit=21000, to=0x..., value=1 ether, data=[],
  // v=37, r, s]) This is a minimal valid transaction structure
  uint8_t rlp_data[] = {
      0xf8, 0x65,                                     // list header (total 101 bytes payload)
      0x80,                                           // nonce = 0
      0x84, 0x3b, 0x9a, 0xca, 0x00,                   // gasPrice = 1000000000 (1 gwei)
      0x82, 0x52, 0x08,                               // gasLimit = 21000
      0x94,                                           // to address (20 bytes)
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // address bytes
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x88, 0x0d, 0xe0, 0xb6, 0xb3, 0xa7, 0x64, 0x00, 0x00, // value = 1 ether
      0x80,                                                       // data = []
      0x25, // v = 37 (EIP-155, chain_id=1, recovery_id=0)
      0xa0, // r (32 bytes)
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
      0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0xa0, // s (32 bytes)
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  };

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, sizeof(rlp_data), &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, tx.type);
  TEST_ASSERT_EQUAL_UINT64(0, tx.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(21000, tx.legacy.gas_limit);
  TEST_ASSERT_NOT_NULL(tx.legacy.to);
  TEST_ASSERT_EQUAL_UINT64(37, tx.legacy.v);
}

void test_legacy_tx_decode_contract_creation(void) {
  // Contract creation transaction (to = empty)
  uint8_t rlp_data[] = {
      0xf8, 0x4d,                         // list header
      0x80,                               // nonce = 0
      0x84, 0x3b, 0x9a, 0xca, 0x00,       // gasPrice = 1 gwei
      0x83, 0x0f, 0x42, 0x40,             // gasLimit = 1000000
      0x80,                               // to = empty (contract creation)
      0x80,                               // value = 0
      0x85, 0x60, 0x80, 0x60, 0x40, 0x52, // data = some bytecode
      0x1b,                               // v = 27 (pre-EIP-155)
      0xa0,                               // r
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
      0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0xa0, // s
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  };

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, sizeof(rlp_data), &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_TRUE(transaction_is_create(&tx));
  TEST_ASSERT_NULL(tx.legacy.to);
}

void test_legacy_tx_chain_id_eip155(void) {
  legacy_tx_t tx;
  legacy_tx_init(&tx);

  // v = 37 = chain_id * 2 + 35 => chain_id = 1
  tx.v = 37;
  uint64_t chain_id = 0;
  TEST_ASSERT_TRUE(legacy_tx_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_UINT64(1, chain_id);
  TEST_ASSERT_EQUAL_INT(0, legacy_tx_recovery_id(&tx));

  // v = 38 = chain_id * 2 + 36 => chain_id = 1, recovery_id = 1
  tx.v = 38;
  TEST_ASSERT_TRUE(legacy_tx_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_UINT64(1, chain_id);
  TEST_ASSERT_EQUAL_INT(1, legacy_tx_recovery_id(&tx));

  // v = 2709 = 1337 * 2 + 35 => chain_id = 1337
  tx.v = 2709;
  TEST_ASSERT_TRUE(legacy_tx_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_UINT64(1337, chain_id);
}

void test_legacy_tx_chain_id_pre_eip155(void) {
  legacy_tx_t tx;
  legacy_tx_init(&tx);

  // Pre-EIP-155: v = 27 or 28
  tx.v = 27;
  uint64_t chain_id = 0;
  TEST_ASSERT_FALSE(legacy_tx_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_INT(0, legacy_tx_recovery_id(&tx));

  tx.v = 28;
  TEST_ASSERT_FALSE(legacy_tx_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_INT(1, legacy_tx_recovery_id(&tx));
}

// ============================================================================
// EIP-1559 Transaction Decoding Tests
// ============================================================================

void test_eip1559_tx_decode_basic(void) {
  // EIP-1559 transaction: 0x02 || RLP([...])
  uint8_t rlp_data[] = {
      0x02,                         // type byte
      0xf8, 0x4f,                   // list header
      0x01,                         // chain_id = 1
      0x80,                         // nonce = 0
      0x84, 0x3b, 0x9a, 0xca, 0x00, // maxPriorityFeePerGas = 1 gwei
      0x84, 0x77, 0x35, 0x94, 0x00, // maxFeePerGas = 2 gwei
      0x82, 0x52, 0x08,             // gasLimit = 21000
      0x94,                         // to address
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x80, // value = 0
      0x80, // data = []
      0xc0, // access_list = []
      0x80, // y_parity = 0
      0xa0, // r
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
      0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0xa0, // s
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  };

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, sizeof(rlp_data), &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_EIP1559, tx.type);
  TEST_ASSERT_EQUAL_UINT64(1, tx.eip1559.chain_id);
  TEST_ASSERT_EQUAL_UINT64(0, tx.eip1559.nonce);
  TEST_ASSERT_EQUAL_UINT64(21000, tx.eip1559.gas_limit);
  TEST_ASSERT_EQUAL_UINT8(0, tx.eip1559.y_parity);
}

void test_eip1559_tx_effective_gas_price(void) {
  eip1559_tx_t tx;
  eip1559_tx_init(&tx);

  // max_priority_fee = 1 gwei, max_fee = 3 gwei, base_fee = 1 gwei
  // effective = min(3, 1 + 1) = 2 gwei
  tx.max_priority_fee_per_gas = uint256_from_u64(1000000000);
  tx.max_fee_per_gas = uint256_from_u64(3000000000);

  uint256_t base_fee = uint256_from_u64(1000000000);
  uint256_t effective = eip1559_tx_effective_gas_price(&tx, base_fee);

  TEST_ASSERT_TRUE(uint256_eq(effective, uint256_from_u64(2000000000)));

  // max_priority_fee = 2 gwei, max_fee = 2 gwei, base_fee = 1 gwei
  // effective = min(max_fee, base_fee + priority) = min(2, 1 + 2) = 2 gwei
  // (capped by max_fee since base + priority = 3 > max_fee = 2)
  tx.max_priority_fee_per_gas = uint256_from_u64(2000000000);
  tx.max_fee_per_gas = uint256_from_u64(2000000000);

  effective = eip1559_tx_effective_gas_price(&tx, base_fee);
  TEST_ASSERT_TRUE(uint256_eq(effective, uint256_from_u64(2000000000)));
}

// ============================================================================
// Transaction Accessors Tests
// ============================================================================

void test_transaction_accessors(void) {
  transaction_t tx;
  tx.type = TX_TYPE_EIP1559;
  eip1559_tx_init(&tx.eip1559);

  tx.eip1559.nonce = 42;
  tx.eip1559.gas_limit = 100000;
  tx.eip1559.chain_id = 1;
  tx.eip1559.value = uint256_from_u64(1000);

  TEST_ASSERT_EQUAL_UINT64(42, transaction_nonce(&tx));
  TEST_ASSERT_EQUAL_UINT64(100000, transaction_gas_limit(&tx));
  TEST_ASSERT_TRUE(uint256_eq(transaction_value(&tx), uint256_from_u64(1000)));

  uint64_t chain_id = 0;
  TEST_ASSERT_TRUE(transaction_chain_id(&tx, &chain_id));
  TEST_ASSERT_EQUAL_UINT64(1, chain_id);
}

// ============================================================================
// Signing Hash Tests
// ============================================================================

void test_legacy_tx_signing_hash(void) {
  legacy_tx_t tx;
  legacy_tx_init(&tx);

  // Set up a simple transaction
  tx.nonce = 9;
  tx.gas_price = uint256_from_u64(20000000000ULL);
  tx.gas_limit = 21000;
  tx.value = uint256_from_u64(1000000000000000000ULL);
  tx.v = 27; // Pre-EIP-155

  // Compute signing hash
  hash_t hash = legacy_tx_signing_hash(&tx, &test_arena);

  // Just verify it's not zero
  TEST_ASSERT_FALSE(hash_is_zero(&hash));
}

void test_eip1559_tx_signing_hash(void) {
  eip1559_tx_t tx;
  eip1559_tx_init(&tx);

  tx.chain_id = 1;
  tx.nonce = 0;
  tx.max_priority_fee_per_gas = uint256_from_u64(1000000000);
  tx.max_fee_per_gas = uint256_from_u64(2000000000);
  tx.gas_limit = 21000;
  tx.value = uint256_zero();

  hash_t hash = eip1559_tx_signing_hash(&tx, &test_arena);

  TEST_ASSERT_FALSE(hash_is_zero(&hash));
}

// ============================================================================
// Sender Recovery Tests
// ============================================================================

void test_transaction_recover_sender_legacy(void) {
  // This test verifies that sender recovery works for a known transaction.
  // We use a real Ethereum mainnet transaction for validation.

  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  legacy_tx_t tx;
  legacy_tx_init(&tx);

  // Known transaction values (placeholder - would need real test vector)
  tx.nonce = 0;
  tx.gas_price = uint256_from_u64(1000000000);
  tx.gas_limit = 21000;
  tx.v = 27; // Pre-EIP-155
  // r and s would need to be set from a known transaction

  // For now, just verify the function doesn't crash with zero signature
  transaction_t unified_tx;
  unified_tx.type = TX_TYPE_LEGACY;
  unified_tx.legacy = tx;

  ecrecover_result_t result = transaction_recover_sender(ctx, &unified_tx, &test_arena);
  // With zero signature, recovery should fail
  TEST_ASSERT_FALSE(result.success);

  secp256k1_ctx_destroy(ctx);
}

// ============================================================================
// Real Test Vectors from Ethereum Tests
// ============================================================================

void test_real_vector_legacy_pre_eip155(void) {
  // From ethereum/tests SenderTest.json
  // Legacy transaction with v=27 (pre-EIP-155)
  // Expected sender: 0x963f4a0d8a11b758de8d5b99ab4ac898d6438ea6
  const char *hex =
      "f85f800182520894095e7baea6a6c7c4c2dfeb977efac326af552d870a801ba048b55bfa915ac795c"
      "431978d8a6a992b628d557da5ff759b307d495a36649353a01fffd310ac743f371de3b9f7f9cb56c"
      "0b28ad43601b4ab949f53faa07bd2c804";

  uint8_t rlp_data[256];
  size_t len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, rlp_data, len));

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, len, &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, tx.type);
  TEST_ASSERT_EQUAL_UINT64(0, tx.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(21000, tx.legacy.gas_limit);
  TEST_ASSERT_EQUAL_UINT64(27, tx.legacy.v); // Pre-EIP-155

  // Verify sender recovery
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  ecrecover_result_t recover_result = transaction_recover_sender(ctx, &tx, &test_arena);
  TEST_ASSERT_TRUE(recover_result.success);

  // Expected sender: 0x963f4a0d8a11b758de8d5b99ab4ac898d6438ea6
  uint8_t expected_sender[20];
  TEST_ASSERT_TRUE(hex_decode("963f4a0d8a11b758de8d5b99ab4ac898d6438ea6", expected_sender, 20));
  TEST_ASSERT_EQUAL_MEMORY(expected_sender, recover_result.address.bytes, 20);

  secp256k1_ctx_destroy(ctx);
}

void test_real_vector_legacy_eip155(void) {
  // From ethereum/tests Vitalik_1.json
  // Legacy transaction with EIP-155 (chain_id=1, v=37)
  // Expected sender: 0xf0f6f18bca1b28cd68e4357452947e021241e9ce
  const char *hex =
      "f864808504a817c800825208943535353535353535353535353535353535353535808025a0044852b"
      "2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116da0044852b2a670ade5407e7"
      "8fb2863c51de9fcb96542a07186fe3aeda6bb8a116d";

  uint8_t rlp_data[256];
  size_t len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, rlp_data, len));

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, len, &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, tx.type);
  TEST_ASSERT_EQUAL_UINT64(0, tx.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(21000, tx.legacy.gas_limit);
  TEST_ASSERT_EQUAL_UINT64(37, tx.legacy.v); // EIP-155, chain_id=1

  // Verify chain_id extraction
  uint64_t chain_id = 0;
  TEST_ASSERT_TRUE(legacy_tx_chain_id(&tx.legacy, &chain_id));
  TEST_ASSERT_EQUAL_UINT64(1, chain_id);

  // Verify sender recovery
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  ecrecover_result_t recover_result = transaction_recover_sender(ctx, &tx, &test_arena);
  TEST_ASSERT_TRUE(recover_result.success);

  // Expected sender: 0xf0f6f18bca1b28cd68e4357452947e021241e9ce
  uint8_t expected_sender[20];
  TEST_ASSERT_TRUE(hex_decode("f0f6f18bca1b28cd68e4357452947e021241e9ce", expected_sender, 20));
  TEST_ASSERT_EQUAL_MEMORY(expected_sender, recover_result.address.bytes, 20);

  secp256k1_ctx_destroy(ctx);
}

void test_real_vector_legacy_vitalik_2(void) {
  // From ethereum/tests Vitalik_2.json
  // Expected sender: 0x23ef145a395ea3fa3deb533b8a9e1b4c6c25d112
  const char *hex =
      "f864018504a817c80182a410943535353535353535353535353535353535353535018025a0489efda"
      "a54c0f20c7adf612882df0950f5a951637e0307cdcb4c672f298b8bcaa0489efdaa54c0f20c7adf6"
      "12882df0950f5a951637e0307cdcb4c672f298b8bc6";

  uint8_t rlp_data[256];
  size_t len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, rlp_data, len));

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, len, &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, tx.type);
  TEST_ASSERT_EQUAL_UINT64(1, tx.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(42000, tx.legacy.gas_limit);
  TEST_ASSERT_EQUAL_UINT64(37, tx.legacy.v);

  // Verify sender recovery
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  ecrecover_result_t recover_result = transaction_recover_sender(ctx, &tx, &test_arena);
  TEST_ASSERT_TRUE(recover_result.success);

  // Expected sender: 0x23ef145a395ea3fa3deb533b8a9e1b4c6c25d112
  uint8_t expected_sender[20];
  TEST_ASSERT_TRUE(hex_decode("23ef145a395ea3fa3deb533b8a9e1b4c6c25d112", expected_sender, 20));
  TEST_ASSERT_EQUAL_MEMORY(expected_sender, recover_result.address.bytes, 20);

  secp256k1_ctx_destroy(ctx);
}

void test_real_vector_eip2930(void) {
  // From ethereum/tests accessListStorage32Bytes.json
  // EIP-2930 transaction with access list
  // Expected sender: 0xebe76799923fd62804659fb00b4f0f1a94c0eb1e
  const char *hex =
      "01f89a018001826a4094095e7baea6a6c7c4c2dfeb977efac326af552d878080f838f794a95e7bae"
      "a6a6c7c4c2dfeb977efac326af552d87e1a0fffffffffffffffffffffffffffffffffffffffffffff"
      "fffffffffffffffffff80a05cbd172231fc0735e0fb994dd5b1a4939170a260b36f0427a8a80866b0"
      "63b948a07c230f7f578dd61785c93361b9871c0706ebfa6d06e3f4491dc9558c5202ed36";

  uint8_t rlp_data[256];
  size_t len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, rlp_data, len));

  transaction_t tx;
  tx_decode_result_t result = transaction_decode(rlp_data, len, &tx, &test_arena);

  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);
  TEST_ASSERT_EQUAL_INT(TX_TYPE_EIP2930, tx.type);
  TEST_ASSERT_EQUAL_UINT64(1, tx.eip2930.chain_id);
  TEST_ASSERT_EQUAL_UINT64(0, tx.eip2930.nonce);
  TEST_ASSERT_EQUAL_UINT64(27200, tx.eip2930.gas_limit); // 0x6a40

  // Verify access list was decoded
  TEST_ASSERT_EQUAL_UINT64(1, tx.eip2930.access_list.count);
  TEST_ASSERT_EQUAL_UINT64(1, tx.eip2930.access_list.entries[0].storage_keys_count);

  // Verify sender recovery
  secp256k1_ctx_t *ctx = secp256k1_ctx_create();
  TEST_ASSERT_NOT_NULL(ctx);

  ecrecover_result_t recover_result = transaction_recover_sender(ctx, &tx, &test_arena);
  TEST_ASSERT_TRUE(recover_result.success);

  // Expected sender: 0xebe76799923fd62804659fb00b4f0f1a94c0eb1e
  uint8_t expected_sender[20];
  TEST_ASSERT_TRUE(hex_decode("ebe76799923fd62804659fb00b4f0f1a94c0eb1e", expected_sender, 20));
  TEST_ASSERT_EQUAL_MEMORY(expected_sender, recover_result.address.bytes, 20);

  secp256k1_ctx_destroy(ctx);
}

// ============================================================================
// Negative Tests (Malformed Input)
// ============================================================================

void test_decode_empty_input(void) {
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(NULL, 0, &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_EMPTY_INPUT, result.error);

  uint8_t empty[1] = {0};
  result = transaction_decode(empty, 0, &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_EMPTY_INPUT, result.error);
}

void test_decode_invalid_type_byte(void) {
  // Type byte 0x05 is not currently defined (valid types are 0x01-0x04)
  uint8_t invalid_type[] = {0x05, 0xc0};
  transaction_t tx;
  tx_decode_result_t result =
      transaction_decode(invalid_type, sizeof(invalid_type), &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_INVALID_TYPE, result.error);
}

void test_decode_truncated_legacy(void) {
  // Truncated legacy transaction (list header says more bytes than provided)
  uint8_t truncated[] = {0xf8, 0x65, 0x80}; // Claims 101 bytes, only has 1
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(truncated, sizeof(truncated), &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_INVALID_RLP, result.error);
}

void test_decode_truncated_typed(void) {
  // Truncated EIP-1559 transaction
  uint8_t truncated[] = {0x02, 0xf8, 0x4f, 0x01}; // Claims 79 bytes, only has 1
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(truncated, sizeof(truncated), &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_INVALID_RLP, result.error);
}

void test_decode_not_a_list(void) {
  // Data that's not an RLP list (just a string) - 0x85 < 0xc0 so treated as typed tx
  // But 0x85 > 0x04 so invalid type
  uint8_t not_list[] = {0x85, 0x68, 0x65, 0x6c, 0x6c, 0x6f}; // "hello" as RLP string
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(not_list, sizeof(not_list), &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_INVALID_TYPE, result.error);
}

void test_decode_missing_fields(void) {
  // Legacy transaction with only 3 fields instead of 9
  // The RLP decoder will return INVALID_RLP when trying to decode more fields
  uint8_t incomplete[] = {0xc3, 0x80, 0x80, 0x80}; // [0, 0, 0]
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(incomplete, sizeof(incomplete), &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_INVALID_RLP, result.error);
}

// ============================================================================
// Encode/Decode Roundtrip Tests
// ============================================================================

void test_roundtrip_legacy_tx(void) {
  // Decode a real legacy transaction
  const char *hex =
      "f85f800182520894095e7baea6a6c7c4c2dfeb977efac326af552d870a801ba048b55bfa915ac795c"
      "431978d8a6a992b628d557da5ff759b307d495a36649353a01fffd310ac743f371de3b9f7f9cb56c"
      "0b28ad43601b4ab949f53faa07bd2c804";

  uint8_t original[256];
  size_t original_len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, original, original_len));

  // Decode
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(original, original_len, &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);

  // Re-encode
  bytes_t encoded = transaction_encode(&tx, &test_arena);
  TEST_ASSERT_EQUAL_UINT64(original_len, encoded.size);
  TEST_ASSERT_EQUAL_MEMORY(original, encoded.data, original_len);
}

void test_roundtrip_eip2930_tx(void) {
  // Decode a real EIP-2930 transaction
  const char *hex =
      "01f89a018001826a4094095e7baea6a6c7c4c2dfeb977efac326af552d878080f838f794a95e7bae"
      "a6a6c7c4c2dfeb977efac326af552d87e1a0fffffffffffffffffffffffffffffffffffffffffffff"
      "fffffffffffffffffff80a05cbd172231fc0735e0fb994dd5b1a4939170a260b36f0427a8a80866b0"
      "63b948a07c230f7f578dd61785c93361b9871c0706ebfa6d06e3f4491dc9558c5202ed36";

  uint8_t original[256];
  size_t original_len = HEX_LEN(hex);
  TEST_ASSERT_TRUE(hex_decode(hex, original, original_len));

  // Decode
  transaction_t tx;
  tx_decode_result_t result = transaction_decode(original, original_len, &tx, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);

  // Re-encode
  bytes_t encoded = transaction_encode(&tx, &test_arena);
  TEST_ASSERT_EQUAL_UINT64(original_len, encoded.size);
  TEST_ASSERT_EQUAL_MEMORY(original, encoded.data, original_len);
}

void test_roundtrip_constructed_legacy(void) {
  // Construct a legacy transaction programmatically
  transaction_t tx;
  tx.type = TX_TYPE_LEGACY;
  legacy_tx_init(&tx.legacy);

  tx.legacy.nonce = 42;
  tx.legacy.gas_price = uint256_from_u64(20000000000ULL);
  tx.legacy.gas_limit = 21000;
  tx.legacy.to = (address_t *)div0_arena_alloc(&test_arena, sizeof(address_t));
  memset(tx.legacy.to->bytes, 0xAB, 20);
  tx.legacy.value = uint256_from_u64(1000000000000000000ULL);
  bytes_init_arena(&tx.legacy.data, &test_arena);
  tx.legacy.v = 27;
  tx.legacy.r = uint256_from_u64(123456789);
  tx.legacy.s = uint256_from_u64(987654321);

  // Encode
  bytes_t encoded = transaction_encode(&tx, &test_arena);
  TEST_ASSERT_TRUE(encoded.size > 0);

  // Decode back
  transaction_t decoded;
  tx_decode_result_t result = transaction_decode(encoded.data, encoded.size, &decoded, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);

  // Verify fields match
  TEST_ASSERT_EQUAL_INT(TX_TYPE_LEGACY, decoded.type);
  TEST_ASSERT_EQUAL_UINT64(42, decoded.legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(21000, decoded.legacy.gas_limit);
  TEST_ASSERT_NOT_NULL(decoded.legacy.to);
  TEST_ASSERT_EQUAL_MEMORY(tx.legacy.to->bytes, decoded.legacy.to->bytes, 20);
  TEST_ASSERT_EQUAL_UINT64(27, decoded.legacy.v);
  TEST_ASSERT_TRUE(uint256_eq(tx.legacy.gas_price, decoded.legacy.gas_price));
  TEST_ASSERT_TRUE(uint256_eq(tx.legacy.value, decoded.legacy.value));
  TEST_ASSERT_TRUE(uint256_eq(tx.legacy.r, decoded.legacy.r));
  TEST_ASSERT_TRUE(uint256_eq(tx.legacy.s, decoded.legacy.s));
}

void test_roundtrip_constructed_eip1559(void) {
  // Construct an EIP-1559 transaction programmatically
  transaction_t tx;
  tx.type = TX_TYPE_EIP1559;
  eip1559_tx_init(&tx.eip1559);

  tx.eip1559.chain_id = 1;
  tx.eip1559.nonce = 100;
  tx.eip1559.max_priority_fee_per_gas = uint256_from_u64(1000000000);
  tx.eip1559.max_fee_per_gas = uint256_from_u64(2000000000);
  tx.eip1559.gas_limit = 50000;
  tx.eip1559.to = (address_t *)div0_arena_alloc(&test_arena, sizeof(address_t));
  memset(tx.eip1559.to->bytes, 0xCD, 20);
  tx.eip1559.value = uint256_from_u64(500000000);
  bytes_init_arena(&tx.eip1559.data, &test_arena);
  tx.eip1559.y_parity = 1;
  tx.eip1559.r = uint256_from_u64(111111111);
  tx.eip1559.s = uint256_from_u64(222222222);

  // Encode
  bytes_t encoded = transaction_encode(&tx, &test_arena);
  TEST_ASSERT_TRUE(encoded.size > 0);

  // Decode back
  transaction_t decoded;
  tx_decode_result_t result = transaction_decode(encoded.data, encoded.size, &decoded, &test_arena);
  TEST_ASSERT_EQUAL_INT(TX_DECODE_OK, result.error);

  // Verify fields match
  TEST_ASSERT_EQUAL_INT(TX_TYPE_EIP1559, decoded.type);
  TEST_ASSERT_EQUAL_UINT64(1, decoded.eip1559.chain_id);
  TEST_ASSERT_EQUAL_UINT64(100, decoded.eip1559.nonce);
  TEST_ASSERT_EQUAL_UINT64(50000, decoded.eip1559.gas_limit);
  TEST_ASSERT_EQUAL_UINT8(1, decoded.eip1559.y_parity);
  TEST_ASSERT_NOT_NULL(decoded.eip1559.to);
  TEST_ASSERT_EQUAL_MEMORY(tx.eip1559.to->bytes, decoded.eip1559.to->bytes, 20);
  TEST_ASSERT_TRUE(
      uint256_eq(tx.eip1559.max_priority_fee_per_gas, decoded.eip1559.max_priority_fee_per_gas));
  TEST_ASSERT_TRUE(uint256_eq(tx.eip1559.max_fee_per_gas, decoded.eip1559.max_fee_per_gas));
}
