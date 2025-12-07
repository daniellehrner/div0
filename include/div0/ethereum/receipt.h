#ifndef DIV0_ETHEREUM_RECEIPT_H
#define DIV0_ETHEREUM_RECEIPT_H

#include <cstdint>
#include <vector>

#include "div0/ethereum/bloom.h"
#include "div0/ethereum/transaction/types.h"
#include "div0/evm/execution_result.h"
#include "div0/types/bytes.h"

namespace div0::ethereum {

// Forward declaration
struct Receipt;

/**
 * @brief RLP encode a single log entry.
 *
 * Log: [address, [topic1, topic2, ...], data]
 */
[[nodiscard]] types::Bytes rlp_encode(const evm::Log& log);

/**
 * @brief RLP encode a list of logs.
 *
 * Returns RLP([log1, log2, ...])
 */
[[nodiscard]] types::Bytes rlp_encode_logs(const std::vector<evm::Log>& logs);

/**
 * @brief RLP encode a receipt.
 *
 * For legacy receipts: RLP([status, cumulative_gas_used, bloom, logs])
 * For typed receipts: type_byte || RLP([status, cumulative_gas_used, bloom, logs])
 */
[[nodiscard]] types::Bytes rlp_encode(const Receipt& receipt);

/**
 * @brief Transaction receipt.
 *
 * Contains execution result, cumulative gas, and logs bloom for a transaction.
 * Post-Byzantium (EIP-658) uses status byte (0=fail, 1=success).
 *
 * RLP encoding varies by transaction type:
 * - Legacy (type 0): RLP([status, cumulative_gas_used, bloom, logs])
 * - Typed (types 1-4): type_byte || RLP([status, cumulative_gas_used, bloom, logs])
 */
struct Receipt {
  TxType tx_type{TxType::Legacy};
  uint8_t status{0};  // 0 = failure, 1 = success (post-Byzantium/EIP-658)
  uint64_t cumulative_gas_used{0};
  Bloom bloom;
  std::vector<evm::Log> logs;

  /**
   * @brief Check if transaction succeeded.
   */
  [[nodiscard]] bool success() const noexcept { return status == 1; }

  /**
   * @brief Build the bloom filter from logs.
   *
   * Populates the bloom field by adding each log's address and topics.
   * Should be called after logs are populated.
   */
  void build_bloom() noexcept {
    bloom.clear();
    for (const auto& log : logs) {
      bloom.add(log.address);
      for (const auto& topic : log.topics) {
        bloom.add(topic);
      }
    }
  }

  /**
   * @brief Create a success receipt with the given execution result.
   *
   * @param tx_type Transaction type
   * @param cumulative_gas Cumulative gas used in block up to and including this tx
   * @param exec_logs Logs from execution
   * @return Receipt with bloom automatically built
   */
  [[nodiscard]] static Receipt create_success(const TxType tx_type, const uint64_t cumulative_gas,
                                              std::vector<evm::Log> exec_logs) {
    Receipt receipt;
    receipt.tx_type = tx_type;
    receipt.status = 1;
    receipt.cumulative_gas_used = cumulative_gas;
    receipt.logs = std::move(exec_logs);
    receipt.build_bloom();
    return receipt;
  }

  /**
   * @brief Create a failure receipt.
   *
   * @param tx_type Transaction type
   * @param cumulative_gas Cumulative gas used in block up to and including this tx
   * @return Receipt with status=0 and empty logs/bloom
   */
  [[nodiscard]] static Receipt create_failure(const TxType tx_type, const uint64_t cumulative_gas) {
    Receipt receipt;
    receipt.tx_type = tx_type;
    receipt.status = 0;
    receipt.cumulative_gas_used = cumulative_gas;
    // logs and bloom remain empty for failed txs
    return receipt;
  }

  [[nodiscard]] bool operator==(const Receipt& other) const noexcept {
    return tx_type == other.tx_type && status == other.status &&
           cumulative_gas_used == other.cumulative_gas_used && bloom == other.bloom &&
           logs.size() == other.logs.size() &&
           std::equal(logs.begin(), logs.end(), other.logs.begin());
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_RECEIPT_H
