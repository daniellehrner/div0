#ifndef DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H
#define DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H

#include <cstdint>
#include <optional>
#include <span>
#include <variant>

#include "div0/ethereum/transaction/eip1559_tx.h"
#include "div0/ethereum/transaction/eip2930_tx.h"
#include "div0/ethereum/transaction/eip4844_tx.h"
#include "div0/ethereum/transaction/eip7702_tx.h"
#include "div0/ethereum/transaction/legacy_tx.h"
#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief Unified transaction wrapper using std::variant.
 *
 * Provides a common interface for all transaction types through std::visit.
 * Supports all EIP-2718 transaction types (0-4).
 */
class Transaction {
 public:
  using Variant = std::variant<LegacyTx, Eip2930Tx, Eip1559Tx, Eip4844Tx, Eip7702Tx>;

  /// Constructors for each transaction type
  explicit Transaction(LegacyTx tx) : tx_(std::move(tx)) {}
  explicit Transaction(Eip2930Tx tx) : tx_(std::move(tx)) {}
  explicit Transaction(Eip1559Tx tx) : tx_(std::move(tx)) {}
  explicit Transaction(Eip4844Tx tx) : tx_(std::move(tx)) {}
  explicit Transaction(Eip7702Tx tx) : tx_(std::move(tx)) {}

  /// Get the transaction type
  [[nodiscard]] TxType type() const noexcept {
    return std::visit(
        [](const auto& tx) -> TxType {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, LegacyTx>) {
            return TxType::Legacy;
          } else if constexpr (std::is_same_v<T, Eip2930Tx>) {
            return TxType::AccessList;
          } else if constexpr (std::is_same_v<T, Eip1559Tx>) {
            return TxType::DynamicFee;
          } else if constexpr (std::is_same_v<T, Eip4844Tx>) {
            return TxType::Blob;
          } else if constexpr (std::is_same_v<T, Eip7702Tx>) {
            return TxType::SetCode;
          }
        },
        tx_);
  }

  /// Get the nonce
  [[nodiscard]] uint64_t nonce() const noexcept {
    return std::visit([](const auto& tx) { return tx.nonce; }, tx_);
  }

  /// Get the gas limit
  [[nodiscard]] uint64_t gas_limit() const noexcept {
    return std::visit([](const auto& tx) { return tx.gas_limit; }, tx_);
  }

  /// Get the value
  [[nodiscard]] const types::Uint256& value() const noexcept {
    return std::visit([](const auto& tx) -> const types::Uint256& { return tx.value; }, tx_);
  }

  /// Get the data
  [[nodiscard]] std::span<const uint8_t> data() const noexcept {
    return std::visit(
        [](const auto& tx) -> std::span<const uint8_t> {
          return std::span<const uint8_t>(tx.data);
        },
        tx_);
  }

  /// Get the recipient (nullopt for contract creation)
  [[nodiscard]] std::optional<types::Address> to() const noexcept {
    return std::visit(
        [](const auto& tx) -> std::optional<types::Address> {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, Eip4844Tx> || std::is_same_v<T, Eip7702Tx>) {
            // Blob and SetCode txs always have a recipient
            return tx.to;
          } else {
            return tx.to;
          }
        },
        tx_);
  }

  /// Get the chain ID (nullopt for pre-EIP-155 legacy)
  [[nodiscard]] std::optional<uint64_t> chain_id() const noexcept {
    return std::visit(
        [](const auto& tx) -> std::optional<uint64_t> {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, LegacyTx>) {
            return tx.chain_id();
          } else {
            return tx.chain_id;
          }
        },
        tx_);
  }

  /// Check if this is a contract creation transaction
  [[nodiscard]] bool is_contract_creation() const noexcept {
    return std::visit([](const auto& tx) { return tx.is_contract_creation(); }, tx_);
  }

  /// Get the recovery ID for signature recovery
  [[nodiscard]] int recovery_id() const noexcept {
    return std::visit([](const auto& tx) { return tx.recovery_id(); }, tx_);
  }

  /// Get the r component of signature
  [[nodiscard]] const types::Uint256& r() const noexcept {
    return std::visit([](const auto& tx) -> const types::Uint256& { return tx.r; }, tx_);
  }

  /// Get the s component of signature
  [[nodiscard]] const types::Uint256& s() const noexcept {
    return std::visit([](const auto& tx) -> const types::Uint256& { return tx.s; }, tx_);
  }

  /// Calculate effective gas price given base fee (for EIP-1559+ txs)
  [[nodiscard]] types::Uint256 effective_gas_price(const types::Uint256& base_fee) const noexcept {
    return std::visit(
        [&base_fee](const auto& tx) -> types::Uint256 {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, LegacyTx> || std::is_same_v<T, Eip2930Tx>) {
            // Legacy and EIP-2930 use simple gas_price
            return tx.gas_price;
          } else {
            return tx.effective_gas_price(base_fee);
          }
        },
        tx_);
  }

  /// Get gas price (for legacy/EIP-2930) or max_fee_per_gas (for EIP-1559+)
  [[nodiscard]] const types::Uint256& max_fee_per_gas() const noexcept {
    return std::visit(
        [](const auto& tx) -> const types::Uint256& {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, LegacyTx> || std::is_same_v<T, Eip2930Tx>) {
            return tx.gas_price;
          } else {
            return tx.max_fee_per_gas;
          }
        },
        tx_);
  }

  /// Get access list (empty for legacy)
  [[nodiscard]] const AccessList* access_list() const noexcept {
    return std::visit(
        [](const auto& tx) -> const AccessList* {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, LegacyTx>) {
            return nullptr;
          } else {
            return &tx.access_list;
          }
        },
        tx_);
  }

  /// Get authorization list (only for EIP-7702)
  [[nodiscard]] const AuthorizationList* authorization_list() const noexcept {
    return std::visit(
        [](const auto& tx) -> const AuthorizationList* {
          using T = std::decay_t<decltype(tx)>;
          if constexpr (std::is_same_v<T, Eip7702Tx>) {
            return &tx.authorization_list;
          } else {
            return nullptr;
          }
        },
        tx_);
  }

  /// Access the underlying variant
  [[nodiscard]] const Variant& variant() const noexcept { return tx_; }
  [[nodiscard]] Variant& variant() noexcept { return tx_; }

  /// Type-safe access to specific transaction type
  template <typename T>
  [[nodiscard]] const T* get_if() const noexcept {
    return std::get_if<T>(&tx_);
  }

  template <typename T>
  [[nodiscard]] T* get_if() noexcept {
    return std::get_if<T>(&tx_);
  }

  /// Equality
  [[nodiscard]] bool operator==(const Transaction& other) const noexcept = default;

 private:
  Variant tx_;
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_TRANSACTION_H
