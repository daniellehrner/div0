// Unit tests for MemoryAccountProvider implementation
// Covers: balance operations, nonce operations, account lifecycle,
// EIP-2929 warm/cold tracking, and is_empty checking

#include <div0/ethereum/account.h>
#include <div0/state/memory_account_provider.h>
#include <div0/utils/hex.h>
#include <gtest/gtest.h>

namespace div0::state {
namespace {

using types::Address;
using types::Hash;
using types::Uint256;

// Test addresses using hex::decode_address
const Address ADDR_A = hex::decode_address("0x1111111111111111111111111111111111111111").value();
const Address ADDR_B = hex::decode_address("0x2222222222222222222222222222222222222222").value();
const Address ADDR_C = hex::decode_address("0x3333333333333333333333333333333333333333").value();

// Test code hash (non-empty) using hex::decode_hash
const Hash NON_EMPTY_CODE_HASH =
    hex::decode_hash("0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789").value();

// Maximum uint64 and uint256 values
constexpr uint64_t MAX64 = UINT64_MAX;
const Uint256 MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);

// =============================================================================
// Account Existence: Non-Existent Account Behavior
// =============================================================================

TEST(MemoryAccountProviderNonExistent, GetBalanceReturnsZero) {
  MemoryAccountProvider provider;
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
}

TEST(MemoryAccountProviderNonExistent, GetNonceReturnsZero) {
  MemoryAccountProvider provider;
  EXPECT_EQ(provider.get_nonce(ADDR_A), 0);
}

TEST(MemoryAccountProviderNonExistent, GetCodeHashReturnsEmptyCodeHash) {
  MemoryAccountProvider provider;
  EXPECT_EQ(provider.get_code_hash(ADDR_A), ethereum::EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderNonExistent, ExistsReturnsFalse) {
  MemoryAccountProvider provider;
  EXPECT_FALSE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderNonExistent, IsEmptyReturnsTrue) {
  MemoryAccountProvider provider;
  EXPECT_TRUE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderNonExistent, IsWarmReturnsFalse) {
  MemoryAccountProvider provider;
  EXPECT_FALSE(provider.is_warm(ADDR_A));
}

// =============================================================================
// Account Creation: create_account()
// =============================================================================

TEST(MemoryAccountProviderCreate, CreateNewAccount) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
  EXPECT_EQ(provider.get_nonce(ADDR_A), 0);
  EXPECT_EQ(provider.get_code_hash(ADDR_A), ethereum::EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderCreate, CreateExistingAccountIsNoOp) {
  MemoryAccountProvider provider;

  // Create and modify account
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(1000));
  provider.set_nonce(ADDR_A, 5);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  // Create again should not reset values
  provider.create_account(ADDR_A);

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(1000));
  EXPECT_EQ(provider.get_nonce(ADDR_A), 5);
  EXPECT_EQ(provider.get_code_hash(ADDR_A), NON_EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderCreate, NewAccountIsEmpty) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);

  EXPECT_TRUE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderCreate, CreateMultipleAccounts) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);
  provider.create_account(ADDR_B);
  provider.create_account(ADDR_C);

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_TRUE(provider.exists(ADDR_B));
  EXPECT_TRUE(provider.exists(ADDR_C));
}

// =============================================================================
// Account Deletion: delete_account()
// =============================================================================

TEST(MemoryAccountProviderDelete, DeleteExistingAccount) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(1000));

  provider.delete_account(ADDR_A);

  EXPECT_FALSE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
}

TEST(MemoryAccountProviderDelete, DeleteNonExistentAccountIsNoOp) {
  MemoryAccountProvider provider;

  // Should not throw or crash
  provider.delete_account(ADDR_A);

  EXPECT_FALSE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderDelete, RecreateAfterDelete) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(1000));
  provider.set_nonce(ADDR_A, 5);

  provider.delete_account(ADDR_A);
  provider.create_account(ADDR_A);

  // New account should have default values
  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
  EXPECT_EQ(provider.get_nonce(ADDR_A), 0);
}

TEST(MemoryAccountProviderDelete, DeleteDoesNotAffectOtherAccounts) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);
  provider.create_account(ADDR_B);
  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_B, Uint256(200));

  provider.delete_account(ADDR_A);

  EXPECT_FALSE(provider.exists(ADDR_A));
  EXPECT_TRUE(provider.exists(ADDR_B));
  EXPECT_EQ(provider.get_balance(ADDR_B), Uint256(200));
}

// =============================================================================
// Balance Operations: set_balance(), get_balance()
// =============================================================================

TEST(MemoryAccountProviderBalance, SetAndGetBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(12345));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(12345));
}

TEST(MemoryAccountProviderBalance, SetBalanceCreatesAccount) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.exists(ADDR_A));

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
}

TEST(MemoryAccountProviderBalance, SetBalanceToZero) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(1000));
  provider.set_balance(ADDR_A, Uint256::zero());

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
}

TEST(MemoryAccountProviderBalance, SetMaxBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, MAX256);

  EXPECT_EQ(provider.get_balance(ADDR_A), MAX256);
}

TEST(MemoryAccountProviderBalance, OverwriteBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_A, Uint256(999));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(999));
}

TEST(MemoryAccountProviderBalance, IndependentBalances) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_B, Uint256(200));
  provider.set_balance(ADDR_C, Uint256(300));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
  EXPECT_EQ(provider.get_balance(ADDR_B), Uint256(200));
  EXPECT_EQ(provider.get_balance(ADDR_C), Uint256(300));
}

// =============================================================================
// Balance Operations: add_balance()
// =============================================================================

TEST(MemoryAccountProviderAddBalance, AddToExistingBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(50)));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(150));
}

TEST(MemoryAccountProviderAddBalance, AddToNonExistentAccountCreatesIt) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.exists(ADDR_A));

  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(100)));

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
}

TEST(MemoryAccountProviderAddBalance, AddZeroSucceeds) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256::zero()));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
}

TEST(MemoryAccountProviderAddBalance, AddZeroToNonExistentDoesNotCreate) {
  MemoryAccountProvider provider;

  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256::zero()));

  // Adding zero should not create the account
  EXPECT_FALSE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderAddBalance, OverflowDetection) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, MAX256);

  // Adding 1 to MAX should overflow
  EXPECT_FALSE(provider.add_balance(ADDR_A, Uint256(1)));

  // Balance should be unchanged
  EXPECT_EQ(provider.get_balance(ADDR_A), MAX256);
}

TEST(MemoryAccountProviderAddBalance, OverflowWithLargeAmount) {
  MemoryAccountProvider provider;

  // Set balance to half of max
  const Uint256 half_max = MAX256 / Uint256(2);
  provider.set_balance(ADDR_A, half_max);

  // Adding more than half should overflow
  EXPECT_FALSE(provider.add_balance(ADDR_A, half_max + Uint256(2)));

  // Balance should be unchanged
  EXPECT_EQ(provider.get_balance(ADDR_A), half_max);
}

TEST(MemoryAccountProviderAddBalance, NoOverflowAtBoundary) {
  MemoryAccountProvider provider;

  const Uint256 almost_max = MAX256 - Uint256(100);
  provider.set_balance(ADDR_A, almost_max);

  // Adding exactly 100 should succeed (reaches MAX256 - but doesn't overflow)
  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(100)));
  EXPECT_EQ(provider.get_balance(ADDR_A), MAX256);
}

TEST(MemoryAccountProviderAddBalance, MultipleAdds) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(10)));
  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(20)));
  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(30)));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(160));
}

// =============================================================================
// Balance Operations: subtract_balance()
// =============================================================================

TEST(MemoryAccountProviderSubtractBalance, SubtractFromExistingBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(30)));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(70));
}

TEST(MemoryAccountProviderSubtractBalance, SubtractFromNonExistentFails) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.subtract_balance(ADDR_A, Uint256(100)));

  // Account should not be created
  EXPECT_FALSE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderSubtractBalance, SubtractZeroSucceeds) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256::zero()));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
}

TEST(MemoryAccountProviderSubtractBalance, SubtractZeroFromNonExistentSucceeds) {
  MemoryAccountProvider provider;

  // Subtracting zero from non-existent should succeed
  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256::zero()));

  // Account should not be created
  EXPECT_FALSE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderSubtractBalance, UnderflowDetection) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(50));

  // Subtracting more than balance should fail
  EXPECT_FALSE(provider.subtract_balance(ADDR_A, Uint256(100)));

  // Balance should be unchanged
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(50));
}

TEST(MemoryAccountProviderSubtractBalance, ExactBalanceSucceeds) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(100)));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
}

TEST(MemoryAccountProviderSubtractBalance, UnderflowFromZeroBalance) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);  // Balance is 0

  EXPECT_FALSE(provider.subtract_balance(ADDR_A, Uint256(1)));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256::zero());
}

TEST(MemoryAccountProviderSubtractBalance, MultipleSubtractions) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));

  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(10)));
  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(20)));
  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(30)));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(40));
}

TEST(MemoryAccountProviderSubtractBalance, SubtractLargeAmount) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, MAX256);

  EXPECT_TRUE(provider.subtract_balance(ADDR_A, MAX256 - Uint256(1)));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(1));
}

// =============================================================================
// Nonce Operations: set_nonce(), get_nonce(), increment_nonce()
// =============================================================================

TEST(MemoryAccountProviderNonce, SetAndGetNonce) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, 42);

  EXPECT_EQ(provider.get_nonce(ADDR_A), 42);
}

TEST(MemoryAccountProviderNonce, SetNonceCreatesAccount) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.exists(ADDR_A));

  provider.set_nonce(ADDR_A, 10);

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_nonce(ADDR_A), 10);
}

TEST(MemoryAccountProviderNonce, SetNonceToZero) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, 100);
  provider.set_nonce(ADDR_A, 0);

  EXPECT_EQ(provider.get_nonce(ADDR_A), 0);
}

TEST(MemoryAccountProviderNonce, SetMaxNonce) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, MAX64);

  EXPECT_EQ(provider.get_nonce(ADDR_A), MAX64);
}

TEST(MemoryAccountProviderNonce, OverwriteNonce) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, 10);
  provider.set_nonce(ADDR_A, 99);

  EXPECT_EQ(provider.get_nonce(ADDR_A), 99);
}

TEST(MemoryAccountProviderNonce, IncrementNonce) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, 5);

  const uint64_t new_nonce = provider.increment_nonce(ADDR_A);

  EXPECT_EQ(new_nonce, 6);
  EXPECT_EQ(provider.get_nonce(ADDR_A), 6);
}

TEST(MemoryAccountProviderNonce, IncrementNonceCreatesAccount) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.exists(ADDR_A));

  const uint64_t new_nonce = provider.increment_nonce(ADDR_A);

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(new_nonce, 1);
  EXPECT_EQ(provider.get_nonce(ADDR_A), 1);
}

TEST(MemoryAccountProviderNonce, MultipleIncrements) {
  MemoryAccountProvider provider;

  provider.create_account(ADDR_A);

  EXPECT_EQ(provider.increment_nonce(ADDR_A), 1);
  EXPECT_EQ(provider.increment_nonce(ADDR_A), 2);
  EXPECT_EQ(provider.increment_nonce(ADDR_A), 3);
  EXPECT_EQ(provider.increment_nonce(ADDR_A), 4);

  EXPECT_EQ(provider.get_nonce(ADDR_A), 4);
}

TEST(MemoryAccountProviderNonce, IndependentNonces) {
  MemoryAccountProvider provider;

  provider.set_nonce(ADDR_A, 10);
  provider.set_nonce(ADDR_B, 20);
  provider.set_nonce(ADDR_C, 30);

  EXPECT_EQ(provider.get_nonce(ADDR_A), 10);
  EXPECT_EQ(provider.get_nonce(ADDR_B), 20);
  EXPECT_EQ(provider.get_nonce(ADDR_C), 30);
}

// =============================================================================
// Code Hash Operations: set_code_hash(), get_code_hash()
// =============================================================================

TEST(MemoryAccountProviderCodeHash, SetAndGetCodeHash) {
  MemoryAccountProvider provider;

  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  EXPECT_EQ(provider.get_code_hash(ADDR_A), NON_EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderCodeHash, SetCodeHashCreatesAccount) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.exists(ADDR_A));

  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  EXPECT_TRUE(provider.exists(ADDR_A));
}

TEST(MemoryAccountProviderCodeHash, SetToEmptyCodeHash) {
  MemoryAccountProvider provider;

  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);
  provider.set_code_hash(ADDR_A, ethereum::EMPTY_CODE_HASH);

  EXPECT_EQ(provider.get_code_hash(ADDR_A), ethereum::EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderCodeHash, OverwriteCodeHash) {
  MemoryAccountProvider provider;

  // NOLINTBEGIN(bugprone-unchecked-optional-access)
  const Hash another_hash =
      hex::decode_hash("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef")
          .value();
  // NOLINTEND(bugprone-unchecked-optional-access)

  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);
  provider.set_code_hash(ADDR_A, another_hash);

  EXPECT_EQ(provider.get_code_hash(ADDR_A), another_hash);
}

// =============================================================================
// is_empty: Empty Account Checking
// =============================================================================

TEST(MemoryAccountProviderIsEmpty, NonExistentIsEmpty) {
  MemoryAccountProvider provider;
  EXPECT_TRUE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, NewlyCreatedIsEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  EXPECT_TRUE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, NonZeroBalanceNotEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(1));

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, NonZeroNonceNotEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_nonce(ADDR_A, 1);

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, NonEmptyCodeHashNotEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, AllFieldsNonZeroNotEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_nonce(ADDR_A, 5);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, ResetToEmptyBecomesEmpty) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_nonce(ADDR_A, 5);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  // Reset all fields
  provider.set_balance(ADDR_A, Uint256::zero());
  provider.set_nonce(ADDR_A, 0);
  provider.set_code_hash(ADDR_A, ethereum::EMPTY_CODE_HASH);

  EXPECT_TRUE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, OnlyBalanceZeroNotEnough) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256::zero());
  provider.set_nonce(ADDR_A, 1);

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderIsEmpty, OnlyNonceZeroNotEnough) {
  MemoryAccountProvider provider;
  provider.create_account(ADDR_A);
  provider.set_balance(ADDR_A, Uint256(1));
  provider.set_nonce(ADDR_A, 0);

  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

// =============================================================================
// EIP-2929 Warm/Cold Address Tracking
// =============================================================================

TEST(MemoryAccountProviderWarm, InitiallyAllCold) {
  MemoryAccountProvider provider;

  EXPECT_FALSE(provider.is_warm(ADDR_A));
  EXPECT_FALSE(provider.is_warm(ADDR_B));
  EXPECT_FALSE(provider.is_warm(ADDR_C));
}

TEST(MemoryAccountProviderWarm, WarmAddressReturnsTrue) {
  MemoryAccountProvider provider;

  // First warm returns true (was cold)
  EXPECT_TRUE(provider.warm_address(ADDR_A));
  EXPECT_TRUE(provider.is_warm(ADDR_A));
}

TEST(MemoryAccountProviderWarm, WarmAddressSecondCallReturnsFalse) {
  MemoryAccountProvider provider;

  EXPECT_TRUE(provider.warm_address(ADDR_A));   // First call - was cold
  EXPECT_FALSE(provider.warm_address(ADDR_A));  // Second call - was warm
}

TEST(MemoryAccountProviderWarm, WarmMultipleAddresses) {
  MemoryAccountProvider provider;

  EXPECT_TRUE(provider.warm_address(ADDR_A));
  EXPECT_TRUE(provider.warm_address(ADDR_B));
  EXPECT_TRUE(provider.warm_address(ADDR_C));

  EXPECT_TRUE(provider.is_warm(ADDR_A));
  EXPECT_TRUE(provider.is_warm(ADDR_B));
  EXPECT_TRUE(provider.is_warm(ADDR_C));
}

TEST(MemoryAccountProviderWarm, WarmDoesNotAffectOtherAddresses) {
  MemoryAccountProvider provider;

  provider.warm_address(ADDR_A);

  EXPECT_TRUE(provider.is_warm(ADDR_A));
  EXPECT_FALSE(provider.is_warm(ADDR_B));
  EXPECT_FALSE(provider.is_warm(ADDR_C));
}

TEST(MemoryAccountProviderWarm, WarmIndependentOfAccountExistence) {
  MemoryAccountProvider provider;

  // Can warm an address without creating account
  EXPECT_TRUE(provider.warm_address(ADDR_A));
  EXPECT_TRUE(provider.is_warm(ADDR_A));
  EXPECT_FALSE(provider.exists(ADDR_A));
}

// =============================================================================
// Transaction Boundary: begin_transaction()
// =============================================================================

TEST(MemoryAccountProviderTransaction, BeginTransactionClearsWarmAddresses) {
  MemoryAccountProvider provider;

  provider.warm_address(ADDR_A);
  provider.warm_address(ADDR_B);
  EXPECT_TRUE(provider.is_warm(ADDR_A));
  EXPECT_TRUE(provider.is_warm(ADDR_B));

  provider.begin_transaction();

  EXPECT_FALSE(provider.is_warm(ADDR_A));
  EXPECT_FALSE(provider.is_warm(ADDR_B));
}

TEST(MemoryAccountProviderTransaction, BeginTransactionPreservesAccounts) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_nonce(ADDR_A, 5);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  provider.begin_transaction();

  EXPECT_TRUE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
  EXPECT_EQ(provider.get_nonce(ADDR_A), 5);
  EXPECT_EQ(provider.get_code_hash(ADDR_A), NON_EMPTY_CODE_HASH);
}

TEST(MemoryAccountProviderTransaction, MultipleTransactionBoundaries) {
  MemoryAccountProvider provider;

  // Transaction 1
  provider.warm_address(ADDR_A);
  EXPECT_TRUE(provider.is_warm(ADDR_A));

  provider.begin_transaction();
  EXPECT_FALSE(provider.is_warm(ADDR_A));

  // Transaction 2
  provider.warm_address(ADDR_B);
  EXPECT_TRUE(provider.is_warm(ADDR_B));

  provider.begin_transaction();
  EXPECT_FALSE(provider.is_warm(ADDR_B));
}

// =============================================================================
// accounts(): State Access
// =============================================================================

TEST(MemoryAccountProviderAccounts, EmptyProviderReturnsEmptyMap) {
  const MemoryAccountProvider provider;

  const auto& accounts = provider.accounts();
  EXPECT_TRUE(accounts.empty());
}

TEST(MemoryAccountProviderAccounts, ReturnsAllCreatedAccounts) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_B, Uint256(200));

  const auto& accounts = provider.accounts();
  EXPECT_EQ(accounts.size(), 2);
  EXPECT_TRUE(accounts.contains(ADDR_A));
  EXPECT_TRUE(accounts.contains(ADDR_B));
}

TEST(MemoryAccountProviderAccounts, DeletedAccountsNotIncluded) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_B, Uint256(200));
  provider.delete_account(ADDR_A);

  const auto& accounts = provider.accounts();
  EXPECT_EQ(accounts.size(), 1);
  EXPECT_FALSE(accounts.contains(ADDR_A));
  EXPECT_TRUE(accounts.contains(ADDR_B));
}

// =============================================================================
// Edge Cases and Integration
// =============================================================================

TEST(MemoryAccountProviderEdge, TransferBetweenAccounts) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(1000));
  provider.set_balance(ADDR_B, Uint256(500));

  const Uint256 transfer_amount(300);

  // Simulate transfer: subtract from A, add to B
  EXPECT_TRUE(provider.subtract_balance(ADDR_A, transfer_amount));
  EXPECT_TRUE(provider.add_balance(ADDR_B, transfer_amount));

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(700));
  EXPECT_EQ(provider.get_balance(ADDR_B), Uint256(800));
}

TEST(MemoryAccountProviderEdge, FailedTransferOnInsufficientBalance) {
  MemoryAccountProvider provider;

  provider.set_balance(ADDR_A, Uint256(100));
  provider.set_balance(ADDR_B, Uint256(500));

  // Try to transfer more than available
  EXPECT_FALSE(provider.subtract_balance(ADDR_A, Uint256(200)));

  // Both balances should be unchanged
  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(100));
  EXPECT_EQ(provider.get_balance(ADDR_B), Uint256(500));
}

TEST(MemoryAccountProviderEdge, AccountStateAfterMultipleOperations) {
  MemoryAccountProvider provider;

  // Complex sequence of operations
  provider.create_account(ADDR_A);
  EXPECT_TRUE(provider.add_balance(ADDR_A, Uint256(1000)));
  provider.increment_nonce(ADDR_A);
  provider.increment_nonce(ADDR_A);
  EXPECT_TRUE(provider.subtract_balance(ADDR_A, Uint256(100)));
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  EXPECT_EQ(provider.get_balance(ADDR_A), Uint256(900));
  EXPECT_EQ(provider.get_nonce(ADDR_A), 2);
  EXPECT_EQ(provider.get_code_hash(ADDR_A), NON_EMPTY_CODE_HASH);
  EXPECT_FALSE(provider.is_empty(ADDR_A));
}

TEST(MemoryAccountProviderEdge, SELFDESTRUCTSimulation) {
  MemoryAccountProvider provider;

  // Setup source account with balance
  provider.set_balance(ADDR_A, Uint256(1000));
  provider.set_nonce(ADDR_A, 5);
  provider.set_code_hash(ADDR_A, NON_EMPTY_CODE_HASH);

  // Transfer balance to beneficiary
  const Uint256 balance = provider.get_balance(ADDR_A);
  EXPECT_TRUE(provider.add_balance(ADDR_B, balance));

  // Delete the self-destructed account
  provider.delete_account(ADDR_A);

  EXPECT_FALSE(provider.exists(ADDR_A));
  EXPECT_EQ(provider.get_balance(ADDR_B), Uint256(1000));
}

TEST(MemoryAccountProviderEdge, LargeNumberOfAccounts) {
  MemoryAccountProvider provider;

  // Create many accounts
  for (uint64_t i = 1; i <= 100; ++i) {
    std::array<uint8_t, 20> addr_bytes{};
    addr_bytes[19] = static_cast<uint8_t>(i);
    addr_bytes[18] = static_cast<uint8_t>(i >> 8);
    const Address addr(addr_bytes);

    provider.set_balance(addr, Uint256(i * 100));
    provider.set_nonce(addr, i);
  }

  EXPECT_EQ(provider.accounts().size(), 100);
}

}  // namespace
}  // namespace div0::state
