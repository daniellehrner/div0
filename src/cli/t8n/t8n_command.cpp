// t8n_command.cpp - State transition tool command implementation

#include "t8n/t8n_command.h"

#include <iostream>

#include "exit_codes.h"

namespace div0::cli {

void setup_t8n_command(CLI::App& app, T8nOptions& opts) {
  // ===========================================================================
  // INPUT OPTIONS  In bash completion, you have access to COMP_WORDS (all words typed so far) and
  // COMP_CWORD (current word index). So you check if a subcommand name appears in the words:

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

int run_t8n(const T8nOptions& /*opts*/) {
  // Implementation in phase-5
  std::cerr << "t8n not yet implemented\n";
  return static_cast<int>(ExitCode::GeneralError);
}

}  // namespace div0::cli
