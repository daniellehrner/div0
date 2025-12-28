#ifndef TEST_TRANSACTION_H
#define TEST_TRANSACTION_H

// Transaction type tests
void test_transaction_type_enum(void);
void test_transaction_init_default(void);

// Legacy transaction decoding tests
void test_legacy_tx_decode_basic(void);
void test_legacy_tx_decode_contract_creation(void);
void test_legacy_tx_chain_id_eip155(void);
void test_legacy_tx_chain_id_pre_eip155(void);

// EIP-1559 transaction decoding tests
void test_eip1559_tx_decode_basic(void);
void test_eip1559_tx_effective_gas_price(void);

// Transaction accessors tests
void test_transaction_accessors(void);

// Signing hash tests
void test_legacy_tx_signing_hash(void);
void test_eip1559_tx_signing_hash(void);

// Sender recovery tests
void test_transaction_recover_sender_legacy(void);

// Real test vectors from Ethereum tests
void test_real_vector_legacy_pre_eip155(void);
void test_real_vector_legacy_eip155(void);
void test_real_vector_legacy_vitalik_2(void);
void test_real_vector_eip2930(void);

// Negative tests (malformed input)
void test_decode_empty_input(void);
void test_decode_invalid_type_byte(void);
void test_decode_truncated_legacy(void);
void test_decode_truncated_typed(void);
void test_decode_not_a_list(void);
void test_decode_missing_fields(void);

// Encode/decode roundtrip tests
void test_roundtrip_legacy_tx(void);
void test_roundtrip_eip2930_tx(void);
void test_roundtrip_constructed_legacy(void);
void test_roundtrip_constructed_eip1559(void);

#endif // TEST_TRANSACTION_H
