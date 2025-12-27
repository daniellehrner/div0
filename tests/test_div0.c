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

  // Cleanup
  div0_arena_destroy(&test_arena);

  return UNITY_END();
}