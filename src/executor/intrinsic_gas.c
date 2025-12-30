#include "div0/ethereum/transaction/access_list.h"
#include "div0/evm/gas.h"
#include "div0/executor/block_executor.h"

uint64_t tx_intrinsic_gas(const transaction_t *tx) {
  uint64_t gas = GAS_TX; // 21000 base cost

  // Calldata cost: 4 per zero byte, 16 per non-zero byte
  const bytes_t *data = transaction_data(tx);
  if (data && data->data) {
    for (size_t i = 0; i < data->size; i++) {
      gas += data->data[i] == 0 ? GAS_TX_DATA_ZERO : GAS_TX_DATA_NON_ZERO;
    }
  }

  // Contract creation cost
  if (transaction_is_create(tx)) {
    gas += GAS_CREATE; // 32000

    // EIP-3860: initcode cost (2 gas per word)
    if (data && data->size > 0) {
      uint64_t words = (data->size + 31) / 32;
      gas += words * 2; // INITCODE_WORD_COST
    }
  }

  // Access list cost (EIP-2930)
  const access_list_t *access_list = transaction_access_list(tx);
  if (access_list) {
    for (size_t i = 0; i < access_list->count; i++) {
      gas += GAS_ACCESS_LIST_ADDRESS; // 2400 per address
      gas +=
          access_list->entries[i].storage_keys_count * GAS_ACCESS_LIST_STORAGE_KEY; // 1900 per key
    }
  }

  return gas;
}
