// t8n_command.c - State transition tool subcommand implementation

#include "t8n_command.h"

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/signer.h"
#include "div0/evm/block_context.h"
#include "div0/evm/evm.h"
#include "div0/executor/block_executor.h"
#include "div0/json/write.h"
#include "div0/mem/arena.h"
#include "div0/state/state_access.h"
#include "div0/state/world_state.h"
#include "div0/t8n/alloc.h"
#include "div0/t8n/env.h"
#include "div0/t8n/result.h"
#include "div0/t8n/txs.h"
#include "div0/types/uint256.h"

#include "../exit_codes.h"

#include <argparse.h>
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
static const int DEFAULT_CHAIN_ID = 1;
static const long DEFAULT_REWARD = 0;
static const int DEFAULT_VERBOSE = 1;

// Maximum path length for output files
static const size_t MAX_PATH_LEN = 4096;

// ============================================================================
// Execution Context (for cleanup)
// ============================================================================

typedef struct {
  div0_arena_t *arena;
  world_state_t *ws;
  secp256k1_ctx_t *secp_ctx;
  bool arena_initialized;
} t8n_context_t;

static void t8n_context_init(t8n_context_t *ctx) {
  ctx->arena = nullptr;
  ctx->ws = nullptr;
  ctx->secp_ctx = nullptr;
  ctx->arena_initialized = false;
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
  if (ctx->arena_initialized && ctx->arena != nullptr) {
    div0_arena_reset(ctx->arena);
    ctx->arena_initialized = false;
  }
}

// ============================================================================
// Fork Parsing
// ============================================================================

typedef enum {
  FORK_SHANGHAI,
  FORK_CANCUN,
  FORK_PRAGUE,
  FORK_UNKNOWN,
} fork_t;

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

static bool get_block_hash_cb(uint64_t block_number, void *user_data, hash_t *out) {
  block_hash_ctx_t *ctx = (block_hash_ctx_t *)user_data;
  for (size_t i = 0; i < ctx->count; i++) {
    if (ctx->hashes[i].number == block_number) {
      *out = ctx->hashes[i].hash;
      return true;
    }
  }
  return false;
}

// ============================================================================
// State Building
// ============================================================================

static bool build_state_from_alloc(world_state_t *ws, const t8n_alloc_t *alloc) {
  state_access_t *state = world_state_access(ws);

  for (size_t i = 0; i < alloc->account_count; i++) {
    const t8n_alloc_account_t *acc = &alloc->accounts[i];

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
  return true;
}

// ============================================================================
// Path Building
// ============================================================================

// Build a file path from basedir and filename with overflow check.
// Returns false if the path would exceed max_len.
static bool build_path(char *out, size_t max_len, const char *basedir, const char *filename) {
  size_t basedir_len = strlen(basedir);
  size_t filename_len = strlen(filename);

  // Need space for: basedir + "/" + filename + "\0"
  if (basedir_len + 1 + filename_len + 1 > max_len) {
    return false;
  }

  __builtin___memcpy_chk(out, basedir, basedir_len, max_len);
  out[basedir_len] = '/';
  __builtin___memcpy_chk(out + basedir_len + 1, filename, filename_len, max_len - basedir_len - 1);
  out[basedir_len + 1 + filename_len] = '\0';
  return true;
}

// ============================================================================
// Output Writing
// ============================================================================

// Diagnostic output uses fprintf to stderr. Return values are intentionally
// ignored as there's no meaningful recovery for stderr write failures.
// NOLINTBEGIN(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

static int write_result_file(const char *basedir, const char *filename,
                             const t8n_result_t *result) {
  // Build full path with overflow check
  char path[MAX_PATH_LEN];
  if (!build_path(path, sizeof(path), basedir, filename)) {
    fprintf(stderr, "t8n: output path too long: %s/%s\n", basedir, filename);
    return DIV0_EXIT_CONFIG_ERROR;
  }

  json_writer_t writer;
  json_result_t init_result = json_writer_init(&writer);
  if (init_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to init JSON writer\n");
    return DIV0_EXIT_JSON_ERROR;
  }

  yyjson_mut_val_t *root = t8n_write_result(result, &writer);
  if (root == nullptr) {
    fprintf(stderr, "t8n: failed to serialize result\n");
    json_writer_free(&writer);
    return DIV0_EXIT_JSON_ERROR;
  }

  json_result_t write_result = json_write_file(&writer, root, path, JSON_WRITE_PRETTY);
  if (write_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to write %s: %s\n", path,
            write_result.detail ? write_result.detail : json_error_name(write_result.error));
    json_writer_free(&writer);
    return DIV0_EXIT_IO_ERROR;
  }

  json_writer_free(&writer);
  return DIV0_EXIT_SUCCESS;
}

static int write_alloc_file(const char *basedir, const char *filename, world_state_t *ws,
                            div0_arena_t *arena) {
  // Build full path with overflow check
  char path[MAX_PATH_LEN];
  if (!build_path(path, sizeof(path), basedir, filename)) {
    fprintf(stderr, "t8n: output path too long: %s/%s\n", basedir, filename);
    return DIV0_EXIT_CONFIG_ERROR;
  }

  (void)ws;
  (void)arena;

  // TODO: Implement world_state_to_alloc() to export actual post-state.
  // Currently writes an empty object - post-state is NOT preserved!
  fprintf(stderr,
          "t8n: WARNING: post-state alloc output not implemented, writing empty object to %s\n",
          path);

  json_writer_t writer;
  json_result_t init_result = json_writer_init(&writer);
  if (init_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to init JSON writer\n");
    return DIV0_EXIT_JSON_ERROR;
  }

  yyjson_mut_val_t *root = json_write_obj(&writer);

  json_result_t write_result = json_write_file(&writer, root, path, JSON_WRITE_PRETTY);
  if (write_result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to write %s: %s\n", path,
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
    fprintf(stderr, "t8n: unknown fork '%s'. Supported: Shanghai, Cancun, Prague\n", opts.fork);
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

  t8n_alloc_t alloc = {};
  json_result_t result = t8n_parse_alloc_file(opts.input_alloc, &arena, &alloc);
  if (result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_alloc,
            result.detail ? result.detail : json_error_name(result.error));
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_JSON_ERROR;
  }

  t8n_env_t env;
  t8n_env_init(&env);
  result = t8n_parse_env_file(opts.input_env, &arena, &env);
  if (result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_env,
            result.detail ? result.detail : json_error_name(result.error));
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_JSON_ERROR;
  }

  t8n_txs_t txs = {};
  result = t8n_parse_txs_file(opts.input_txs, &arena, &txs);
  if (result.error != JSON_OK) {
    fprintf(stderr, "t8n: failed to parse %s: %s\n", opts.input_txs,
            result.detail ? result.detail : json_error_name(result.error));
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_JSON_ERROR;
  }

  if (opts.verbose) {
    fprintf(stderr, "  loaded: %zu accounts, %zu transactions\n", alloc.account_count,
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

  if (!build_state_from_alloc(ws, &alloc)) {
    fprintf(stderr, "t8n: failed to build state from alloc\n");
    t8n_context_cleanup(&ctx);
    return DIV0_EXIT_GENERAL_ERROR;
  }

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

  // Create EVM
  evm_t *evm = div0_arena_alloc(&arena, sizeof(evm_t));
  evm_init(evm, &arena);

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
    fprintf(stderr, "  executed: %zu successful, %zu rejected, %lu gas used\n",
            exec_result.receipt_count, exec_result.rejected_count,
            (unsigned long)exec_result.gas_used);
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
  if (env.has_base_fee) {
    t8n_result.has_current_base_fee = true;
    t8n_result.current_base_fee = env.base_fee;
  }
  if (fork >= FORK_CANCUN && env.has_excess_blob_gas) {
    t8n_result.has_current_excess_blob_gas = true;
    t8n_result.current_excess_blob_gas = env.excess_blob_gas;
    t8n_result.has_blob_gas_used = true;
    t8n_result.blob_gas_used = exec_result.blob_gas_used;
  }

  // Write outputs
  if (opts.verbose) {
    fprintf(stderr, "t8n: writing outputs...\n");
  }
  int exit_code = write_result_file(opts.output_basedir, opts.output_result, &t8n_result);
  if (exit_code != DIV0_EXIT_SUCCESS) {
    t8n_context_cleanup(&ctx);
    return exit_code;
  }

  exit_code = write_alloc_file(opts.output_basedir, opts.output_alloc, ws, &arena);
  if (exit_code != DIV0_EXIT_SUCCESS) {
    t8n_context_cleanup(&ctx);
    return exit_code;
  }

  if (opts.verbose) {
    fprintf(stderr, "t8n: done\n");
  }

  t8n_context_cleanup(&ctx);
  return DIV0_EXIT_SUCCESS;
}

// NOLINTEND(cert-err33-c,clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
