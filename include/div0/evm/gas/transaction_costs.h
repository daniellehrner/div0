#ifndef DIV0_EVM_GAS_TRANSACTION_COSTS_H
#define DIV0_EVM_GAS_TRANSACTION_COSTS_H

#include <cstdint>
#include <span>

namespace div0::evm::gas {

/// Transaction gas constants
namespace tx {

// Base transaction costs
constexpr uint64_t BASE_GAS = 21000;               // Base gas for non-contract-creation tx
constexpr uint64_t CONTRACT_CREATION_GAS = 53000;  // Base gas for contract creation tx

// Calldata costs
constexpr uint64_t DATA_ZERO_GAS = 4;               // Per zero byte in calldata
constexpr uint64_t DATA_NONZERO_GAS_FRONTIER = 68;  // Per non-zero byte (pre-EIP-2028)
constexpr uint64_t DATA_NONZERO_GAS_EIP2028 = 16;   // Per non-zero byte (post-EIP-2028)

// EIP-7623: Data floor costs
constexpr uint64_t TOKEN_PER_NONZERO_BYTE = 4;  // Token cost per non-zero byte
constexpr uint64_t COST_FLOOR_PER_TOKEN = 10;   // Cost floor per token

// Init code costs (EIP-3860)
constexpr uint64_t INIT_CODE_WORD_GAS = 2;  // Per 32-byte word of init code

// Access list costs (EIP-2930)
constexpr uint64_t ACCESS_LIST_ADDRESS_GAS = 2400;      // Per address in access list
constexpr uint64_t ACCESS_LIST_STORAGE_KEY_GAS = 1900;  // Per storage key in access list

// EIP-7702: Set code authorization costs
constexpr uint64_t AUTH_TUPLE_GAS = 12500;          // Per authorization tuple
constexpr uint64_t AUTH_EMPTY_ACCOUNT_GAS = 25000;  // If authorizing non-existent account

// EIP-4844: Blob transaction costs
constexpr uint64_t BLOB_GAS_PER_BLOB = 1 << 17;  // 131072 gas per blob
constexpr uint64_t BLOB_BASE_COST = 1 << 13;     // 8192 base execution cost for a blob
constexpr uint64_t BLOB_MIN_GAS_PRICE = 1;       // Minimum blob gas price
constexpr uint64_t BLOB_MAX_BLOBS = 6;           // Maximum blobs per transaction

// Blob field element sizes
constexpr uint64_t BLOB_BYTES_PER_FIELD_ELEMENT = 32;
constexpr uint64_t BLOB_FIELD_ELEMENTS_PER_BLOB = 4096;

// Point evaluation precompile (EIP-4844)
constexpr uint64_t BLOB_POINT_EVALUATION_GAS = 50000;

}  // namespace tx

/**
 * @brief Count zero bytes in data.
 * @param data Input data span
 * @return Number of zero bytes
 */
[[nodiscard]] inline uint64_t count_zero_bytes(std::span<const uint8_t> data) noexcept {
  uint64_t count = 0;
  for (const uint8_t byte : data) {
    if (byte == 0) {
      ++count;
    }
  }
  return count;
}

/**
 * @brief Calculate intrinsic gas for a transaction.
 *
 * Formula:
 * - base_gas (21000 or 53000 for creation)
 * - + calldata_cost (zero_bytes * 4 + nonzero_bytes * 16/68)
 * - + access_list_cost (addresses * 2400 + storage_keys * 1900)
 * - + init_code_cost (word_count * 2 for EIP-3860 contract creation)
 * - + auth_tuple_cost (auth_count * 25000 for EIP-7702)
 *
 * @param data Transaction calldata
 * @param is_contract_creation Whether this creates a contract
 * @param is_eip2028 Whether EIP-2028 (reduced calldata cost) is active
 * @param is_eip3860 Whether EIP-3860 (init code metering) is active
 * @param access_list_addresses Number of addresses in access list
 * @param access_list_storage_keys Number of storage keys in access list
 * @param auth_tuple_count Number of authorization tuples (EIP-7702)
 * @return Total intrinsic gas, or UINT64_MAX on overflow
 */
[[nodiscard]] inline uint64_t intrinsic_gas(std::span<const uint8_t> data,
                                            bool is_contract_creation, bool is_eip2028,
                                            bool is_eip3860, uint64_t access_list_addresses = 0,
                                            uint64_t access_list_storage_keys = 0,
                                            uint64_t auth_tuple_count = 0) noexcept {
  // Base gas
  uint64_t gas = is_contract_creation ? tx::CONTRACT_CREATION_GAS : tx::BASE_GAS;

  // Calldata cost
  if (!data.empty()) {
    const uint64_t data_len = data.size();
    const uint64_t zero_bytes = count_zero_bytes(data);
    const uint64_t nonzero_bytes = data_len - zero_bytes;

    const uint64_t nonzero_gas =
        is_eip2028 ? tx::DATA_NONZERO_GAS_EIP2028 : tx::DATA_NONZERO_GAS_FRONTIER;

    // Check overflow for non-zero bytes
    uint64_t nonzero_cost = 0;
    if (__builtin_mul_overflow(nonzero_bytes, nonzero_gas, &nonzero_cost)) [[unlikely]] {
      return UINT64_MAX;
    }
    if (__builtin_add_overflow(gas, nonzero_cost, &gas)) [[unlikely]] {
      return UINT64_MAX;
    }

    // Zero bytes cost
    uint64_t zero_cost = 0;
    if (__builtin_mul_overflow(zero_bytes, tx::DATA_ZERO_GAS, &zero_cost)) [[unlikely]] {
      return UINT64_MAX;
    }
    if (__builtin_add_overflow(gas, zero_cost, &gas)) [[unlikely]] {
      return UINT64_MAX;
    }

    // Init code word cost (EIP-3860)
    if (is_contract_creation && is_eip3860) {
      // Word count = ceil(data_len / 32)
      const uint64_t words = (data_len + 31) / 32;
      uint64_t init_cost = 0;
      if (__builtin_mul_overflow(words, tx::INIT_CODE_WORD_GAS, &init_cost)) [[unlikely]] {
        return UINT64_MAX;
      }
      if (__builtin_add_overflow(gas, init_cost, &gas)) [[unlikely]] {
        return UINT64_MAX;
      }
    }
  }

  // Access list cost (EIP-2930)
  if (access_list_addresses > 0) {
    uint64_t addr_cost = 0;
    if (__builtin_mul_overflow(access_list_addresses, tx::ACCESS_LIST_ADDRESS_GAS, &addr_cost))
        [[unlikely]] {
      return UINT64_MAX;
    }
    if (__builtin_add_overflow(gas, addr_cost, &gas)) [[unlikely]] {
      return UINT64_MAX;
    }
  }

  if (access_list_storage_keys > 0) {
    uint64_t key_cost = 0;
    if (__builtin_mul_overflow(access_list_storage_keys, tx::ACCESS_LIST_STORAGE_KEY_GAS,
                               &key_cost)) [[unlikely]] {
      return UINT64_MAX;
    }
    if (__builtin_add_overflow(gas, key_cost, &gas)) [[unlikely]] {
      return UINT64_MAX;
    }
  }

  // Authorization tuple cost (EIP-7702)
  // Note: EIP-7702 uses 25000 per auth tuple (same as CallNewAccountGas in geth)
  if (auth_tuple_count > 0) {
    uint64_t auth_cost = 0;
    if (__builtin_mul_overflow(auth_tuple_count, tx::AUTH_EMPTY_ACCOUNT_GAS, &auth_cost))
        [[unlikely]] {
      return UINT64_MAX;
    }
    if (__builtin_add_overflow(gas, auth_cost, &gas)) [[unlikely]] {
      return UINT64_MAX;
    }
  }

  return gas;
}

/**
 * @brief Calculate floor data gas for EIP-7623.
 *
 * Minimum gas required based on data tokens:
 * tokens = nonzero_bytes * 4 + zero_bytes
 * floor_gas = 21000 + tokens * 10
 *
 * @param data Transaction calldata
 * @return Floor gas cost, or UINT64_MAX on overflow
 */
[[nodiscard]] inline uint64_t floor_data_gas(std::span<const uint8_t> data) noexcept {
  if (data.empty()) {
    return tx::BASE_GAS;
  }

  const uint64_t data_len = data.size();
  const uint64_t zero_bytes = count_zero_bytes(data);
  const uint64_t nonzero_bytes = data_len - zero_bytes;

  // tokens = nonzero_bytes * 4 + zero_bytes
  uint64_t tokens = 0;
  uint64_t nz_tokens = 0;
  if (__builtin_mul_overflow(nonzero_bytes, tx::TOKEN_PER_NONZERO_BYTE, &nz_tokens)) [[unlikely]] {
    return UINT64_MAX;
  }
  if (__builtin_add_overflow(nz_tokens, zero_bytes, &tokens)) [[unlikely]] {
    return UINT64_MAX;
  }

  // floor_gas = BASE_GAS + tokens * COST_FLOOR_PER_TOKEN
  uint64_t token_cost = 0;
  if (__builtin_mul_overflow(tokens, tx::COST_FLOOR_PER_TOKEN, &token_cost)) [[unlikely]] {
    return UINT64_MAX;
  }

  uint64_t floor_gas = 0;
  if (__builtin_add_overflow(tx::BASE_GAS, token_cost, &floor_gas)) [[unlikely]] {
    return UINT64_MAX;
  }

  return floor_gas;
}

/**
 * @brief Calculate blob gas used by a transaction.
 * @param blob_count Number of blobs in the transaction
 * @return Total blob gas (blob_count * 131072)
 */
[[nodiscard]] constexpr uint64_t blob_gas_used(uint64_t blob_count) noexcept {
  return blob_count * tx::BLOB_GAS_PER_BLOB;
}

}  // namespace div0::evm::gas

#endif  // DIV0_EVM_GAS_TRANSACTION_COSTS_H
