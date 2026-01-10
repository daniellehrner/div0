// t8n_command.c - State transition tool subcommand implementation

#include "t8n_command.h"

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/signer.h"
#include "div0/evm/block_context.h"
#include "div0/evm/evm.h"
#include "div0/executor/block_executor.h"
#include "div0/json/parse.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"
#include "div0/state/state_access.h"
#include "div0/state/world_state.h"
#include "div0/t8n/alloc.h"
#include "div0/t8n/env.h"
#include "div0/t8n/result.h"
#include "div0/t8n/txs.h"
#include "div0/trie/node.h"
#include "div0/types/uint256.h"

#include "../exit_codes.h"

#include <argparse.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Default Values
// ============================================================================

static const char *const DEFAULT_INPUT_ALLOC = "alloc.json";
static const char *const DEFAULT_INPUT_ENV = "env.json";
static const char *const DEFAULT_INPUT_TXS = "txs.json";
static const char *const DEFAULT_OUTPUT_BASEDIR = ".";
static const char *const DEFAULT_OUTPUT_RESULT = "result.json";
static const char *const DEFAULT_OUTPUT_ALLOC = "alloc.json";
static const char *const DEFAULT_FORK = "Shanghai";
static constexpr int DEFAULT_CHAIN_ID = 1;
static constexpr long DEFAULT_REWARD = 0;
static constexpr int DEFAULT_VERBOSE = 1;

// Maximum path length for output files.
// 4096 matches PATH_MAX on Linux and is a reasonable upper bound for other platforms.
static constexpr size_t MAX_PATH_LEN = 4096;

// Initial buffer size for reading stdin
static constexpr size_t STDIN_INITIAL_BUFFER_SIZE = 65536;

// ============================================================================
// Stdin/Stdout Helpers
// ============================================================================

/// Check if path refers to stdin
static bool is_stdin(const char *path) {
  return strcmp(path, "stdin") == 0;
}

/// Check if path refers to stdout
static bool is_stdout(const char *path) {
  return strcmp(path, "stdout") == 0;
}

/// Read all of stdin into a malloc'd buffer.
/// Returns nullptr on error. Caller must free the returned buffer.
static char *read_stdin(size_t *out_len) {
  size_t capacity = STDIN_INITIAL_BUFFER_SIZE;
  size_t len = 0;
  char *buffer = malloc(capacity);
  if (buffer == nullptr) {
    return nullptr;
  }

  while (!feof(stdin)) {
    size_t space = capacity - len;
    if (space < 1024) {
      // Check for overflow before doubling capacity
      if (capacity > SIZE_MAX / 2) {
        free(buffer);
        return nullptr;
      }
      capacity *= 2;
      char *new_buffer = realloc(buffer, capacity);
      if (new_buffer == nullptr) {
        free(buffer);
        return nullptr;
      }
      buffer = new_buffer;
      space = capacity - len;
    }
    const size_t bytes_read = fread(buffer + len, 1, space, stdin);
    len += bytes_read;
    if (ferror(stdin)) {
      free(buffer);
      return nullptr;
    }
  }

  *out_len = len;
  return buffer;
}

// ============================================================================
// Execution Context (for cleanup)
// ============================================================================

typedef struct {
  div0_arena_t *arena;
  world_state_t *ws;
  secp256k1_ctx_t *secp_ctx;
  char *stdin_buffer;   // malloc'd stdin buffer (needs free)
  json_doc_t stdin_doc; // parsed stdin document
  bool arena_initialized;
  bool stdin_doc_valid;
} t8n_context_t;

static void t8n_context_init(t8n_context_t *ctx) {
  ctx->arena = nullptr;
  ctx->ws = nullptr;
  ctx->secp_ctx = nullptr;
  ctx->stdin_buffer = nullptr;
  ctx->stdin_doc.doc = nullptr;
  ctx->arena_initialized = false;
  ctx->stdin_doc_valid = false;
}

static void t8n_context_cleanup(t8n_context_t *ctx) {
  if (ctx->secp_ctx != nullptr) {
    secp256k1_ctx_destroy(ctx->secp_ctx);
    ctx->secp_ctx = nullptr;
  }
  if (ctx->ws != nullptr) {
    world_state_destroy(ctx->ws);
    ctx->ws = nullptr;
  }
  if (ctx->stdin_doc_valid) {
    json_doc_free(&ctx->stdin_doc);
    ctx->stdin_doc_valid = false;
  }
  if (ctx->stdin_buffer != nullptr) {
    free(ctx->stdin_buffer);
    ctx->stdin_buffer = nullptr;
  }
  // arena is stack-allocated, destroy frees all malloc'd blocks
  if (ctx->arena_initialized) {
    div0_arena_destroy(ctx->arena);
    ctx->arena_initialized = false;
  }
}

// ============================================================================
// Fork Parsing
// ============================================================================

static fork_t parse_fork(const char *name) {
  if (strcmp(name, "Shanghai") == 0) {
    return FORK_SHANGHAI;
  }
  if (strcmp(name, "Cancun") == 0) {
    return FORK_CANCUN;
  }
  if (strcmp(name, "Prague") == 0) {
    return FORK_PRAGUE;
  }
  return FORK_UNKNOWN;
}

// ============================================================================
// Block Hash Callback
// ============================================================================

typedef struct {
  const t8n_block_hash_t *hashes;
  size_t count;
} block_hash_ctx_t;

static bool get_block_hash_cb(const uint64_t block_number, void *const user_data,
                              hash_t *const out) {
  const auto ctx = (block_hash_ctx_t *)user_data;
  for (size_t i = 0; i < ctx->count; i++) {
    if (ctx->hashes[i].number == block_number) {
      *out = ctx->hashes[i].hash;
      return true;
    }
  }
  // Defensive: set output to zero hash even though caller should check return value
  *out = hash_zero();
  return false;
}

// ============================================================================
// State Building
// ============================================================================

static void build_state_from_snapshot(world_state_t *ws, const state_snapshot_t *snapshot) {
  state_access_t *state = world_state_access(ws);

  for (size_t i = 0; i < snapshot->account_count; i++) {
    const account_snapshot_t *acc = &snapshot->accounts[i];

    // Set balance
    state_set_balance(state, &acc->address, acc->balance);

    // Set nonce
    if (acc->nonce > 0) {
      state_set_nonce(state, &acc->address, acc->nonce);
    }

    // Set code if present
    if (acc->code.data != nullptr && acc->code.size > 0) {
      state_set_code(state, &acc->address, acc->code.data, acc->code.size);
    }

    // Set storage
    for (size_t j = 0; j < acc->storage_count; j++) {
      state_set_storage(state, &acc->address, acc->storage[j].slot, acc->storage[j].value);
    }
  }
}

// ============================================================================
// Path Building
// ============================================================================

// Build a file path from basedir and filename with overflow check.
// Returns false if the path would exceed MAX_PATH_LEN.
static bool build_path(char *const out, const char *const basedir, const char *const filename) {
  const size_t basedir_len = strlen(basedir);
  const size_t filename_len = strlen(filename);

  // Need space for: basedir + "/" + filename + "\0"
  if (basedir_len + 1 + filename_len + 1 > MAX_PATH_LEN) {
    return false;
  }

  __builtin___memcpy_chk(out, basedir, basedir_len, MAX_PATH_LEN);
  out[basedir_len] = '/';
  __builtin___memcpy_chk(out + basedir_len + 1, filename, filename_len,
                         MAX_PATH_LEN - basedir_len - 1);
  out[basedir_len + 1 + filename_len] = '\0';
  return true;
}

// ============================================================================
// Output Writing
// ============================================================================

// Diagnostic output uses fprintf to stderr. Return values are intentionally
// ignored as there's no meaningful recovery for stderr write failures.
// NOLINTBEGIN(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

/// Write JSON to stdout or file. Caller retains ownership of writer.
/// @param basedir Base directory for file output
/// @param filename Output filename or "stdout"
/// @param root JSON value to write
/// @param writer JSON writer (caller must free)
/// @param what Description for error messages (e.g., "result", "alloc")
/// @return Exit code
static int write_json_to_output(const char *const basedir, const char *const filename,
                                yyjson_mut_val_t *const root, const json_writer_t *const writer,
                                const char *const what) {
  json_result_t write_result;
  if (is_stdout(filename)) {
    write_result = json_write_fp(writer, root, stdout, JSON_WRITE_PRETTY);
    if (write_result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to write %s to stdout: %s\n", what,
              write_result.detail ? write_result.detail : json_error_name(write_result.error));
      return DIV0_EXIT_IO_ERROR;
    }
  } else {
    char path[MAX_PATH_LEN];
    if (!build_path(path, basedir, filename)) {
      fprintf(stderr, "t8n: output path too long: %s/%s\n", basedir, filename);
      return DIV0_EXIT_CONFIG_ERROR;
    }
    write_result = json_write_file(writer, root, path, JSON_WRITE_PRETTY);
    if (write_result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to write %s: %s\n", path,
              write_result.detail ? write_result.detail : json_error_name(write_result.error));
      return DIV0_EXIT_IO_ERROR;
    }
  }
  return DIV0_EXIT_SUCCESS;
}

static int write_result_output(const char *basedir, const char *filename,
                               const t8n_result_t *result) {
  json_writer_t writer;
  if (json_writer_init(&writer).error != JSON_OK) {
    fprintf(stderr, "t8n: failed to init JSON writer\n");
    return DIV0_EXIT_JSON_ERROR;
  }

  yyjson_mut_val_t *root = t8n_write_result(result, &writer);
  if (root == nullptr) {
    fprintf(stderr, "t8n: failed to serialize result\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }

  const int exit_code = write_json_to_output(basedir, filename, root, &writer, "result");
  json_writer_free(&writer);
  return exit_code;
}

static int write_alloc_output(const char *basedir, const char *filename, world_state_t *ws,
                              div0_arena_t *arena) {
  state_snapshot_t snapshot = {};
  if (!world_state_snapshot(ws, arena, &snapshot)) {
    fprintf(stderr, "t8n: failed to export post-state\n");
    return DIV0_EXIT_GENERAL_ERROR;
  }

  json_writer_t writer;
  if (json_writer_init(&writer).error != JSON_OK) {
    fprintf(stderr, "t8n: failed to init JSON writer\n");
    return DIV0_EXIT_JSON_ERROR;
  }

  yyjson_mut_val_t *root = t8n_write_alloc(&snapshot, &writer);
  if (root == nullptr) {
    fprintf(stderr, "t8n: failed to serialize post-state\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }

  const int exit_code = write_json_to_output(basedir, filename, root, &writer, "alloc");
  json_writer_free(&writer);
  return exit_code;
}

/// Write combined result+alloc to stdout in geth stream format.
///
/// Format: {"result": {...}, "alloc": {...}, "body": "0x"}
static int write_combined_stdout(const t8n_result_t *result, world_state_t *ws,
                                 div0_arena_t *arena) {
  // Export post-state
  state_snapshot_t snapshot = {};
  if (!world_state_snapshot(ws, arena, &snapshot)) {
    fprintf(stderr, "t8n: failed to export post-state\n");
    return DIV0_EXIT_GENERAL_ERROR;
  }

  json_writer_t writer;
  const json_result_t init_result = json_writer_init(&writer);
  if (init_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to init JSON writer\n");
    return DIV0_EXIT_JSON_ERROR;
  }

  // Create combined object
  yyjson_mut_val_t *root = json_write_obj(&writer);
  if (root == nullptr) {
    fprintf(stderr, "t8n: failed to create combined output\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }

  // Add result sub-object
  yyjson_mut_val_t *result_obj = t8n_write_result(result, &writer);
  if (result_obj == nullptr) {
    fprintf(stderr, "t8n: failed to serialize result\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }
  json_obj_add(&writer, root, "result", result_obj);

  // Add alloc sub-object
  yyjson_mut_val_t *alloc_obj = t8n_write_alloc(&snapshot, &writer);
  if (alloc_obj == nullptr) {
    fprintf(stderr, "t8n: failed to serialize alloc\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }
  // NOLINTNEXTLINE(readability-suspicious-call-argument) - false positive
  json_obj_add(&writer, root, "alloc", alloc_obj);

  // Add empty body (RLP encoding not implemented)
  json_obj_add_str(&writer, root, "body", "0x");

  // Write to stdout
  const json_result_t write_result = json_write_fp(&writer, root, stdout, JSON_WRITE_PRETTY);
  if (write_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to write combined output to stdout: %s\n",
            write_result.detail ? write_result.detail : json_error_name(write_result.error));
    json_writer_free(&writer);
    return DIV0_EXIT_IO_ERROR;
  }

  json_writer_free(&writer);
  return DIV0_EXIT_SUCCESS;
}

// NOLINTEND(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

// ============================================================================
// Main Execution
// ============================================================================

void t8n_options_init(t8n_options_t *opts) {
  opts->input_alloc = DEFAULT_INPUT_ALLOC;
  opts->input_env = DEFAULT_INPUT_ENV;
  opts->input_txs = DEFAULT_INPUT_TXS;
  opts->output_basedir = DEFAULT_OUTPUT_BASEDIR;
  opts->output_result = DEFAULT_OUTPUT_RESULT;
  opts->output_alloc = DEFAULT_OUTPUT_ALLOC;
  opts->output_body = nullptr;
  opts->fork = DEFAULT_FORK;
  opts->chain_id = DEFAULT_CHAIN_ID;
  opts->reward = DEFAULT_REWARD;
  opts->verbose = DEFAULT_VERBOSE;
}

// Diagnostic output uses fprintf to stderr. Return values are intentionally
// ignored as there's no meaningful recovery for stderr write failures.
// NOLINTBEGIN(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

int cmd_t8n(int argc, const char **argv) {
  t8n_options_t opts;
  t8n_options_init(&opts);

  // clang-format off
  static const char *const usages[] = {
    "div0 t8n [options]",
    nullptr,
  };
  // clang-format on

  int quiet = 0;
  // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_BOOLEAN('q', "quiet", &quiet, "Suppress progress messages", nullptr, 0, 0),
      OPT_GROUP("Input options"),
      OPT_STRING(0, "input.alloc", &opts.input_alloc, "Input allocations file", nullptr, 0, 0),
      OPT_STRING(0, "input.env", &opts.input_env, "Input environment file", nullptr, 0, 0),
      OPT_STRING(0, "input.txs", &opts.input_txs, "Input transactions file", nullptr, 0, 0),
      OPT_GROUP("Output options"),
      OPT_STRING(0, "output.basedir", &opts.output_basedir, "Output directory", nullptr, 0, 0),
      OPT_STRING(0, "output.result", &opts.output_result, "Result output file", nullptr, 0, 0),
      OPT_STRING(0, "output.alloc", &opts.output_alloc, "Post-state output file", nullptr, 0, 0),
      // TODO: --output.body for RLP-encoded transactions is not yet implemented
      OPT_STRING(0, "output.body", &opts.output_body, "RLP transactions output (NOT IMPLEMENTED)",
                 nullptr, 0, 0),
      OPT_GROUP("State options"),
      OPT_STRING(0, "state.fork", &opts.fork, "Fork name (Shanghai, Cancun, Prague)", nullptr, 0,
                 0),
      OPT_INTEGER(0, "state.chainid", &opts.chain_id, "Chain ID", nullptr, 0, 0),
      OPT_INTEGER(0, "state.reward", &opts.reward, "Block reward (-1 to disable)", nullptr, 0, 0),
      OPT_END(),
  };
  // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argparse_describe(&argparse, "\nExecute state transition on input data.", nullptr);
  argc = argparse_parse(&argparse, argc, argv);
  (void)argc; // Remaining argc not used after parsing

  // Apply quiet flag to verbose setting
  opts.verbose = !quiet;

  // Validate fork
  fork_t fork = parse_fork(opts.fork);
  if (fork == FORK_UNKNOWN) {
    fprintf(stderr, "t8n: ERROR: unknown fork '%s'. Supported: Shanghai, Cancun, Prague\n",
            opts.fork);
    return DIV0_EXIT_CONFIG_ERROR;
  }

  // Warn if --output.body is specified (not implemented)
  if (opts.output_body != nullptr) {
    fprintf(stderr, "t8n: WARNING: --output.body is not implemented, ignoring\n");
  }

  // Initialize execution context for cleanup
  t8n_context_t ctx;
  t8n_context_init(&ctx);

  // Create arena for allocations
  div0_arena_t arena;
  if (!div0_arena_init(&arena)) {
    fprintf(stderr, "t8n: failed to create arena\n");
    return DIV0_EXIT_GENERAL_ERROR;
  }
  ctx.arena = &arena;
  ctx.arena_initialized = true;

  // Parse input files
  if (opts.verbose) {
    fprintf(stderr, "t8n: loading inputs...\n");
    fprintf(stderr, "  fork: %s, chain_id: %d\n", opts.fork, opts.chain_id);
    fprintf(stderr, "  inputs: alloc=%s, env=%s, txs=%s\n", opts.input_alloc, opts.input_env,
            opts.input_txs);
    fprintf(stderr, "  outputs: basedir=%s, result=%s, alloc=%s\n", opts.output_basedir,
            opts.output_result, opts.output_alloc);
  }

  state_snapshot_t pre_state = {};
  t8n_env_t env;
  t8n_env_init(&env);
  t8n_txs_t txs = {};
  json_result_t result;

  // Check if all inputs are from stdin (combined JSON mode)
  bool all_stdin =
      is_stdin(opts.input_alloc) && is_stdin(opts.input_env) && is_stdin(opts.input_txs);

  if (all_stdin) {
    // Read combined JSON from stdin: {"alloc": {...}, "env": {...}, "txs": [...]}
    size_t stdin_len = 0;
    ctx.stdin_buffer = read_stdin(&stdin_len);
    if (ctx.stdin_buffer == nullptr) {
      fprintf(stderr, "t8n: failed to read stdin\n");
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_IO_ERROR;
    }

    // NOLINTNEXTLINE(clang-analyzer-unix.Malloc) - cleanup frees stdin_buffer
    result = json_parse(ctx.stdin_buffer, stdin_len, &ctx.stdin_doc);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse stdin: %s\n",
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
    ctx.stdin_doc_valid = true;

    yyjson_val_t *root = json_doc_root(&ctx.stdin_doc);
    if (!json_is_obj(root)) {
      fprintf(stderr, "t8n: stdin must be a JSON object with alloc, env, txs keys\n");
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }

    // Parse alloc from stdin
    yyjson_val_t *alloc_val = json_obj_get(root, "alloc");
    if (alloc_val == nullptr) {
      fprintf(stderr, "t8n: missing 'alloc' key in stdin JSON\n");
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
    result = t8n_parse_alloc_value(alloc_val, &arena, &pre_state);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse alloc from stdin: %s\n",
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }

    // Parse env from stdin
    yyjson_val_t *env_val = json_obj_get(root, "env");
    if (env_val == nullptr) {
      fprintf(stderr, "t8n: missing 'env' key in stdin JSON\n");
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
    result = t8n_parse_env_value(env_val, &arena, &env);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse env from stdin: %s\n",
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }

    // Parse txs from stdin
    yyjson_val_t *txs_val = json_obj_get(root, "txs");
    if (txs_val == nullptr) {
      fprintf(stderr, "t8n: missing 'txs' key in stdin JSON\n");
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
    result = t8n_parse_txs_value(txs_val, &arena, &txs);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse txs from stdin: %s\n",
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
  } else {
    // Parse from individual files
    result = t8n_parse_alloc_file(opts.input_alloc, &arena, &pre_state);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_alloc,
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }

    result = t8n_parse_env_file(opts.input_env, &arena, &env);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_env,
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }

    result = t8n_parse_txs_file(opts.input_txs, &arena, &txs);
    if (result.error != JSON_OK) {
      fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_txs,
              result.detail ? result.detail : json_error_name(result.error));
      t8n_context_cleanup(&ctx);
      return DIV0_EXIT_JSON_ERROR;
    }
  }

  if (opts.verbose) {
    fprintf(stderr, "  loaded: %zu accounts, %zu transactions\n", pre_state.account_count,
            txs.tx_count);
  }

  // Build initial world state
  if (opts.verbose) {
    fprintf(stderr, "t8n: building initial state...\n");
  }
  world_state_t *ws = world_state_create(&arena);
  if (ws == nullptr) {
    fprintf(stderr, "t8n: failed to create world state\n");
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_GENERAL_ERROR;
  }
  ctx.ws = ws;
  build_state_from_snapshot(ws, &pre_state);

  // Build block context from env
  block_context_t block_ctx;
  block_context_init(&block_ctx);
  block_ctx.number = env.number;
  block_ctx.timestamp = env.timestamp;
  block_ctx.gas_limit = env.gas_limit;
  block_ctx.chain_id = (uint64_t)opts.chain_id;
  block_ctx.coinbase = env.coinbase;

  if (env.has_base_fee) {
    block_ctx.base_fee = env.base_fee;
  }
  if (env.has_prev_randao) {
    block_ctx.prev_randao = env.prev_randao;
  }

  // Set up block hash callback
  block_hash_ctx_t hash_ctx = {.hashes = env.block_hashes, .count = env.block_hash_count};
  block_ctx.get_block_hash = get_block_hash_cb;
  block_ctx.block_hash_user_data = &hash_ctx;

  // Create EVM (large struct, requires 64-byte alignment for cache-line aligned fields)
  evm_t *evm = div0_arena_alloc_large(&arena, sizeof(evm_t), 64);
  if (evm == nullptr) {
    fprintf(stderr, "t8n: failed to allocate EVM\n");
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_GENERAL_ERROR;
  }
  evm_init(evm, &arena, fork);

  // Initialize secp256k1 context for signature recovery
  secp256k1_ctx_t *secp_ctx = secp256k1_ctx_create();
  if (secp_ctx == nullptr) {
    fprintf(stderr, "t8n: failed to create secp256k1 context\n");
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_GENERAL_ERROR;
  }
  ctx.secp_ctx = secp_ctx;

  // Build transaction array with sender recovery
  if (opts.verbose) {
    fprintf(stderr, "t8n: executing %zu transactions...\n", txs.tx_count);
  }
  block_tx_t *block_txs = div0_arena_alloc(&arena, txs.tx_count * sizeof(block_tx_t));
  for (size_t i = 0; i < txs.tx_count; i++) {
    block_txs[i].tx = &txs.txs[i];
    block_txs[i].original_index = i;

    // Recover sender from signature
    ecrecover_result_t recover_result = transaction_recover_sender(secp_ctx, &txs.txs[i], &arena);
    if (recover_result.success) {
      block_txs[i].sender = recover_result.address;
      block_txs[i].sender_recovered = true;
    } else {
      // Failed to recover sender - will be rejected
      block_txs[i].sender = address_zero();
      block_txs[i].sender_recovered = false;
    }
  }

  // Execute transactions
  block_executor_t executor;
  block_executor_init(&executor, world_state_access(ws), &block_ctx, evm, &arena,
                      (uint64_t)opts.chain_id);

  block_exec_result_t exec_result;
  if (!block_executor_run(&executor, block_txs, txs.tx_count, &exec_result)) {
    fprintf(stderr, "t8n: block execution failed\n");
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_EVM_ERROR;
  }

  // Apply block reward if enabled (reward >= 0)
  if (opts.reward >= 0) {
    uint256_t reward = uint256_from_u64((uint64_t)opts.reward);
    state_access_t *state = world_state_access(ws);
    uint256_t current_balance = state_get_balance(state, &env.coinbase);
    uint256_t new_balance = uint256_add(current_balance, reward);
    state_set_balance(state, &env.coinbase, new_balance);
    if (opts.verbose && opts.reward > 0) {
      fprintf(stderr, "  applied block reward: %ld wei to coinbase\n", opts.reward);
    }
  }

  if (opts.verbose) {
    fprintf(stderr, "  executed: %zu successful, %zu rejected, %" PRIu64 " gas used\n",
            exec_result.receipt_count, exec_result.rejected_count, exec_result.gas_used);
  }

  // Build t8n result
  t8n_result_t t8n_result;
  t8n_result_init(&t8n_result);

  t8n_result.state_root = exec_result.state_root;
  t8n_result.gas_used = exec_result.gas_used;

  // TODO: Compute tx_root, receipts_root, logs_hash, logs_bloom
  // For now, use empty values
  t8n_result.tx_root = hash_zero();
  t8n_result.receipts_root = hash_zero();
  t8n_result.logs_hash = hash_zero();
  __builtin___memset_chk(t8n_result.logs_bloom, 0, sizeof(t8n_result.logs_bloom),
                         sizeof(t8n_result.logs_bloom));

  // Copy receipts
  t8n_result.receipt_count = exec_result.receipt_count;
  if (exec_result.receipt_count > 0) {
    t8n_result.receipts =
        div0_arena_alloc(&arena, exec_result.receipt_count * sizeof(t8n_receipt_t));
    for (size_t i = 0; i < exec_result.receipt_count; i++) {
      exec_receipt_t *r = &exec_result.receipts[i];
      t8n_receipt_t *tr = &t8n_result.receipts[i];
      tr->type = r->tx_type;
      tr->tx_hash = r->tx_hash;
      tr->transaction_index = i;
      tr->gas_used = r->gas_used;
      tr->cumulative_gas = r->cumulative_gas;
      tr->status = r->success;
      __builtin___memset_chk(tr->bloom, 0, sizeof(tr->bloom), sizeof(tr->bloom));
      tr->logs = nullptr;
      tr->log_count = 0;
      tr->contract_address = r->created_address;
    }
  }

  // Copy rejected transactions
  t8n_result.rejected_count = exec_result.rejected_count;
  if (exec_result.rejected_count > 0) {
    t8n_result.rejected =
        div0_arena_alloc(&arena, exec_result.rejected_count * sizeof(t8n_rejected_tx_t));
    for (size_t i = 0; i < exec_result.rejected_count; i++) {
      t8n_result.rejected[i].index = exec_result.rejected[i].index;
      t8n_result.rejected[i].error = exec_result.rejected[i].error_message;
    }
  }

  // Set optional fields from env
  if (env.has_difficulty) {
    t8n_result.has_current_difficulty = true;
    t8n_result.current_difficulty = env.difficulty;
  }

  // Base fee is required for EIP-1559+ forks (London onwards, including Shanghai)
  // All currently supported forks (Shanghai, Cancun, Prague) require base fee
  t8n_result.has_current_base_fee = true;
  if (env.has_base_fee) {
    t8n_result.current_base_fee = env.base_fee;
  } else {
    // Default base fee matching execution-spec-tests framework (0x7 = 7 wei)
    t8n_result.current_base_fee = uint256_from_u64(7);
  }
  if (fork >= FORK_CANCUN && env.has_excess_blob_gas) {
    t8n_result.has_current_excess_blob_gas = true;
    t8n_result.current_excess_blob_gas = env.excess_blob_gas;
    t8n_result.has_blob_gas_used = true;
    t8n_result.blob_gas_used = exec_result.blob_gas_used;
  }

  // Withdrawals root is required for Shanghai+ forks (EIP-4895)
  // All currently supported forks require withdrawals root
  // TODO: Compute actual root from withdrawals list when implemented
  // For now, use empty trie root (keccak256(RLP([])))
  t8n_result.has_withdrawals_root = true;
  t8n_result.withdrawals_root = MPT_EMPTY_ROOT;

  // Write outputs
  if (opts.verbose) {
    fprintf(stderr, "t8n: writing outputs...\n");
  }

  int exit_code;
  // When both result and alloc go to stdout, write combined JSON
  bool both_stdout = is_stdout(opts.output_result) && is_stdout(opts.output_alloc);
  if (both_stdout) {
    exit_code = write_combined_stdout(&t8n_result, ws, &arena);
    if (exit_code != DIV0_EXIT_SUCCESS) {
      t8n_context_cleanup(&ctx);
      return exit_code;
    }
  } else {
    exit_code = write_result_output(opts.output_basedir, opts.output_result, &t8n_result);
    if (exit_code != DIV0_EXIT_SUCCESS) {
      t8n_context_cleanup(&ctx);
      return exit_code;
    }

    exit_code = write_alloc_output(opts.output_basedir, opts.output_alloc, ws, &arena);
    if (exit_code != DIV0_EXIT_SUCCESS) {
      t8n_context_cleanup(&ctx);
      return exit_code;
    }
  }

  if (opts.verbose) {
    fprintf(stderr, "t8n: done\n");
  }

  t8n_context_cleanup(&ctx);
  return DIV0_EXIT_SUCCESS;
}

// NOLINTEND(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
