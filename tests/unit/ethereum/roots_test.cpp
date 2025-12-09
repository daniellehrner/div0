// Unit tests for Ethereum root computation and account encoding

#include <div0/ethereum/account.h>
#include <div0/ethereum/roots.h>
#include <div0/trie/mpt.h>
#include <gtest/gtest.h>

#include <map>
#include <vector>

namespace div0::ethereum {

// =============================================================================
// Constants
// =============================================================================

TEST(EthereumConstants, EmptyCodeHash) {
  // keccak256("") = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
  const auto expected =
      types::Hash::from_hex("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  EXPECT_EQ(EMPTY_CODE_HASH, expected);
}

// =============================================================================
// Account RLP Encoding
// =============================================================================

TEST(AccountRlpEncode, EmptyAccount) {
  // Empty account: [0, 0, EMPTY_ROOT, EMPTY_CODE_HASH]
  AccountState account;
  account.nonce = 0;
  account.balance = types::Uint256::zero();
  account.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  account.code_hash = EMPTY_CODE_HASH;

  const auto encoded = rlp_encode_account(account);

  // Should be a list containing:
  // - nonce: 0x80 (empty string for 0)
  // - balance: 0x80 (empty string for 0)
  // - storage_root: 0xa0 + 32 bytes
  // - code_hash: 0xa0 + 32 bytes
  // Total payload: 1 + 1 + 33 + 33 = 68 bytes
  // List prefix: 0xf8 0x44 (short list with 68 byte payload)
  EXPECT_EQ(encoded.size(), 70);  // 2 (list header) + 68 (payload)
  EXPECT_EQ(encoded[0], 0xf8);    // List with 2-byte length
  EXPECT_EQ(encoded[1], 0x44);    // 68 bytes payload
}

TEST(AccountRlpEncode, NonZeroNonce) {
  AccountState account;
  account.nonce = 1;
  account.balance = types::Uint256::zero();
  account.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  account.code_hash = EMPTY_CODE_HASH;

  const auto encoded = rlp_encode_account(account);

  // nonce = 1 encodes as single byte 0x01
  EXPECT_EQ(encoded[2], 0x01);  // After list header
}

TEST(AccountRlpEncode, NonZeroBalance) {
  AccountState account;
  account.nonce = 0;
  account.balance = types::Uint256(1000000);  // 0x0F4240
  account.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  account.code_hash = EMPTY_CODE_HASH;

  const auto encoded = rlp_encode_account(account);

  // balance = 0x0F4240 (3 bytes) encodes as 0x83 0x0F 0x42 0x40
  // Find balance after nonce (0x80)
  EXPECT_EQ(encoded[2], 0x80);  // nonce = 0
  EXPECT_EQ(encoded[3], 0x83);  // balance prefix (3-byte string)
  EXPECT_EQ(encoded[4], 0x0F);
  EXPECT_EQ(encoded[5], 0x42);
  EXPECT_EQ(encoded[6], 0x40);
}

// =============================================================================
// State Root Computation
// =============================================================================

TEST(StateRoot, EmptyState) {
  const std::map<types::Address, AccountState> accounts;
  const auto root = compute_state_root(accounts);

  // Empty state should return empty trie root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StateRoot, SingleAccount) {
  std::map<types::Address, AccountState> accounts;

  // Create a simple account
  AccountState account;
  account.nonce = 0;
  account.balance = types::Uint256::zero();
  account.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  account.code_hash = EMPTY_CODE_HASH;

  const types::Address addr = types::Address::zero();
  accounts[addr] = account;

  const auto root = compute_state_root(accounts);

  // Should produce a valid non-empty root
  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StateRoot, Deterministic) {
  std::map<types::Address, AccountState> accounts;

  AccountState account;
  account.nonce = 1;
  account.balance = types::Uint256(100);
  account.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  account.code_hash = EMPTY_CODE_HASH;

  accounts[types::Address::zero()] = account;

  // Compute root multiple times
  const auto root1 = compute_state_root(accounts);
  const auto root2 = compute_state_root(accounts);
  const auto root3 = compute_state_root(accounts);

  // Should be deterministic
  EXPECT_EQ(root1, root2);
  EXPECT_EQ(root2, root3);
}

// =============================================================================
// Transactions Root Computation
// =============================================================================

TEST(TransactionsRoot, EmptyTransactions) {
  const std::vector<types::Bytes> tx_rlps;
  const auto root = compute_transactions_root(tx_rlps);

  // Empty transactions should return empty trie root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(TransactionsRoot, SingleTransaction) {
  std::vector<types::Bytes> tx_rlps;

  // Add a mock transaction (just some bytes)
  tx_rlps.push_back({0xf8, 0x65, 0x80, 0x85, 0x04, 0xa8, 0x17, 0xc8, 0x00});

  const auto root = compute_transactions_root(tx_rlps);

  // Should produce a valid non-empty root
  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(TransactionsRoot, Deterministic) {
  std::vector<types::Bytes> tx_rlps;
  tx_rlps.push_back({0xf8, 0x65, 0x80});
  tx_rlps.push_back({0xf8, 0x66, 0x01});

  const auto root1 = compute_transactions_root(tx_rlps);
  const auto root2 = compute_transactions_root(tx_rlps);

  EXPECT_EQ(root1, root2);
}

TEST(TransactionsRoot, OrderMatters) {
  std::vector<types::Bytes> tx_rlps1;
  tx_rlps1.push_back({0xaa});
  tx_rlps1.push_back({0xbb});

  std::vector<types::Bytes> tx_rlps2;
  tx_rlps2.push_back({0xbb});
  tx_rlps2.push_back({0xaa});

  const auto root1 = compute_transactions_root(tx_rlps1);
  const auto root2 = compute_transactions_root(tx_rlps2);

  // Different order should produce different roots
  EXPECT_NE(root1, root2);
}

// =============================================================================
// Receipts Root Computation
// =============================================================================

TEST(ReceiptsRoot, EmptyReceipts) {
  const std::vector<types::Bytes> receipt_rlps;
  const auto root = compute_receipts_root(receipt_rlps);

  // Empty receipts should return empty trie root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(ReceiptsRoot, SingleReceipt) {
  std::vector<types::Bytes> receipt_rlps;

  // Add a mock receipt (just some bytes)
  receipt_rlps.push_back({0xf9, 0x01, 0x00, 0x01, 0x82, 0x52, 0x08});

  const auto root = compute_receipts_root(receipt_rlps);

  // Should produce a valid non-empty root
  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(ReceiptsRoot, Deterministic) {
  std::vector<types::Bytes> receipt_rlps;
  receipt_rlps.push_back({0xf9, 0x01});
  receipt_rlps.push_back({0xf9, 0x02});

  const auto root1 = compute_receipts_root(receipt_rlps);
  const auto root2 = compute_receipts_root(receipt_rlps);

  EXPECT_EQ(root1, root2);
}

// =============================================================================
// Storage Root Computation
// =============================================================================

TEST(StorageRoot, EmptyStorage) {
  const std::map<types::Uint256, types::Uint256> storage;
  const auto root = compute_storage_root(storage);

  // Empty storage should return empty trie root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, AllZeroValues) {
  // Storage with only zero values should be treated as empty
  std::map<types::Uint256, types::Uint256> storage;
  storage[types::Uint256(1)] = types::Uint256::zero();
  storage[types::Uint256(2)] = types::Uint256::zero();
  storage[types::Uint256(100)] = types::Uint256::zero();

  const auto root = compute_storage_root(storage);

  // All zero values are excluded, so should be empty root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, SingleSlot) {
  std::map<types::Uint256, types::Uint256> storage;
  storage[types::Uint256(0)] = types::Uint256(1);

  const auto root = compute_storage_root(storage);

  // Should produce a valid non-empty root
  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, MultipleSlots) {
  std::map<types::Uint256, types::Uint256> storage;
  storage[types::Uint256(0)] = types::Uint256(100);
  storage[types::Uint256(1)] = types::Uint256(200);
  storage[types::Uint256(2)] = types::Uint256(300);

  const auto root = compute_storage_root(storage);

  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, ZeroValuesExcluded) {
  // Storage with mixed zero and non-zero values
  std::map<types::Uint256, types::Uint256> storage_with_zeros;
  storage_with_zeros[types::Uint256(0)] = types::Uint256(100);
  storage_with_zeros[types::Uint256(1)] = types::Uint256::zero();  // Should be excluded
  storage_with_zeros[types::Uint256(2)] = types::Uint256(300);

  std::map<types::Uint256, types::Uint256> storage_without_zeros;
  storage_without_zeros[types::Uint256(0)] = types::Uint256(100);
  storage_without_zeros[types::Uint256(2)] = types::Uint256(300);

  const auto root_with_zeros = compute_storage_root(storage_with_zeros);
  const auto root_without_zeros = compute_storage_root(storage_without_zeros);

  // Both should produce the same root since zero values are excluded
  EXPECT_EQ(root_with_zeros, root_without_zeros);
}

TEST(StorageRoot, Deterministic) {
  std::map<types::Uint256, types::Uint256> storage;
  storage[types::Uint256(0x1234)] = types::Uint256(0xABCD);
  storage[types::Uint256(0x5678)] = types::Uint256(0xEF01);

  const auto root1 = compute_storage_root(storage);
  const auto root2 = compute_storage_root(storage);
  const auto root3 = compute_storage_root(storage);

  EXPECT_EQ(root1, root2);
  EXPECT_EQ(root2, root3);
}

TEST(StorageRoot, LargeSlotKey) {
  std::map<types::Uint256, types::Uint256> storage;
  // Large slot key (full 256-bit value)
  const types::Uint256 large_key(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
  storage[large_key] = types::Uint256(42);

  const auto root = compute_storage_root(storage);

  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, LargeValue) {
  std::map<types::Uint256, types::Uint256> storage;
  // Large value (full 256-bit value)
  const types::Uint256 large_value(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL,
                                   0xFEDCBA0987654321ULL, 0xABCDEF0123456789ULL);
  storage[types::Uint256(0)] = large_value;

  const auto root = compute_storage_root(storage);

  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(StorageRoot, DifferentSlotsDifferentRoots) {
  std::map<types::Uint256, types::Uint256> storage1;
  storage1[types::Uint256(0)] = types::Uint256(100);

  std::map<types::Uint256, types::Uint256> storage2;
  storage2[types::Uint256(1)] = types::Uint256(100);

  const auto root1 = compute_storage_root(storage1);
  const auto root2 = compute_storage_root(storage2);

  // Different slots should produce different roots
  EXPECT_NE(root1, root2);
}

TEST(StorageRoot, DifferentValuesDifferentRoots) {
  std::map<types::Uint256, types::Uint256> storage1;
  storage1[types::Uint256(0)] = types::Uint256(100);

  std::map<types::Uint256, types::Uint256> storage2;
  storage2[types::Uint256(0)] = types::Uint256(200);

  const auto root1 = compute_storage_root(storage1);
  const auto root2 = compute_storage_root(storage2);

  // Different values should produce different roots
  EXPECT_NE(root1, root2);
}

// =============================================================================
// Withdrawals Root Computation
// =============================================================================

TEST(WithdrawalsRoot, EmptyWithdrawals) {
  const std::vector<Withdrawal> withdrawals;
  const auto root = compute_withdrawals_root(withdrawals);

  // Empty withdrawals should return empty trie root
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(WithdrawalsRoot, SingleWithdrawal) {
  std::vector<Withdrawal> withdrawals;
  Withdrawal w;
  w.index = 0;
  w.validator_index = 1;
  w.address = types::Address::zero();
  w.amount = 1000;
  withdrawals.push_back(w);

  const auto root = compute_withdrawals_root(withdrawals);

  // Should produce a valid non-empty root
  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(WithdrawalsRoot, MultipleWithdrawals) {
  std::vector<Withdrawal> withdrawals;

  Withdrawal w1;
  w1.index = 0;
  w1.validator_index = 100;
  w1.address = types::Address::zero();
  w1.amount = 32000000000;  // 32 ETH in Gwei
  withdrawals.push_back(w1);

  Withdrawal w2;
  w2.index = 1;
  w2.validator_index = 101;
  w2.address = types::Address(types::Uint256(1));
  w2.amount = 16000000000;  // 16 ETH in Gwei
  withdrawals.push_back(w2);

  const auto root = compute_withdrawals_root(withdrawals);

  EXPECT_FALSE(root.is_zero());
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

TEST(WithdrawalsRoot, Deterministic) {
  std::vector<Withdrawal> withdrawals;
  Withdrawal w;
  w.index = 42;
  w.validator_index = 123;
  w.address = types::Address(types::Uint256(0xDEAD));
  w.amount = 5000000000;
  withdrawals.push_back(w);

  const auto root1 = compute_withdrawals_root(withdrawals);
  const auto root2 = compute_withdrawals_root(withdrawals);
  const auto root3 = compute_withdrawals_root(withdrawals);

  EXPECT_EQ(root1, root2);
  EXPECT_EQ(root2, root3);
}

TEST(WithdrawalsRoot, OrderMatters) {
  Withdrawal w1;
  w1.index = 0;
  w1.validator_index = 100;
  w1.address = types::Address::zero();
  w1.amount = 1000;

  Withdrawal w2;
  w2.index = 1;
  w2.validator_index = 200;
  w2.address = types::Address(types::Uint256(1));
  w2.amount = 2000;

  const std::vector withdrawals1 = {w1, w2};
  const std::vector withdrawals2 = {w2, w1};

  const auto root1 = compute_withdrawals_root(withdrawals1);
  const auto root2 = compute_withdrawals_root(withdrawals2);

  // Different order should produce different roots
  EXPECT_NE(root1, root2);
}

TEST(WithdrawalsRoot, ZeroAmountIncluded) {
  // Unlike storage, zero-amount withdrawals ARE included
  std::vector<Withdrawal> withdrawals;
  Withdrawal w;
  w.index = 0;
  w.validator_index = 1;
  w.address = types::Address::zero();
  w.amount = 0;  // Zero amount
  withdrawals.push_back(w);

  const auto root = compute_withdrawals_root(withdrawals);

  // Should NOT be empty root (zero withdrawals are included)
  EXPECT_NE(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

// =============================================================================
// Withdrawals Root Test Vectors from go-ethereum
// =============================================================================
// Test vectors from: references/go-ethereum/cmd/evm/testdata/26/

TEST(WithdrawalsRoot, GethTestVector_SingleWithdrawal) {
  // From testdata/26/env.json:
  // {
  //   "index": "0x42",
  //   "validatorIndex": "0x42",
  //   "address": "0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b",
  //   "amount": "0x2a"
  // }
  // Expected withdrawalsRoot from exp.json:
  // "0x4921c0162c359755b2ae714a0978a1dad2eb8edce7ff9b38b9b6fc4cbc547eb5"

  std::vector<Withdrawal> withdrawals;
  Withdrawal w;
  w.index = 0x42;
  w.validator_index = 0x42;

  // Address: 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b
  const std::array<uint8_t, 20> addr_bytes = {0xa9, 0x4f, 0x53, 0x74, 0xfc, 0xe5, 0xed,
                                              0xbc, 0x8e, 0x2a, 0x86, 0x97, 0xc1, 0x53,
                                              0x31, 0x67, 0x7e, 0x6e, 0xbf, 0x0b};
  w.address = types::Address(addr_bytes);
  w.amount = 0x2a;

  withdrawals.push_back(w);

  const auto root = compute_withdrawals_root(withdrawals);

  // Expected: 0x4921c0162c359755b2ae714a0978a1dad2eb8edce7ff9b38b9b6fc4cbc547eb5
  const auto expected =
      types::Hash::from_hex("4921c0162c359755b2ae714a0978a1dad2eb8edce7ff9b38b9b6fc4cbc547eb5");

  EXPECT_EQ(root, expected);
}

TEST(WithdrawalsRoot, GethTestVector_EmptyWithdrawals) {
  // From testdata/28/env.json: "withdrawals" : []
  // Expected withdrawalsRoot from exp.json:
  // "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"

  const std::vector<Withdrawal> withdrawals;
  const auto root = compute_withdrawals_root(withdrawals);

  // Expected: EMPTY_ROOT
  EXPECT_EQ(root, trie::MerklePatriciaTrie::EMPTY_ROOT);
}

}  // namespace div0::ethereum
