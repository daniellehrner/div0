// t8n_command.h - State transition tool command

#ifndef DIV0_CLI_T8N_COMMAND_H
#define DIV0_CLI_T8N_COMMAND_H

#include <CLI/CLI.hpp>
#include <cstdint>
#include <string>

namespace div0::cli {

/**
 * @brief Options for t8n command.
 */
struct T8nOptions {
  // Input files
  std::string input_alloc = "alloc.json";
  std::string input_env = "env.json";
  std::string input_txs = "txs.json";

  // Output files
  std::string output_basedir = ".";
  std::string output_result = "result.json";
  std::string output_alloc = "alloc.json";
  std::string output_body;  // Optional: RLP transactions

  // State configuration
  std::string fork = "Shanghai";
  uint64_t chain_id = 1;
  int64_t reward = 0;  // -1 to disable

  // Tracing (future)
  bool trace = false;
  bool trace_memory = false;
  bool trace_returndata = false;
};

/**
 * @brief Set up t8n subcommand options.
 *
 * @param app CLI11 subcommand to configure
 * @param opts Options struct to bind to CLI flags
 */
void setup_t8n_command(CLI::App& app, T8nOptions& opts);

/**
 * @brief Execute t8n command.
 *
 * @param opts Parsed command options
 * @return Exit code (0 = success)
 */
int run_t8n(const T8nOptions& opts);

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_COMMAND_H
