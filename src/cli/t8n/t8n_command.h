// t8n_command.h - State transition tool subcommand

#ifndef DIV0_CLI_T8N_COMMAND_H
#define DIV0_CLI_T8N_COMMAND_H

/// t8n subcommand options.
typedef struct {
  // Input files
  const char *input_alloc; // Pre-state allocations (default: "alloc.json")
  const char *input_env;   // Block environment (default: "env.json")
  const char *input_txs;   // Transactions (default: "txs.json")

  // Output files
  const char *output_basedir; // Output directory (default: ".")
  const char *output_result;  // Result file (default: "result.json")
  const char *output_alloc;   // Post-state file (default: "alloc.json")
  const char *output_body;    // RLP transactions (optional, TODO: not implemented)

  // State configuration
  const char *fork; // Fork name (default: "Shanghai")
  int chain_id;     // Chain ID (default: 1)
  long reward;      // Block reward, -1 to disable (default: 0)

  // Verbosity
  int verbose; // Print progress messages to stderr (default: 1)
} t8n_options_t;

/// Initialize t8n options with default values.
void t8n_options_init(t8n_options_t *opts);

/// Run the t8n subcommand.
/// @param argc Argument count (includes "t8n" as argv[0])
/// @param argv Argument vector
/// @return Exit code
int cmd_t8n(int argc, const char **argv);

#endif // DIV0_CLI_T8N_COMMAND_H
