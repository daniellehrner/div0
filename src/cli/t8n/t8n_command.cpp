// t8n_command.cpp - State transition tool command implementation

#include "t8n/t8n_command.h"

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/eip1559.h"
#include "div0/ethereum/roots.h"
#include "div0/log/log.h"
#include "div0/trie/mpt.h"
#include "exit_codes.h"
#include "t8n/executor.h"
#include "t8n/fork_parser.h"
#include "t8n/input.h"
#include "t8n/output.h"
#include "t8n/state_builder.h"

namespace div0::cli {

void setup_t8n_command(CLI::App& app, T8nOptions& opts) {
  // ===========================================================================
  // INPUT OPTIONS
  // ===========================================================================

  app.add_option("--input.alloc", opts.input_alloc, "Input allocations file (or 'stdin')")
      ->default_val("alloc.json");

  app.add_option("--input.env", opts.input_env, "Input environment file (or 'stdin')")
      ->default_val("env.json");

  app.add_option("--input.txs", opts.input_txs, "Input transactions file (or 'stdin')")
      ->default_val("txs.json");

  // ===========================================================================
  // OUTPUT OPTIONS
  // ===========================================================================

  app.add_option("--output.basedir", opts.output_basedir, "Output directory")->default_val(".");

  app.add_option("--output.result", opts.output_result, "Result output file")
      ->default_val("result.json");

  app.add_option("--output.alloc", opts.output_alloc, "Post-state output file")
      ->default_val("alloc.json");

  app.add_option("--output.body", opts.output_body, "RLP-encoded transactions output file");

  // ===========================================================================
  // STATE OPTIONS
  // ===========================================================================

  app.add_option("--state.fork", opts.fork, "Fork name (e.g., Shanghai, Cancun, Prague)")
      ->default_val("Shanghai");

  app.add_option("--state.chainid", opts.chain_id, "Chain ID")->default_val(1);

  app.add_option("--state.reward", opts.reward, "Block reward (-1 to disable)")->default_val(0);

  // ===========================================================================
  // TRACING OPTIONS
  // ===========================================================================

  app.add_flag("--trace", opts.trace, "Enable execution tracing");

  app.add_flag("--trace.memory", opts.trace_memory, "Include memory in trace");

  app.add_flag("--trace.returndata", opts.trace_returndata, "Include return data in trace");
}

int run_t8n(const T8nOptions& opts) {
  auto& logger = log::t8n();

  // Log execution overview
  logger.info("State Transition Tool");
  logger.info("  fork: {}, chain_id: {}", opts.fork, opts.chain_id);
  logger.info("  inputs: alloc={}, env={}, txs={}", opts.input_alloc, opts.input_env,
              opts.input_txs);
  logger.info("  outputs: basedir={}, result={}, alloc={}", opts.output_basedir, opts.output_result,
              opts.output_alloc);

  // 1. Parse fork
  auto fork_opt = parse_fork(opts.fork);
  if (!fork_opt) {
    logger.error("Unknown fork '{}'. Supported: Shanghai, Cancun, Prague", opts.fork);
    return static_cast<int>(ExitCode::ConfigError);
  }
  const evm::Fork fork = *fork_opt;

  // 2. Parse inputs
  logger.info("Loading inputs...");
  auto input_result = parse_inputs(opts.input_alloc, opts.input_env, opts.input_txs);
  if (auto* err = std::get_if<ParseError>(&input_result)) {
    logger.error("{}", err->message);
    return err->exit_code;
  }
  auto& input = std::get<InputData>(input_result);
  logger.info("Loaded {} accounts, {} transactions", input.alloc.size(), input.txs.size());

  // 3. Validate env for fork
  if (fork_has_eip4844(fork) && !input.env.excess_blob_gas.has_value()) {
    // Set default if not provided for blob-supporting forks
    input.env.excess_blob_gas = 0;
  }

  // 4. Build state from alloc
  logger.info("Building initial state...");
  auto state = build_state(input.alloc);

  // 5. Execute transactions
  logger.info("Executing {} transactions...", input.txs.size());
  crypto::Secp256k1Context secp_ctx;

  // Build trace config from options
  const TraceConfig trace_config{
      .enabled = opts.trace,
      .output_dir = opts.output_basedir,
      .tracer_config = {.memory = opts.trace_memory, .return_data = opts.trace_returndata}};

  if (opts.trace) {
    logger.info("Tracing enabled (memory={}, returndata={})", opts.trace_memory,
                opts.trace_returndata);
  }

  auto exec_output = execute_transactions(*state, input.txs, input.env, fork, opts.chain_id,
                                          secp_ctx, trace_config);
  logger.info("Executed: {} successful, {} rejected, {} gas used", exec_output.executed_txs.size(),
              exec_output.rejected.size(), exec_output.gas_used);

  // 6. Compute result (roots, bloom, etc.)
  logger.info("Computing state roots...");
  auto result = compute_result(*state, exec_output);

  // 6b. Add additional fields from env for blockchain test compatibility
  if (input.env.difficulty.has_value()) {
    result.current_difficulty = input.env.difficulty;
  }

  // Calculate base fee: use provided value, or calculate from parent parameters
  if (input.env.base_fee.has_value()) {
    result.current_base_fee = input.env.base_fee;
  } else if (input.env.parent_base_fee.has_value() && input.env.parent_gas_used.has_value() &&
             input.env.parent_gas_limit.has_value()) {
    // Calculate current base fee from parent block parameters (EIP-1559)
    result.current_base_fee = ethereum::calc_base_fee(input.env.parent_base_fee.value(),
                                                      input.env.parent_gas_used.value(),
                                                      input.env.parent_gas_limit.value());
  }

  if (input.env.excess_blob_gas.has_value()) {
    result.current_excess_blob_gas = input.env.excess_blob_gas;
  }

  // Compute withdrawals root for Shanghai+ (all supported forks)
  if (!input.env.withdrawals.empty()) {
    std::vector<ethereum::Withdrawal> eth_withdrawals;
    eth_withdrawals.reserve(input.env.withdrawals.size());
    for (const auto& w : input.env.withdrawals) {
      eth_withdrawals.push_back({w.index, w.validator_index, w.address, w.amount});
    }
    result.withdrawals_root = ethereum::compute_withdrawals_root(eth_withdrawals);
  } else {
    // Empty withdrawals still get the empty trie root for Shanghai+
    result.withdrawals_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  // 7. Write outputs
  logger.info("Writing outputs...");
  const int exit_code = write_outputs(opts.output_basedir, opts.output_result, opts.output_alloc,
                                      opts.output_body, result, *state, exec_output.executed_txs);

  if (exit_code == 0) {
    logger.info("Done");
  }
  return exit_code;
}

}  // namespace div0::cli
