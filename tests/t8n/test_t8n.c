#include "test_t8n.h"

#include "div0/mem/arena.h"
#include "div0/t8n/alloc.h"
#include "div0/t8n/env.h"
#include "div0/t8n/txs.h"

#include <stdlib.h>
#include <string.h>
#include <unity.h>

// ============================================================================
// Alloc Parsing Tests
// ============================================================================

void test_alloc_parse_empty(void) {
  const char *json = "{}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  state_snapshot_t snapshot;
  json_result_t result = t8n_parse_alloc(json, strlen(json), &arena, &snapshot);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(0, snapshot.account_count);

  div0_arena_destroy(&arena);
}

void test_alloc_parse_single_account(void) {
  const char *json = "{"
                     "  \"0x1234567890123456789012345678901234567890\": {"
                     "    \"balance\": \"0x100\""
                     "  }"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  state_snapshot_t snapshot;
  json_result_t result = t8n_parse_alloc(json, strlen(json), &arena, &snapshot);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, snapshot.account_count);

  const account_snapshot_t *account = &snapshot.accounts[0];
  TEST_ASSERT_EQUAL_UINT8(0x12, account->address.bytes[0]);
  TEST_ASSERT_EQUAL_UINT64(0x100, account->balance.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, account->nonce);
  TEST_ASSERT_EQUAL(0, account->code.size);
  TEST_ASSERT_EQUAL(0, account->storage_count);

  div0_arena_destroy(&arena);
}

void test_alloc_parse_with_storage(void) {
  const char *json = "{"
                     "  \"0x1234567890123456789012345678901234567890\": {"
                     "    \"balance\": \"0x0\","
                     "    \"storage\": {"
                     "      \"0x01\": \"0x02\","
                     "      \"0x03\": \"0x04\""
                     "    }"
                     "  }"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  state_snapshot_t snapshot;
  json_result_t result = t8n_parse_alloc(json, strlen(json), &arena, &snapshot);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, snapshot.account_count);

  const account_snapshot_t *account = &snapshot.accounts[0];
  TEST_ASSERT_EQUAL(2, account->storage_count);

  div0_arena_destroy(&arena);
}

void test_alloc_parse_with_code(void) {
  const char *json = "{"
                     "  \"0x1234567890123456789012345678901234567890\": {"
                     "    \"balance\": \"0x0\","
                     "    \"code\": \"0x6080604052\","
                     "    \"nonce\": \"0x5\""
                     "  }"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  state_snapshot_t snapshot;
  json_result_t result = t8n_parse_alloc(json, strlen(json), &arena, &snapshot);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, snapshot.account_count);

  const account_snapshot_t *account = &snapshot.accounts[0];
  TEST_ASSERT_EQUAL_UINT64(5, account->nonce);
  TEST_ASSERT_EQUAL(5, account->code.size);
  TEST_ASSERT_EQUAL_UINT8(0x60, account->code.data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x80, account->code.data[1]);

  div0_arena_destroy(&arena);
}

void test_alloc_roundtrip(void) {
  const char *json = "{"
                     "  \"0x1234567890123456789012345678901234567890\": {"
                     "    \"balance\": \"0x100\","
                     "    \"nonce\": \"0x1\""
                     "  }"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  state_snapshot_t snapshot;
  json_result_t result = t8n_parse_alloc(json, strlen(json), &arena, &snapshot);
  TEST_ASSERT_TRUE(json_is_ok(result));

  // Write back to JSON
  json_writer_t w;
  result = json_writer_init(&w);
  TEST_ASSERT_TRUE(json_is_ok(result));

  yyjson_mut_val_t *obj = t8n_write_alloc(&snapshot, &w);
  TEST_ASSERT_NOT_NULL(obj);

  size_t len;
  char *output = json_write_string(&w, obj, JSON_WRITE_COMPACT, &len);
  TEST_ASSERT_NOT_NULL(output);

  // Verify contains expected data
  TEST_ASSERT_NOT_NULL(strstr(output, "0x1234567890123456789012345678901234567890"));
  TEST_ASSERT_NOT_NULL(strstr(output, "balance"));

  free(output);
  json_writer_free(&w);
  div0_arena_destroy(&arena);
}

// ============================================================================
// Env Parsing Tests
// ============================================================================

void test_env_parse_required_fields(void) {
  const char *json = "{"
                     "  \"currentCoinbase\": \"0x1234567890123456789012345678901234567890\","
                     "  \"currentGasLimit\": \"0x1000000\","
                     "  \"currentNumber\": \"0x10\","
                     "  \"currentTimestamp\": \"0x5f5e100\""
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_env_t env;
  json_result_t result = t8n_parse_env(json, strlen(json), &arena, &env);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL_UINT8(0x12, env.coinbase.bytes[0]);
  TEST_ASSERT_EQUAL_UINT64(0x1000000, env.gas_limit);
  TEST_ASSERT_EQUAL_UINT64(0x10, env.number);
  TEST_ASSERT_EQUAL_UINT64(0x5f5e100, env.timestamp);

  div0_arena_destroy(&arena);
}

void test_env_parse_optional_fields(void) {
  const char *json = "{"
                     "  \"currentCoinbase\": \"0x0000000000000000000000000000000000000000\","
                     "  \"currentGasLimit\": \"0x1000000\","
                     "  \"currentNumber\": \"0x10\","
                     "  \"currentTimestamp\": \"0x5f5e100\","
                     "  \"currentBaseFee\": \"0x3b9aca00\","
                     "  \"currentDifficulty\": \"0x20000\""
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_env_t env;
  json_result_t result = t8n_parse_env(json, strlen(json), &arena, &env);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_TRUE(env.has_base_fee);
  TEST_ASSERT_EQUAL_UINT64(0x3b9aca00, env.base_fee.limbs[0]);
  TEST_ASSERT_TRUE(env.has_difficulty);
  TEST_ASSERT_EQUAL_UINT64(0x20000, env.difficulty.limbs[0]);

  div0_arena_destroy(&arena);
}

void test_env_parse_block_hashes(void) {
  const char *json = "{"
                     "  \"currentCoinbase\": \"0x0000000000000000000000000000000000000000\","
                     "  \"currentGasLimit\": \"0x1000000\","
                     "  \"currentNumber\": \"0x10\","
                     "  \"currentTimestamp\": \"0x5f5e100\","
                     "  \"blockHashes\": {"
                     "    \"0x0f\": "
                     "\"0x0000000000000000000000000000000000000000000000000000000000001234\""
                     "  }"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_env_t env;
  json_result_t result = t8n_parse_env(json, strlen(json), &arena, &env);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, env.block_hash_count);
  TEST_ASSERT_EQUAL_UINT64(0x0f, env.block_hashes[0].number);
  TEST_ASSERT_EQUAL_UINT8(0x12, env.block_hashes[0].hash.bytes[30]);
  TEST_ASSERT_EQUAL_UINT8(0x34, env.block_hashes[0].hash.bytes[31]);

  div0_arena_destroy(&arena);
}

void test_env_parse_withdrawals(void) {
  const char *json = "{"
                     "  \"currentCoinbase\": \"0x0000000000000000000000000000000000000000\","
                     "  \"currentGasLimit\": \"0x1000000\","
                     "  \"currentNumber\": \"0x10\","
                     "  \"currentTimestamp\": \"0x5f5e100\","
                     "  \"withdrawals\": ["
                     "    {"
                     "      \"index\": \"0x0\","
                     "      \"validatorIndex\": \"0x1\","
                     "      \"address\": \"0x1234567890123456789012345678901234567890\","
                     "      \"amount\": \"0x100\""
                     "    }"
                     "  ]"
                     "}";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_env_t env;
  json_result_t result = t8n_parse_env(json, strlen(json), &arena, &env);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, env.withdrawal_count);
  TEST_ASSERT_EQUAL_UINT64(0, env.withdrawals[0].index);
  TEST_ASSERT_EQUAL_UINT64(1, env.withdrawals[0].validator_index);
  TEST_ASSERT_EQUAL_UINT64(0x100, env.withdrawals[0].amount);

  div0_arena_destroy(&arena);
}

// ============================================================================
// Txs Parsing Tests
// ============================================================================

void test_txs_parse_empty_array(void) {
  const char *json = "[]";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_txs_t txs;
  json_result_t result = t8n_parse_txs(json, strlen(json), &arena, &txs);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(0, txs.tx_count);

  div0_arena_destroy(&arena);
}

void test_txs_parse_legacy(void) {
  const char *json = "["
                     "  {"
                     "    \"type\": \"0x0\","
                     "    \"nonce\": \"0x1\","
                     "    \"gasPrice\": \"0x3b9aca00\","
                     "    \"gas\": \"0x5208\","
                     "    \"to\": \"0x1234567890123456789012345678901234567890\","
                     "    \"value\": \"0xde0b6b3a7640000\","
                     "    \"input\": \"0x\","
                     "    \"v\": \"0x1b\","
                     "    \"r\": \"0x1\","
                     "    \"s\": \"0x2\""
                     "  }"
                     "]";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_txs_t txs;
  json_result_t result = t8n_parse_txs(json, strlen(json), &arena, &txs);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, txs.tx_count);
  TEST_ASSERT_EQUAL(TX_TYPE_LEGACY, txs.txs[0].type);
  TEST_ASSERT_EQUAL_UINT64(1, txs.txs[0].legacy.nonce);
  TEST_ASSERT_EQUAL_UINT64(0x5208, txs.txs[0].legacy.gas_limit);
  TEST_ASSERT_EQUAL_UINT64(0x1b, txs.txs[0].legacy.v);

  div0_arena_destroy(&arena);
}

void test_txs_parse_eip1559(void) {
  const char *json = "["
                     "  {"
                     "    \"type\": \"0x2\","
                     "    \"chainId\": \"0x1\","
                     "    \"nonce\": \"0x0\","
                     "    \"maxPriorityFeePerGas\": \"0x3b9aca00\","
                     "    \"maxFeePerGas\": \"0x77359400\","
                     "    \"gas\": \"0x5208\","
                     "    \"to\": \"0x1234567890123456789012345678901234567890\","
                     "    \"value\": \"0x0\","
                     "    \"input\": \"0x\","
                     "    \"accessList\": [],"
                     "    \"yParity\": \"0x0\","
                     "    \"r\": \"0x1\","
                     "    \"s\": \"0x2\""
                     "  }"
                     "]";
  div0_arena_t arena;
  TEST_ASSERT_TRUE(div0_arena_init(&arena));

  t8n_txs_t txs;
  json_result_t result = t8n_parse_txs(json, strlen(json), &arena, &txs);

  TEST_ASSERT_TRUE(json_is_ok(result));
  TEST_ASSERT_EQUAL(1, txs.tx_count);
  TEST_ASSERT_EQUAL(TX_TYPE_EIP1559, txs.txs[0].type);
  TEST_ASSERT_EQUAL_UINT64(1, txs.txs[0].eip1559.chain_id);
  TEST_ASSERT_EQUAL_UINT64(0, txs.txs[0].eip1559.nonce);
  TEST_ASSERT_EQUAL_UINT64(0x5208, txs.txs[0].eip1559.gas_limit);

  div0_arena_destroy(&arena);
}
