#include "div0/div0.h"

#include "unity.h"

// Test headers - types
#include "types/test_address.h"
#include "types/test_bytes.h"
#include "types/test_bytes32.h"
#include "types/test_hash.h"
#include "types/test_uint256.h"

// Test headers - mem
#include "mem/test_arena.h"

// Test headers - util
#include "util/test_hex.h"

// Test headers - evm
#include "evm/test_evm.h"
#include "evm/test_stack.h"
#include "evm/test_stack_pool.h"

// Test headers - crypto
#include "crypto/test_keccak256.h"
#include "crypto/test_secp256k1.h"

// Test headers - rlp
#include "rlp/test_rlp.h"

// Test headers - trie
#include "trie/test_hex_prefix.h"
#include "trie/test_mpt.h"
#include "trie/test_nibbles.h"
#include "trie/test_node.h"

// Test headers - state
#include "state/test_account.h"
#include "state/test_world_state.h"

// Test headers - ethereum
#include "ethereum/transaction/test_transaction.h"

// Global arena for tests (shared across test files)
div0_arena_t test_arena;
static bool arena_initialized = false;

void setUp(void) {
  // Initialize arena once, reset between tests
  if (!arena_initialized) {
    TEST_ASSERT_TRUE(div0_arena_init(&test_arena));
    arena_initialized = true;
  }
}

void tearDown(void) {
  // Reset arena after each test (keeps blocks for reuse)
  div0_arena_reset(&test_arena);
}

int main(void) {
  UNITY_BEGIN();

  // uint256 tests
  RUN_TEST(test_uint256_zero_is_zero);
  RUN_TEST(test_uint256_from_u64_works);
  RUN_TEST(test_uint256_eq_works);
  RUN_TEST(test_uint256_add_no_overflow);
  RUN_TEST(test_uint256_add_with_carry);
  RUN_TEST(test_uint256_add_overflow_wraps);
  RUN_TEST(test_uint256_bytes_be_roundtrip);
  RUN_TEST(test_uint256_from_bytes_be_short);
  RUN_TEST(test_uint256_sub_basic);
  RUN_TEST(test_uint256_sub_with_borrow);
  RUN_TEST(test_uint256_sub_underflow_wraps);
  RUN_TEST(test_uint256_lt_basic);
  RUN_TEST(test_uint256_lt_equal);
  RUN_TEST(test_uint256_lt_multi_limb);
  RUN_TEST(test_uint256_mul_basic);
  RUN_TEST(test_uint256_mul_limb_boundary);
  RUN_TEST(test_uint256_mul_multi_limb);
  RUN_TEST(test_uint256_mul_overflow_wraps);
  RUN_TEST(test_uint256_div_basic);
  RUN_TEST(test_uint256_div_by_zero);
  RUN_TEST(test_uint256_div_smaller_than_divisor);
  RUN_TEST(test_uint256_div_with_remainder);
  RUN_TEST(test_uint256_div_multi_limb);
  RUN_TEST(test_uint256_mod_basic);
  RUN_TEST(test_uint256_mod_by_zero);
  RUN_TEST(test_uint256_mod_by_one);
  RUN_TEST(test_uint256_mod_no_remainder);
  RUN_TEST(test_uint256_div_mod_consistency);

  // bytes32 tests
  RUN_TEST(test_bytes32_zero_is_zero);
  RUN_TEST(test_bytes32_from_bytes_works);
  RUN_TEST(test_bytes32_from_bytes_padded_short);
  RUN_TEST(test_bytes32_from_bytes_padded_long);
  RUN_TEST(test_bytes32_equal_works);
  RUN_TEST(test_bytes32_to_uint256_roundtrip);

  // hash tests
  RUN_TEST(test_hash_zero_is_zero);
  RUN_TEST(test_hash_from_bytes_works);
  RUN_TEST(test_hash_equal_works);
  RUN_TEST(test_hash_to_uint256_roundtrip);
  RUN_TEST(test_hash_alignment);

  // address tests
  RUN_TEST(test_address_zero_is_zero);
  RUN_TEST(test_address_from_bytes_works);
  RUN_TEST(test_address_equal_works);
  RUN_TEST(test_address_to_uint256_roundtrip);
  RUN_TEST(test_address_from_uint256_truncates);

  // bytes tests
  RUN_TEST(test_bytes_init_empty);
  RUN_TEST(test_bytes_from_data_works);
  RUN_TEST(test_bytes_append_works);
  RUN_TEST(test_bytes_append_byte_works);
  RUN_TEST(test_bytes_clear_works);
  RUN_TEST(test_bytes_free_works);
  RUN_TEST(test_bytes_arena_backed);
  RUN_TEST(test_bytes_arena_no_realloc);

  // arena tests
  RUN_TEST(test_arena_alloc_basic);
  RUN_TEST(test_arena_alloc_aligned);
  RUN_TEST(test_arena_realloc);
  RUN_TEST(test_arena_reset);

  // hex utility tests
  RUN_TEST(test_hex_char_to_nibble_digits);
  RUN_TEST(test_hex_char_to_nibble_lowercase);
  RUN_TEST(test_hex_char_to_nibble_uppercase);
  RUN_TEST(test_hex_char_to_nibble_invalid);
  RUN_TEST(test_hex_decode_basic);
  RUN_TEST(test_hex_decode_with_prefix);
  RUN_TEST(test_hex_decode_uppercase);
  RUN_TEST(test_hex_decode_mixed_case);
  RUN_TEST(test_hex_decode_null_input);
  RUN_TEST(test_hex_decode_null_output);
  RUN_TEST(test_hex_decode_wrong_length);
  RUN_TEST(test_hex_decode_invalid_char);

  // stack tests
  RUN_TEST(test_stack_init_is_empty);
  RUN_TEST(test_stack_push_pop);
  RUN_TEST(test_stack_lifo_order);
  RUN_TEST(test_stack_peek);
  RUN_TEST(test_stack_has_space_overflow);
  RUN_TEST(test_stack_has_items);
  RUN_TEST(test_stack_clear);
  RUN_TEST(test_stack_growth);
  RUN_TEST(test_stack_dup);
  RUN_TEST(test_stack_swap);

  // stack pool tests
  RUN_TEST(test_stack_pool_init);
  RUN_TEST(test_stack_pool_borrow);
  RUN_TEST(test_stack_pool_multiple_borrows);

  // evm tests
  RUN_TEST(test_evm_stop);
  RUN_TEST(test_evm_empty_code);
  RUN_TEST(test_evm_push1);
  RUN_TEST(test_evm_push32);
  RUN_TEST(test_evm_add);
  RUN_TEST(test_evm_add_multiple);
  RUN_TEST(test_evm_invalid_opcode);
  RUN_TEST(test_evm_stack_underflow);
  RUN_TEST(test_evm_mstore);
  RUN_TEST(test_evm_mstore8);
  RUN_TEST(test_evm_return_empty);
  RUN_TEST(test_evm_return_with_data);
  RUN_TEST(test_evm_revert_empty);
  RUN_TEST(test_evm_revert_with_data);
  RUN_TEST(test_evm_call_without_state);

  // keccak256 tests
  RUN_TEST(test_keccak256_empty);
  RUN_TEST(test_keccak256_single_zero);
  RUN_TEST(test_keccak256_five_zeros);
  RUN_TEST(test_keccak256_ten_zeros);
  RUN_TEST(test_keccak256_32_zeros);
  RUN_TEST(test_keccak256_hasher_empty);
  RUN_TEST(test_keccak256_hasher_single_update);
  RUN_TEST(test_keccak256_hasher_multiple_updates);
  RUN_TEST(test_keccak256_hasher_byte_by_byte);
  RUN_TEST(test_keccak256_hasher_reuse);
  RUN_TEST(test_keccak256_hasher_reset);
  RUN_TEST(test_keccak256_one_shot_matches_incremental);
  RUN_TEST(test_keccak256_deterministic);
  RUN_TEST(test_keccak256_avalanche);

  // secp256k1 tests
  RUN_TEST(test_secp256k1_ctx_create_destroy);
  RUN_TEST(test_secp256k1_ecrecover_reth_vector);
  RUN_TEST(test_secp256k1_ecrecover_v28);
  RUN_TEST(test_secp256k1_ecrecover_eip155);
  RUN_TEST(test_secp256k1_ecrecover_eip155_wrong_chain_id);
  RUN_TEST(test_secp256k1_ecrecover_invalid_v);
  RUN_TEST(test_secp256k1_ecrecover_zero_signature);
  RUN_TEST(test_secp256k1_recover_pubkey_invalid_recovery_id);

  // RLP encoding tests
  RUN_TEST(test_rlp_encode_empty_string);
  RUN_TEST(test_rlp_encode_single_byte_00);
  RUN_TEST(test_rlp_encode_single_byte_7f);
  RUN_TEST(test_rlp_encode_short_string_dog);
  RUN_TEST(test_rlp_encode_short_string_55_bytes);
  RUN_TEST(test_rlp_encode_long_string_56_bytes);
  RUN_TEST(test_rlp_encode_u64_zero);
  RUN_TEST(test_rlp_encode_u64_small);
  RUN_TEST(test_rlp_encode_u64_medium);
  RUN_TEST(test_rlp_encode_uint256_zero);
  RUN_TEST(test_rlp_encode_uint256_single_byte);
  RUN_TEST(test_rlp_encode_uint256_multi_byte);
  RUN_TEST(test_rlp_encode_address);
  RUN_TEST(test_rlp_encode_empty_list);
  RUN_TEST(test_rlp_encode_nested_list);

  // RLP decoding tests
  RUN_TEST(test_rlp_decode_empty_string);
  RUN_TEST(test_rlp_decode_single_byte_00);
  RUN_TEST(test_rlp_decode_single_byte_7f);
  RUN_TEST(test_rlp_decode_short_string_dog);
  RUN_TEST(test_rlp_decode_u64_zero);
  RUN_TEST(test_rlp_decode_u64_small);
  RUN_TEST(test_rlp_decode_u64_medium);
  RUN_TEST(test_rlp_decode_uint256_zero);
  RUN_TEST(test_rlp_decode_uint256_big);
  RUN_TEST(test_rlp_decode_address_valid);
  RUN_TEST(test_rlp_decode_address_wrong_size);
  RUN_TEST(test_rlp_decode_empty_list);
  RUN_TEST(test_rlp_decode_list_items);

  // RLP decoding error tests
  RUN_TEST(test_rlp_decode_error_input_too_short);
  RUN_TEST(test_rlp_decode_error_leading_zeros);
  RUN_TEST(test_rlp_decode_error_non_canonical);

  // RLP roundtrip tests
  RUN_TEST(test_rlp_roundtrip_bytes);
  RUN_TEST(test_rlp_roundtrip_u64);
  RUN_TEST(test_rlp_roundtrip_uint256);

  // RLP helper tests
  RUN_TEST(test_rlp_prefix_length);
  RUN_TEST(test_rlp_length_of_length);
  RUN_TEST(test_rlp_byte_length_u64);
  RUN_TEST(test_rlp_is_string_prefix);
  RUN_TEST(test_rlp_is_list_prefix);

  // Nibbles tests
  RUN_TEST(test_nibbles_from_bytes_empty);
  RUN_TEST(test_nibbles_from_bytes_single);
  RUN_TEST(test_nibbles_from_bytes_multiple);
  RUN_TEST(test_nibbles_to_bytes_empty);
  RUN_TEST(test_nibbles_to_bytes_single);
  RUN_TEST(test_nibbles_to_bytes_multiple);
  RUN_TEST(test_nibbles_to_bytes_alloc_works);
  RUN_TEST(test_nibbles_common_prefix_none);
  RUN_TEST(test_nibbles_common_prefix_partial);
  RUN_TEST(test_nibbles_common_prefix_full);
  RUN_TEST(test_nibbles_common_prefix_different_lengths);
  RUN_TEST(test_nibbles_slice_full);
  RUN_TEST(test_nibbles_slice_partial);
  RUN_TEST(test_nibbles_slice_view);
  RUN_TEST(test_nibbles_slice_empty);
  RUN_TEST(test_nibbles_slice_out_of_bounds);
  RUN_TEST(test_nibbles_copy_works);
  RUN_TEST(test_nibbles_copy_empty);
  RUN_TEST(test_nibbles_cmp_equal);
  RUN_TEST(test_nibbles_cmp_less);
  RUN_TEST(test_nibbles_cmp_greater);
  RUN_TEST(test_nibbles_cmp_prefix);
  RUN_TEST(test_nibbles_equal_works);
  RUN_TEST(test_nibbles_is_empty);
  RUN_TEST(test_nibbles_get);
  RUN_TEST(test_nibbles_roundtrip);

  // Hex-prefix tests
  RUN_TEST(test_hex_prefix_encode_odd_extension);
  RUN_TEST(test_hex_prefix_encode_even_extension);
  RUN_TEST(test_hex_prefix_encode_odd_leaf);
  RUN_TEST(test_hex_prefix_encode_even_leaf);
  RUN_TEST(test_hex_prefix_encode_empty);
  RUN_TEST(test_hex_prefix_encode_single_nibble);
  RUN_TEST(test_hex_prefix_decode_odd_extension);
  RUN_TEST(test_hex_prefix_decode_even_extension);
  RUN_TEST(test_hex_prefix_decode_odd_leaf);
  RUN_TEST(test_hex_prefix_decode_even_leaf);
  RUN_TEST(test_hex_prefix_decode_empty);
  RUN_TEST(test_hex_prefix_decode_null_input);
  RUN_TEST(test_hex_prefix_roundtrip_various);

  // Node tests
  RUN_TEST(test_mpt_node_empty);
  RUN_TEST(test_mpt_node_leaf);
  RUN_TEST(test_mpt_node_extension);
  RUN_TEST(test_mpt_node_branch);
  RUN_TEST(test_node_ref_null);
  RUN_TEST(test_node_ref_is_null);
  RUN_TEST(test_mpt_node_encode_empty);
  RUN_TEST(test_mpt_node_encode_leaf);
  RUN_TEST(test_mpt_node_encode_extension);
  RUN_TEST(test_mpt_node_encode_branch_empty);
  RUN_TEST(test_mpt_node_hash_empty);
  RUN_TEST(test_mpt_node_hash_leaf);
  RUN_TEST(test_mpt_node_hash_caching);
  RUN_TEST(test_mpt_node_ref_small_embeds);
  RUN_TEST(test_mpt_node_ref_large_hashes);
  RUN_TEST(test_mpt_branch_child_count);
  RUN_TEST(test_mpt_empty_root_constant);

  // MPT tests
  RUN_TEST(test_mpt_init);
  RUN_TEST(test_mpt_empty_is_empty);
  RUN_TEST(test_mpt_empty_root_hash);
  RUN_TEST(test_mpt_insert_single);
  RUN_TEST(test_mpt_insert_overwrite);
  RUN_TEST(test_mpt_insert_two_different_keys);
  RUN_TEST(test_mpt_insert_common_prefix);
  RUN_TEST(test_mpt_insert_branch_creation);
  RUN_TEST(test_mpt_get_not_found);
  RUN_TEST(test_mpt_get_after_insert);
  RUN_TEST(test_mpt_contains_works);
  RUN_TEST(test_mpt_root_hash_single_entry);
  RUN_TEST(test_mpt_root_hash_deterministic);
  RUN_TEST(test_mpt_root_hash_order_independent);
  RUN_TEST(test_mpt_memory_backend_create);
  RUN_TEST(test_mpt_memory_backend_alloc_node);

  // Ethereum test vectors
  RUN_TEST(test_mpt_ethereum_vector_dogs);
  RUN_TEST(test_mpt_ethereum_vector_puppy);
  RUN_TEST(test_mpt_ethereum_vector_foo);
  RUN_TEST(test_mpt_ethereum_vector_small_values);
  RUN_TEST(test_mpt_ethereum_vector_testy);
  RUN_TEST(test_mpt_ethereum_vector_hex);
  RUN_TEST(test_mpt_ethereum_vector_insert_middle_leaf);
  RUN_TEST(test_mpt_ethereum_vector_branch_value_update);

  // MPT edge case tests
  RUN_TEST(test_mpt_empty_value);
  RUN_TEST(test_mpt_contains_empty_value);
  RUN_TEST(test_mpt_long_key);
  RUN_TEST(test_mpt_binary_keys);
  RUN_TEST(test_mpt_shared_prefix_keys);
  RUN_TEST(test_mpt_many_keys);

  // MPT delete tests
  RUN_TEST(test_mpt_delete_single);
  RUN_TEST(test_mpt_delete_not_found);
  RUN_TEST(test_mpt_delete_from_branch);
  RUN_TEST(test_mpt_delete_collapses_branch);
  RUN_TEST(test_mpt_delete_and_reinsert);

  // Account tests
  RUN_TEST(test_account_empty_creation);
  RUN_TEST(test_account_empty_has_correct_defaults);
  RUN_TEST(test_account_is_empty_checks_correctly);
  RUN_TEST(test_account_rlp_encode_empty);
  RUN_TEST(test_account_rlp_encode_with_balance);
  RUN_TEST(test_account_rlp_encode_with_nonce);
  RUN_TEST(test_account_rlp_encode_full_account);
  RUN_TEST(test_account_rlp_decode_empty);
  RUN_TEST(test_account_rlp_decode_with_balance);
  RUN_TEST(test_account_rlp_roundtrip);
  RUN_TEST(test_account_rlp_decode_invalid_returns_false);
  RUN_TEST(test_empty_code_hash_constant);

  // World state tests
  RUN_TEST(test_world_state_create);
  RUN_TEST(test_world_state_empty_root);
  RUN_TEST(test_world_state_get_nonexistent_account);
  RUN_TEST(test_world_state_set_and_get_account);
  RUN_TEST(test_world_state_delete_empty_account);
  RUN_TEST(test_world_state_balance_operations);
  RUN_TEST(test_world_state_add_balance);
  RUN_TEST(test_world_state_sub_balance);
  RUN_TEST(test_world_state_nonce_operations);
  RUN_TEST(test_world_state_increment_nonce);
  RUN_TEST(test_world_state_code_operations);
  RUN_TEST(test_world_state_code_hash);
  RUN_TEST(test_world_state_storage_operations);
  RUN_TEST(test_world_state_storage_multiple_slots);
  RUN_TEST(test_world_state_warm_address);
  RUN_TEST(test_world_state_warm_slot);
  RUN_TEST(test_world_state_root_changes);
  RUN_TEST(test_world_state_access_interface);
  RUN_TEST(test_world_state_begin_transaction);
  RUN_TEST(test_world_state_get_original_storage);
  RUN_TEST(test_world_state_original_storage_unset_slot);
  RUN_TEST(test_world_state_multi_account_storage_isolation);
  RUN_TEST(test_world_state_multi_account_balance_isolation);
  RUN_TEST(test_world_state_large_balance);
  RUN_TEST(test_world_state_large_storage_key);
  RUN_TEST(test_world_state_large_storage_value);
  RUN_TEST(test_world_state_add_balance_overflow);
  RUN_TEST(test_world_state_clear);
  RUN_TEST(test_world_state_account_is_empty_interface);

  // Transaction tests
  RUN_TEST(test_transaction_type_enum);
  RUN_TEST(test_transaction_init_default);
  RUN_TEST(test_legacy_tx_decode_basic);
  RUN_TEST(test_legacy_tx_decode_contract_creation);
  RUN_TEST(test_legacy_tx_chain_id_eip155);
  RUN_TEST(test_legacy_tx_chain_id_pre_eip155);
  RUN_TEST(test_eip1559_tx_decode_basic);
  RUN_TEST(test_eip1559_tx_effective_gas_price);
  RUN_TEST(test_transaction_accessors);
  RUN_TEST(test_legacy_tx_signing_hash);
  RUN_TEST(test_eip1559_tx_signing_hash);
  RUN_TEST(test_transaction_recover_sender_legacy);

  // Real test vectors from Ethereum tests
  RUN_TEST(test_real_vector_legacy_pre_eip155);
  RUN_TEST(test_real_vector_legacy_eip155);
  RUN_TEST(test_real_vector_legacy_vitalik_2);
  RUN_TEST(test_real_vector_eip2930);

  // Negative tests (malformed input)
  RUN_TEST(test_decode_empty_input);
  RUN_TEST(test_decode_invalid_type_byte);
  RUN_TEST(test_decode_truncated_legacy);
  RUN_TEST(test_decode_truncated_typed);
  RUN_TEST(test_decode_not_a_list);
  RUN_TEST(test_decode_missing_fields);

  // Encode/decode roundtrip tests
  RUN_TEST(test_roundtrip_legacy_tx);
  RUN_TEST(test_roundtrip_eip2930_tx);
  RUN_TEST(test_roundtrip_constructed_legacy);
  RUN_TEST(test_roundtrip_constructed_eip1559);

  // Cleanup
  div0_arena_destroy(&test_arena);

  return UNITY_END();
}