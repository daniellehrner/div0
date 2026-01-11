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
#include "evm/test_opcodes_arithmetic.h"
#include "evm/test_opcodes_bitwise.h"
#include "evm/test_opcodes_comparison.h"
#include "evm/test_opcodes_context.h"
#include "evm/test_opcodes_logging.h"
#include "evm/test_opcodes_stack.h"
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

// Test headers - executor
#include "executor/test_block_executor.h"

// Test headers - JSON and t8n (hosted only)
#ifndef DIV0_FREESTANDING
#include "json/test_json.h"
#include "t8n/test_t8n.h"
#endif

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

  // Signed arithmetic tests
  RUN_TEST(test_uint256_is_negative_zero);
  RUN_TEST(test_uint256_is_negative_positive);
  RUN_TEST(test_uint256_is_negative_negative);
  RUN_TEST(test_uint256_sdiv_by_zero);
  RUN_TEST(test_uint256_sdiv_positive_by_positive);
  RUN_TEST(test_uint256_sdiv_negative_by_positive);
  RUN_TEST(test_uint256_sdiv_positive_by_negative);
  RUN_TEST(test_uint256_sdiv_negative_by_negative);
  RUN_TEST(test_uint256_sdiv_min_value_by_minus_one);
  RUN_TEST(test_uint256_smod_by_zero);
  RUN_TEST(test_uint256_smod_positive_by_positive);
  RUN_TEST(test_uint256_smod_negative_by_positive);
  RUN_TEST(test_uint256_smod_positive_by_negative);
  RUN_TEST(test_uint256_smod_negative_by_negative);
  RUN_TEST(test_uint256_sdiv_smod_identity);

  // Sign extend tests
  RUN_TEST(test_uint256_signextend_byte_pos_zero_positive);
  RUN_TEST(test_uint256_signextend_byte_pos_zero_negative);
  RUN_TEST(test_uint256_signextend_byte_pos_one_positive);
  RUN_TEST(test_uint256_signextend_byte_pos_one_negative);
  RUN_TEST(test_uint256_signextend_byte_pos_31_or_larger);
  RUN_TEST(test_uint256_signextend_clears_high_bits);

  // Modular arithmetic tests
  RUN_TEST(test_uint256_addmod_by_zero);
  RUN_TEST(test_uint256_addmod_no_overflow);
  RUN_TEST(test_uint256_addmod_with_overflow);
  RUN_TEST(test_uint256_addmod_result_equals_modulus);
  RUN_TEST(test_uint256_addmod_modulus_one);
  RUN_TEST(test_uint256_mulmod_by_zero);
  RUN_TEST(test_uint256_mulmod_no_overflow);
  RUN_TEST(test_uint256_mulmod_with_overflow);
  RUN_TEST(test_uint256_mulmod_modulus_one);

  // Exponentiation tests
  RUN_TEST(test_uint256_exp_exponent_zero);
  RUN_TEST(test_uint256_exp_base_zero);
  RUN_TEST(test_uint256_exp_base_one);
  RUN_TEST(test_uint256_exp_exponent_one);
  RUN_TEST(test_uint256_exp_small_powers);
  RUN_TEST(test_uint256_exp_powers_of_two);
  RUN_TEST(test_uint256_exp_overflow);

  // Byte length tests
  RUN_TEST(test_uint256_byte_length_zero);
  RUN_TEST(test_uint256_byte_length_small_values);
  RUN_TEST(test_uint256_byte_length_limb_boundaries);

  // Bitwise operation tests
  RUN_TEST(test_uint256_and_basic);
  RUN_TEST(test_uint256_or_basic);
  RUN_TEST(test_uint256_xor_basic);
  RUN_TEST(test_uint256_not_basic);

  // Byte extraction tests
  RUN_TEST(test_uint256_byte_index_zero);
  RUN_TEST(test_uint256_byte_index_31);
  RUN_TEST(test_uint256_byte_index_out_of_range);

  // Shift operation tests
  RUN_TEST(test_uint256_shl_by_zero);
  RUN_TEST(test_uint256_shl_by_small);
  RUN_TEST(test_uint256_shl_cross_limb);
  RUN_TEST(test_uint256_shl_by_256);
  RUN_TEST(test_uint256_shr_by_zero);
  RUN_TEST(test_uint256_shr_by_small);
  RUN_TEST(test_uint256_shr_cross_limb);
  RUN_TEST(test_uint256_shr_by_256);
  RUN_TEST(test_uint256_sar_positive);
  RUN_TEST(test_uint256_sar_negative);
  RUN_TEST(test_uint256_sar_negative_large_shift);

  // Signed comparison tests
  RUN_TEST(test_uint256_slt_both_positive);
  RUN_TEST(test_uint256_slt_both_negative);
  RUN_TEST(test_uint256_slt_mixed_signs);
  RUN_TEST(test_uint256_sgt_basic);

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
  RUN_TEST(test_arena_alloc_large);
  RUN_TEST(test_arena_alloc_large_alignment);
  RUN_TEST(test_arena_alloc_large_freed_on_reset);
  RUN_TEST(test_arena_alloc_large_multiple);

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
  RUN_TEST(test_evm_mload);
  RUN_TEST(test_evm_mload_mstore_roundtrip);
  RUN_TEST(test_evm_keccak256_empty);
  RUN_TEST(test_evm_keccak256_single_byte);
  RUN_TEST(test_evm_keccak256_32_bytes);
  RUN_TEST(test_evm_return_empty);
  RUN_TEST(test_evm_return_with_data);
  RUN_TEST(test_evm_revert_empty);
  RUN_TEST(test_evm_revert_with_data);
  RUN_TEST(test_evm_call_without_state);
  RUN_TEST(test_evm_sload_empty_slot);
  RUN_TEST(test_evm_sstore_and_sload);
  RUN_TEST(test_evm_sstore_multiple_slots);
  RUN_TEST(test_evm_sload_gas_cold);
  RUN_TEST(test_evm_sload_gas_warm);
  RUN_TEST(test_evm_sstore_without_state);
  RUN_TEST(test_evm_init_shanghai);
  RUN_TEST(test_evm_init_cancun);
  RUN_TEST(test_evm_init_prague);
  RUN_TEST(test_evm_gas_refund_initialized);
  RUN_TEST(test_evm_gas_refund_reset);
  RUN_TEST(test_evm_mload_underflow);
  RUN_TEST(test_evm_keccak256_underflow);
  RUN_TEST(test_evm_mload_out_of_gas);
  RUN_TEST(test_evm_keccak256_out_of_gas);
  RUN_TEST(test_evm_mstore_underflow);
  RUN_TEST(test_evm_mstore8_underflow);

  // Block information opcodes (0x40-0x4A)
  RUN_TEST(test_evm_coinbase);
  RUN_TEST(test_evm_timestamp);
  RUN_TEST(test_evm_number);
  RUN_TEST(test_evm_prevrandao);
  RUN_TEST(test_evm_gaslimit);
  RUN_TEST(test_evm_chainid);
  RUN_TEST(test_evm_basefee);
  RUN_TEST(test_evm_blobbasefee);
  RUN_TEST(test_evm_selfbalance);
  RUN_TEST(test_evm_selfbalance_without_state);
  RUN_TEST(test_evm_blockhash_valid);
  RUN_TEST(test_evm_blockhash_out_of_range);
  RUN_TEST(test_evm_blockhash_no_callback);
  RUN_TEST(test_evm_blobhash_valid);
  RUN_TEST(test_evm_blobhash_out_of_bounds);
  RUN_TEST(test_evm_blobhash_no_blobs);

  // Arithmetic opcode tests
  RUN_TEST(test_opcode_sub_basic);
  RUN_TEST(test_opcode_mul_basic);
  RUN_TEST(test_opcode_div_basic);
  RUN_TEST(test_opcode_div_by_zero);
  RUN_TEST(test_opcode_mod_basic);
  RUN_TEST(test_opcode_mod_by_zero);
  RUN_TEST(test_opcode_sdiv_positive_by_positive);
  RUN_TEST(test_opcode_sdiv_negative_by_positive);
  RUN_TEST(test_opcode_sdiv_by_zero);
  RUN_TEST(test_opcode_smod_positive_by_positive);
  RUN_TEST(test_opcode_smod_negative_by_positive);
  RUN_TEST(test_opcode_smod_by_zero);
  RUN_TEST(test_opcode_addmod_basic);
  RUN_TEST(test_opcode_addmod_by_zero);
  RUN_TEST(test_opcode_mulmod_basic);
  RUN_TEST(test_opcode_mulmod_by_zero);
  RUN_TEST(test_opcode_exp_basic);
  RUN_TEST(test_opcode_exp_zero_exponent);
  RUN_TEST(test_opcode_exp_gas_cost);
  RUN_TEST(test_opcode_signextend_byte_zero);
  RUN_TEST(test_opcode_signextend_byte_one);
  RUN_TEST(test_opcode_signextend_byte_31);
  RUN_TEST(test_opcode_sub_stack_underflow);
  RUN_TEST(test_opcode_addmod_stack_underflow);
  RUN_TEST(test_opcode_arithmetic_out_of_gas);

  // Comparison opcode tests
  RUN_TEST(test_opcode_lt_true_when_less);
  RUN_TEST(test_opcode_lt_false_when_greater);
  RUN_TEST(test_opcode_lt_false_when_equal);
  RUN_TEST(test_opcode_lt_stack_underflow);
  RUN_TEST(test_opcode_gt_true_when_greater);
  RUN_TEST(test_opcode_gt_false_when_less);
  RUN_TEST(test_opcode_gt_false_when_equal);
  RUN_TEST(test_opcode_gt_stack_underflow);
  RUN_TEST(test_opcode_eq_true_when_equal);
  RUN_TEST(test_opcode_eq_false_when_not_equal);
  RUN_TEST(test_opcode_eq_zero_equals_zero);
  RUN_TEST(test_opcode_eq_stack_underflow);
  RUN_TEST(test_opcode_iszero_true_when_zero);
  RUN_TEST(test_opcode_iszero_false_when_nonzero);
  RUN_TEST(test_opcode_iszero_false_when_one);
  RUN_TEST(test_opcode_iszero_stack_underflow);
  RUN_TEST(test_opcode_slt_positive_less_than_positive);
  RUN_TEST(test_opcode_slt_negative_less_than_positive);
  RUN_TEST(test_opcode_slt_positive_not_less_than_negative);
  RUN_TEST(test_opcode_slt_stack_underflow);
  RUN_TEST(test_opcode_sgt_positive_greater_than_negative);
  RUN_TEST(test_opcode_sgt_negative_not_greater_than_positive);
  RUN_TEST(test_opcode_sgt_larger_positive_greater);
  RUN_TEST(test_opcode_sgt_stack_underflow);
  RUN_TEST(test_opcode_comparison_out_of_gas);

  // Bitwise opcode tests
  RUN_TEST(test_opcode_and_basic);
  RUN_TEST(test_opcode_and_with_zero);
  RUN_TEST(test_opcode_and_with_max);
  RUN_TEST(test_opcode_and_stack_underflow);
  RUN_TEST(test_opcode_or_basic);
  RUN_TEST(test_opcode_or_with_zero);
  RUN_TEST(test_opcode_or_stack_underflow);
  RUN_TEST(test_opcode_xor_basic);
  RUN_TEST(test_opcode_xor_with_self);
  RUN_TEST(test_opcode_xor_stack_underflow);
  RUN_TEST(test_opcode_not_zero);
  RUN_TEST(test_opcode_not_max);
  RUN_TEST(test_opcode_not_double);
  RUN_TEST(test_opcode_not_stack_underflow);
  RUN_TEST(test_opcode_byte_extract_byte0);
  RUN_TEST(test_opcode_byte_extract_byte31);
  RUN_TEST(test_opcode_byte_index_out_of_range);
  RUN_TEST(test_opcode_byte_stack_underflow);
  RUN_TEST(test_opcode_shl_by_one);
  RUN_TEST(test_opcode_shl_by_zero);
  RUN_TEST(test_opcode_shl_by_256);
  RUN_TEST(test_opcode_shl_stack_underflow);
  RUN_TEST(test_opcode_shr_by_one);
  RUN_TEST(test_opcode_shr_by_zero);
  RUN_TEST(test_opcode_shr_by_256);
  RUN_TEST(test_opcode_shr_stack_underflow);
  RUN_TEST(test_opcode_sar_positive_value);
  RUN_TEST(test_opcode_sar_negative_value);
  RUN_TEST(test_opcode_sar_negative_by_large_shift);
  RUN_TEST(test_opcode_sar_stack_underflow);
  RUN_TEST(test_opcode_bitwise_out_of_gas);

  // Stack manipulation opcode tests - POP
  RUN_TEST(test_opcode_pop_basic);
  RUN_TEST(test_opcode_pop_multiple);
  RUN_TEST(test_opcode_pop_stack_underflow);
  RUN_TEST(test_opcode_pop_gas_consumption);
  RUN_TEST(test_opcode_pop_out_of_gas);

  // Stack manipulation opcode tests - DUP
  RUN_TEST(test_opcode_dup1_basic);
  RUN_TEST(test_opcode_dup1_stack_underflow);
  RUN_TEST(test_opcode_dup1_stack_overflow);
  RUN_TEST(test_opcode_dup2_basic);
  RUN_TEST(test_opcode_dup3_basic);
  RUN_TEST(test_opcode_dup4_basic);
  RUN_TEST(test_opcode_dup5_basic);
  RUN_TEST(test_opcode_dup6_basic);
  RUN_TEST(test_opcode_dup7_basic);
  RUN_TEST(test_opcode_dup8_basic);
  RUN_TEST(test_opcode_dup9_basic);
  RUN_TEST(test_opcode_dup10_basic);
  RUN_TEST(test_opcode_dup11_basic);
  RUN_TEST(test_opcode_dup12_basic);
  RUN_TEST(test_opcode_dup13_basic);
  RUN_TEST(test_opcode_dup14_basic);
  RUN_TEST(test_opcode_dup15_basic);
  RUN_TEST(test_opcode_dup16_basic);
  RUN_TEST(test_opcode_dup_gas_consumption);
  RUN_TEST(test_opcode_dup_out_of_gas);

  // Stack manipulation opcode tests - SWAP
  RUN_TEST(test_opcode_swap1_basic);
  RUN_TEST(test_opcode_swap1_stack_underflow);
  RUN_TEST(test_opcode_swap2_basic);
  RUN_TEST(test_opcode_swap3_basic);
  RUN_TEST(test_opcode_swap4_basic);
  RUN_TEST(test_opcode_swap5_basic);
  RUN_TEST(test_opcode_swap6_basic);
  RUN_TEST(test_opcode_swap7_basic);
  RUN_TEST(test_opcode_swap8_basic);
  RUN_TEST(test_opcode_swap9_basic);
  RUN_TEST(test_opcode_swap10_basic);
  RUN_TEST(test_opcode_swap11_basic);
  RUN_TEST(test_opcode_swap12_basic);
  RUN_TEST(test_opcode_swap13_basic);
  RUN_TEST(test_opcode_swap14_basic);
  RUN_TEST(test_opcode_swap15_basic);
  RUN_TEST(test_opcode_swap16_basic);
  RUN_TEST(test_opcode_swap_gas_consumption);
  RUN_TEST(test_opcode_swap_out_of_gas);

  // Context opcode tests - ADDRESS
  RUN_TEST(test_opcode_address_basic);
  RUN_TEST(test_opcode_address_gas_consumption);
  RUN_TEST(test_opcode_address_out_of_gas);
  RUN_TEST(test_opcode_address_stack_overflow);

  // Context opcode tests - CALLER
  RUN_TEST(test_opcode_caller_basic);
  RUN_TEST(test_opcode_caller_gas_consumption);
  RUN_TEST(test_opcode_caller_out_of_gas);

  // Context opcode tests - CALLVALUE
  RUN_TEST(test_opcode_callvalue_basic);
  RUN_TEST(test_opcode_callvalue_zero);
  RUN_TEST(test_opcode_callvalue_large);
  RUN_TEST(test_opcode_callvalue_gas_consumption);

  // Context opcode tests - CALLDATASIZE
  RUN_TEST(test_opcode_calldatasize_basic);
  RUN_TEST(test_opcode_calldatasize_empty);
  RUN_TEST(test_opcode_calldatasize_large);

  // Context opcode tests - CODESIZE
  RUN_TEST(test_opcode_codesize_basic);
  RUN_TEST(test_opcode_codesize_minimal);

  // Context opcode tests - ORIGIN
  RUN_TEST(test_opcode_origin_basic);
  RUN_TEST(test_opcode_origin_gas_consumption);

  // Context opcode tests - GASPRICE
  RUN_TEST(test_opcode_gasprice_basic);
  RUN_TEST(test_opcode_gasprice_large);

  // Context opcode tests - CALLDATALOAD
  RUN_TEST(test_opcode_calldataload_basic);
  RUN_TEST(test_opcode_calldataload_offset);
  RUN_TEST(test_opcode_calldataload_partial);
  RUN_TEST(test_opcode_calldataload_out_of_bounds);
  RUN_TEST(test_opcode_calldataload_empty);
  RUN_TEST(test_opcode_calldataload_stack_underflow);

  // Context opcode tests - CALLDATACOPY
  RUN_TEST(test_opcode_calldatacopy_basic);
  RUN_TEST(test_opcode_calldatacopy_zero_size);
  RUN_TEST(test_opcode_calldatacopy_zero_pad);
  RUN_TEST(test_opcode_calldatacopy_out_of_bounds);
  RUN_TEST(test_opcode_calldatacopy_stack_underflow);

  // Context opcode tests - CODECOPY
  RUN_TEST(test_opcode_codecopy_basic);
  RUN_TEST(test_opcode_codecopy_zero_size);
  RUN_TEST(test_opcode_codecopy_zero_pad);
  RUN_TEST(test_opcode_codecopy_stack_underflow);

  // Context opcode tests - RETURNDATASIZE
  RUN_TEST(test_opcode_returndatasize_zero);

  // Context opcode tests - RETURNDATACOPY
  RUN_TEST(test_opcode_returndatacopy_empty);

  // Logging opcode tests - Basic functionality
  RUN_TEST(test_opcode_log0_basic);
  RUN_TEST(test_opcode_log1_basic);
  RUN_TEST(test_opcode_log2_basic);
  RUN_TEST(test_opcode_log3_basic);
  RUN_TEST(test_opcode_log4_basic);

  // Logging opcode tests - Zero-size data
  RUN_TEST(test_opcode_log0_zero_data);
  RUN_TEST(test_opcode_log1_zero_data);
  RUN_TEST(test_opcode_log4_zero_data);

  // Logging opcode tests - Stack underflow
  RUN_TEST(test_opcode_log0_stack_underflow);
  RUN_TEST(test_opcode_log1_stack_underflow);
  RUN_TEST(test_opcode_log2_stack_underflow);
  RUN_TEST(test_opcode_log3_stack_underflow);
  RUN_TEST(test_opcode_log4_stack_underflow);

  // Logging opcode tests - Static context (write protection)
  RUN_TEST(test_opcode_log0_static_context);
  RUN_TEST(test_opcode_log1_static_context);
  RUN_TEST(test_opcode_log4_static_context);

  // Logging opcode tests - Gas consumption
  RUN_TEST(test_opcode_log0_gas_exact);
  RUN_TEST(test_opcode_log1_gas_exact);
  RUN_TEST(test_opcode_log2_gas_exact);
  RUN_TEST(test_opcode_log4_gas_exact);

  // Logging opcode tests - Out of gas
  RUN_TEST(test_opcode_log0_out_of_gas_base);
  RUN_TEST(test_opcode_log1_out_of_gas_topics);
  RUN_TEST(test_opcode_log0_out_of_gas_data);
  RUN_TEST(test_opcode_log0_out_of_gas_memory);

  // Logging opcode tests - Topic verification
  RUN_TEST(test_opcode_log1_topic_value);
  RUN_TEST(test_opcode_log4_all_topics);
  RUN_TEST(test_opcode_log2_topic_order);

  // Logging opcode tests - Data verification
  RUN_TEST(test_opcode_log0_data_content);
  RUN_TEST(test_opcode_log0_data_offset);
  RUN_TEST(test_opcode_log0_large_data);

  // Logging opcode tests - Multiple logs
  RUN_TEST(test_opcode_log_multiple_logs);
  RUN_TEST(test_opcode_log_mixed_types);

  // Logging opcode tests - Address verification
  RUN_TEST(test_opcode_log0_address);

  // Logging opcode tests - Memory expansion
  RUN_TEST(test_opcode_log0_memory_expansion);
  RUN_TEST(test_opcode_log0_memory_preexisting);

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
  RUN_TEST(test_world_state_snapshot_empty);
  RUN_TEST(test_world_state_snapshot_single_account);
  RUN_TEST(test_world_state_snapshot_with_storage);
  RUN_TEST(test_world_state_snapshot_multiple_accounts);
  RUN_TEST(test_world_state_snapshot_with_code);

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

  // Block executor - intrinsic gas tests
  RUN_TEST(test_intrinsic_gas_simple_transfer);
  RUN_TEST(test_intrinsic_gas_with_calldata);
  RUN_TEST(test_intrinsic_gas_contract_creation);
  RUN_TEST(test_intrinsic_gas_with_access_list);

  // Block executor - validation tests
  RUN_TEST(test_validation_valid_tx);
  RUN_TEST(test_validation_nonce_too_low);
  RUN_TEST(test_validation_nonce_too_high);
  RUN_TEST(test_validation_insufficient_balance);
  RUN_TEST(test_validation_intrinsic_gas_too_low);
  RUN_TEST(test_validation_gas_limit_exceeded);

  // Block executor - execution tests
  RUN_TEST(test_block_executor_empty_block);
  RUN_TEST(test_block_executor_single_tx);
  RUN_TEST(test_block_executor_recipient_balance);
  RUN_TEST(test_block_executor_coinbase_fee);
  RUN_TEST(test_block_executor_multiple_txs);
  RUN_TEST(test_block_executor_mixed_valid_rejected);
  RUN_TEST(test_block_executor_nonce_increment_on_failed_execution);

#ifndef DIV0_FREESTANDING
  // JSON core tests
  RUN_TEST(test_json_parse_empty_object);
  RUN_TEST(test_json_parse_nested_object);
  RUN_TEST(test_json_parse_array);
  RUN_TEST(test_json_get_hex_u64);
  RUN_TEST(test_json_get_hex_uint256);
  RUN_TEST(test_json_get_hex_address);
  RUN_TEST(test_json_get_hex_hash);
  RUN_TEST(test_json_get_hex_bytes);
  RUN_TEST(test_json_obj_iteration);
  RUN_TEST(test_json_arr_iteration);
  RUN_TEST(test_json_write_object);
  RUN_TEST(test_json_write_array);
  RUN_TEST(test_json_write_hex_values);

  // Hex encoding tests
  RUN_TEST(test_hex_encode_u64);
  RUN_TEST(test_hex_encode_uint256);
  RUN_TEST(test_hex_encode_uint256_padded);
  RUN_TEST(test_hex_decode_u64);
  RUN_TEST(test_hex_decode_uint256);

  // Alloc parsing tests
  RUN_TEST(test_alloc_parse_empty);
  RUN_TEST(test_alloc_parse_single_account);
  RUN_TEST(test_alloc_parse_with_storage);
  RUN_TEST(test_alloc_parse_with_code);
  RUN_TEST(test_alloc_roundtrip);

  // Env parsing tests
  RUN_TEST(test_env_parse_required_fields);
  RUN_TEST(test_env_parse_optional_fields);
  RUN_TEST(test_env_parse_block_hashes);
  RUN_TEST(test_env_parse_withdrawals);

  // Txs parsing tests
  RUN_TEST(test_txs_parse_legacy);
  RUN_TEST(test_txs_parse_eip1559);
  RUN_TEST(test_txs_parse_empty_array);
#endif

  // Cleanup
  div0_arena_destroy(&test_arena);

  return UNITY_END();
}