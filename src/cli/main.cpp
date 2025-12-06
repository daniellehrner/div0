// main.cpp - div0 CLI entry point

#include <CLI/CLI.hpp>
#include <iostream>

#include "crash_handler.h"
#include "t8n/t8n_command.h"
#include "version.h"

// NOLINTBEGIN(bugprone-exception-escape,clang-analyzer-cplusplus.NewDeleteLeaks)
int main(int argc, char** argv) {
  div0::cli::install_crash_handler();

  CLI::App app{"div0 - High-performance EVM implementation"};

  // ===========================================================================
  // GLOBAL OPTIONS
  // ===========================================================================

  // Version flag
  app.set_version_flag("-v,--version", DIV0_VERSION_STRING);

  // Allow running with no subcommand
  app.require_subcommand(0, 1);

  // ===========================================================================
  // SUBCOMMANDS
  // ===========================================================================

  // t8n subcommand
  auto* t8n_cmd = app.add_subcommand("t8n", "Execute state transition");
  div0::cli::T8nOptions t8n_opts;
  div0::cli::setup_t8n_command(*t8n_cmd, t8n_opts);

  // ===========================================================================
  // PARSE AND EXECUTE
  // ===========================================================================

  CLI11_PARSE(app, argc, argv);

  if (app.got_subcommand("t8n")) {
    return div0::cli::run_t8n(t8n_opts);
  }

  // Check if any unknown subcommand was parsed
  if (!app.get_subcommands().empty()) {
    std::cerr << "Unknown subcommand\n";
    return 1;
  }

  // Default: print basic info
  std::cout << "div0 - High-performance EVM implementation\n";
  std::cout << "Version: " << DIV0_VERSION_STRING << "\n";
  std::cout << "\nUse 'div0 --help' for usage information.\n";
  std::cout << "Use 'div0 t8n --help' for state transition tool.\n";

  return 0;
}
// NOLINTEND(bugprone-exception-escape,clang-analyzer-cplusplus.NewDeleteLeaks)
