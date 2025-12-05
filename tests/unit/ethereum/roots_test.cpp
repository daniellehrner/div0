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

}  // namespace div0::ethereum
