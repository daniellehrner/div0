// main.c - div0 CLI entry point

#include "cli/version.h"
#include "crash_handler.h"
#include "exit_codes.h"
#include "t8n/t8n_command.h"

#include <argparse.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Subcommand Dispatch
// ============================================================================

struct cmd_struct {
  const char *cmd;
  int (*fn)(int argc, const char **argv);
};

static struct cmd_struct commands[] = {
    {"t8n", cmd_t8n},
    {nullptr, nullptr},
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, const char **argv) {
  install_crash_handler();

  // clang-format off
  static const char *const usages[] = {
    "div0 [options] [subcommand]",
    nullptr,
  };
  // clang-format on

  int show_version = 0;

  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_BOOLEAN('v', "version", &show_version, "show version", nullptr, 0, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, usages, ARGPARSE_STOP_AT_NON_OPTION);
  argparse_describe(&argparse, "\ndiv0 - High-performance EVM implementation",
                    "\nSubcommands:\n  t8n    Execute state transition");
  argc = argparse_parse(&argparse, argc, argv);

  if (show_version) {
    // Output matches geth's format for execution-spec-tests compatibility
    printf("evm version %s\n", DIV0_VERSION_STRING);
    return DIV0_EXIT_SUCCESS;
  }

  if (argc < 1) {
    // No subcommand - print info
    printf("div0 - High-performance EVM implementation\n");
    printf("Version: %s\n\n", DIV0_VERSION_STRING);
    printf("Use 'div0 --help' for usage information.\n");
    printf("Use 'div0 t8n --help' for state transition tool.\n");
    return DIV0_EXIT_SUCCESS;
  }

  // Dispatch to subcommand (false positive: nullptr sentinel terminates loop)
  // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
  for (struct cmd_struct *p = commands; p->cmd != nullptr; p++) {
    if (strcmp(p->cmd, argv[0]) == 0) {
      return p->fn(argc, argv);
    }
  }

  // NOLINTBEGIN(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  fprintf(stderr, "Unknown subcommand: %s\n", argv[0]);
  fprintf(stderr, "Use 'div0 --help' for usage information.\n");
  // NOLINTEND(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  return DIV0_EXIT_GENERAL_ERROR;
}
