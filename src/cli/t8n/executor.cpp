// executor.cpp - Transaction execution for t8n

#include "t8n/executor.h"

#include "div0/ethereum/transaction/rlp.h"
#include "div0/evm/block_context.h"
#include "div0/evm/evm.h"
#include "div0/evm/execution_environment.h"

namespace div0::cli {

namespace {

/// Build BlockContext from EnvInput with block hash callback
evm::BlockContext build_block_context(const EnvInput& env,
                                      const std::map<uint64_t, types::Hash>& block_hashes) {
  evm::BlockContext block_ctx{};
  block_ctx.number = env.number;
  block_ctx.timestamp = env.timestamp;
  block_ctx.gas_limit = env.gas_limit;
  block_ctx.coinbase = env.coinbase;
  block_ctx.base_fee = env.base_fee.value_or(types::Uint256::zero());
  block_ctx.prev_randao = env.prev_randao.value_or(types::Uint256::zero());

  // Set up block hash lookup callback (captures block_hashes by value to avoid dangling reference)
  block_ctx.get_block_hash = [block_hashes](uint64_t block_number) -> types::Uint256 {
    auto it = block_hashes.find(block_number);
    if (it != block_hashes.end()) {
      return it->second.to_uint256();
    }
    return types::Uint256::zero();
  };

  return block_ctx;
}

/// Get effective gas price for a transaction
[[nodiscard]] types::Uint256 get_effective_gas_price(const ethereum::Transaction& tx,
                                                     const types::Uint256& base_fee) {
  return tx.effective_gas_price(base_fee);
}

}  // namespace

ExecutionOutput execute_transactions(T8nState& state, const std::vector<ethereum::Transaction>& txs,
                                     const EnvInput& env, evm::Fork fork,
                                     [[maybe_unused]] uint64_t chain_id,
                                     crypto::Secp256k1Context& secp_ctx) {
  ExecutionOutput output;

  // Build block context
  const auto block_ctx = build_block_context(env, env.block_hashes);

  // Create EVM instance
  evm::EVM evm(state.context(), fork);

  // Base fee for gas price calculation
  const auto base_fee = env.base_fee.value_or(types::Uint256::zero());

  uint64_t cumulative_gas = 0;

  for (size_t i = 0; i < txs.size(); ++i) {
    const auto& tx = txs[i];

    // Recover sender from signature
    auto sender_opt = ethereum::recover_sender(tx, secp_ctx);
    if (!sender_opt) {
      output.rejected.push_back({i, "invalid signature or failed sender recovery"});
      continue;
    }
    const types::Address sender = *sender_opt;

    // Get destination and code
    types::Address to_address;
    std::span<const uint8_t> code;
    types::Bytes code_storage;  // Storage for code if we need to copy it

    // Check if this is a contract creation (to is nullopt)
    const auto to_opt = tx.to();
    if (!to_opt) {
      // Contract creation - code is in tx data, to address is zero for now
      // CREATE address computation will be added when we implement ethereum library
      to_address = types::Address(types::Uint256::zero());
      const auto input = tx.data();
      code = {input.data(), input.size()};
    } else {
      // Message call - get code from state
      to_address = *to_opt;
      const auto code_span = state.code().get_code(to_address);
      code_storage.assign(code_span.begin(), code_span.end());
      code = {code_storage.data(), code_storage.size()};
    }

    // Build execution environment
    const auto gas_price = get_effective_gas_price(tx, base_fee);
    const auto gas_limit = tx.gas_limit();
    const auto& value = tx.value();
    const auto input = tx.data();

    // Get blob hashes if this is a blob transaction (convert Hash to Uint256)
    std::vector<types::Uint256> blob_hash_storage;
    std::span<const types::Uint256> blob_hashes;
    if (const auto* blob_tx = tx.get_if<ethereum::Eip4844Tx>()) {
      blob_hash_storage.reserve(blob_tx->blob_versioned_hashes.size());
      for (const auto& hash : blob_tx->blob_versioned_hashes) {
        blob_hash_storage.push_back(hash.to_uint256());
      }
      blob_hashes = {blob_hash_storage.data(), blob_hash_storage.size()};
    }

    const evm::ExecutionEnvironment exec_env{
        .block = &block_ctx,
        .tx = {.origin = sender, .gas_price = gas_price, .blob_hashes = blob_hashes},
        .call = {.value = value,
                 .gas = gas_limit,
                 .code = code,
                 .input = {input.data(), input.size()},
                 .caller = sender,
                 .address = to_address,
                 .is_static = false}};

    // Execute transaction
    const auto result = evm.execute(exec_env);

    // Update cumulative gas
    cumulative_gas += result.gas_used;

    // Build receipt
    ethereum::Receipt receipt;
    if (result.success()) {
      receipt = ethereum::Receipt::create_success(tx.type(), cumulative_gas, result.logs);
    } else {
      receipt = ethereum::Receipt::create_failure(tx.type(), cumulative_gas);
    }

    output.receipts.push_back(std::move(receipt));
    output.executed_txs.push_back(tx);
    output.gas_used += result.gas_used;

    // Track blob gas if applicable
    if (const auto* blob_tx = tx.get_if<ethereum::Eip4844Tx>()) {
      // Each blob uses BLOB_GAS_PER_BLOB (131072) gas
      output.blob_gas_used += blob_tx->blob_versioned_hashes.size() * 131072;
    }
  }

  return output;
}

}  // namespace div0::cli
