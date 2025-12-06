// t8n_command_test.cpp - Tests for t8n CLI command

#include "t8n/t8n_command.h"

#include <gtest/gtest.h>

#include "exit_codes.h"

namespace div0::cli {
namespace {

// =============================================================================
// T8nOptions Default Values
// =============================================================================

TEST(T8nOptionsTest, DefaultInputFiles) {
  const T8nOptions opts;

  EXPECT_EQ(opts.input_alloc, "alloc.json");
  EXPECT_EQ(opts.input_env, "env.json");
  EXPECT_EQ(opts.input_txs, "txs.json");
}

TEST(T8nOptionsTest, DefaultOutputFiles) {
  const T8nOptions opts;

  EXPECT_EQ(opts.output_basedir, ".");
  EXPECT_EQ(opts.output_result, "result.json");
  EXPECT_EQ(opts.output_alloc, "alloc.json");
  EXPECT_TRUE(opts.output_body.empty());
}

TEST(T8nOptionsTest, DefaultStateConfig) {
  const T8nOptions opts;

  EXPECT_EQ(opts.fork, "Shanghai");
  EXPECT_EQ(opts.chain_id, 1);
  EXPECT_EQ(opts.reward, 0);
}

TEST(T8nOptionsTest, DefaultTraceFlags) {
  const T8nOptions opts;

  EXPECT_FALSE(opts.trace);
  EXPECT_FALSE(opts.trace_memory);
  EXPECT_FALSE(opts.trace_returndata);
}

// =============================================================================
// setup_t8n_command Tests
// =============================================================================

TEST(SetupT8nCommandTest, RegistersInputOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  // Check options exist
  EXPECT_NE(app.get_option("--input.alloc"), nullptr);
  EXPECT_NE(app.get_option("--input.env"), nullptr);
  EXPECT_NE(app.get_option("--input.txs"), nullptr);
}

TEST(SetupT8nCommandTest, RegistersOutputOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  EXPECT_NE(app.get_option("--output.basedir"), nullptr);
  EXPECT_NE(app.get_option("--output.result"), nullptr);
  EXPECT_NE(app.get_option("--output.alloc"), nullptr);
  EXPECT_NE(app.get_option("--output.body"), nullptr);
}

TEST(SetupT8nCommandTest, RegistersStateOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  EXPECT_NE(app.get_option("--state.fork"), nullptr);
  EXPECT_NE(app.get_option("--state.chainid"), nullptr);
  EXPECT_NE(app.get_option("--state.reward"), nullptr);
}

TEST(SetupT8nCommandTest, RegistersTraceOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  EXPECT_NE(app.get_option("--trace"), nullptr);
  EXPECT_NE(app.get_option("--trace.memory"), nullptr);
  EXPECT_NE(app.get_option("--trace.returndata"), nullptr);
}

TEST(SetupT8nCommandTest, ParsesInputOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  std::vector<std::string> args = {"--input.alloc=custom_alloc.json", "--input.env=custom_env.json",
                                   "--input.txs=custom_txs.json"};
  app.parse(args);

  EXPECT_EQ(opts.input_alloc, "custom_alloc.json");
  EXPECT_EQ(opts.input_env, "custom_env.json");
  EXPECT_EQ(opts.input_txs, "custom_txs.json");
}

TEST(SetupT8nCommandTest, ParsesOutputOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  std::vector<std::string> args = {"--output.basedir=/tmp", "--output.result=out.json",
                                   "--output.body=txs.rlp"};
  app.parse(args);

  EXPECT_EQ(opts.output_basedir, "/tmp");
  EXPECT_EQ(opts.output_result, "out.json");
  EXPECT_EQ(opts.output_body, "txs.rlp");
}

TEST(SetupT8nCommandTest, ParsesStateOptions) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  std::vector<std::string> args = {"--state.fork=Cancun", "--state.chainid=137",
                                   "--state.reward=-1"};
  app.parse(args);

  EXPECT_EQ(opts.fork, "Cancun");
  EXPECT_EQ(opts.chain_id, 137);
  EXPECT_EQ(opts.reward, -1);
}

TEST(SetupT8nCommandTest, ParsesTraceFlags) {
  CLI::App app{"test"};
  T8nOptions opts;
  setup_t8n_command(app, opts);

  std::vector<std::string> args = {"--trace", "--trace.memory", "--trace.returndata"};
  app.parse(args);

  EXPECT_TRUE(opts.trace);
  EXPECT_TRUE(opts.trace_memory);
  EXPECT_TRUE(opts.trace_returndata);
}

// =============================================================================
// run_t8n Tests
// =============================================================================

TEST(RunT8nTest, ReturnsGeneralErrorNotImplemented) {
  const T8nOptions opts;

  // Currently returns GeneralError because it's not yet implemented
  const int result = run_t8n(opts);
  EXPECT_EQ(result, static_cast<int>(ExitCode::GeneralError));
}

// =============================================================================
// ExitCode Tests
// =============================================================================

TEST(ExitCodeTest, SuccessIsZero) { EXPECT_EQ(static_cast<int>(ExitCode::Success), 0); }

TEST(ExitCodeTest, GeneralErrorIsOne) { EXPECT_EQ(static_cast<int>(ExitCode::GeneralError), 1); }

TEST(ExitCodeTest, AllCodesHaveDistinctValues) {
  // Verify no duplicate values
  std::set<int> codes;
  codes.insert(static_cast<int>(ExitCode::Success));
  codes.insert(static_cast<int>(ExitCode::GeneralError));
  codes.insert(static_cast<int>(ExitCode::EvmError));
  codes.insert(static_cast<int>(ExitCode::ConfigError));
  codes.insert(static_cast<int>(ExitCode::MissingBlockHash));
  codes.insert(static_cast<int>(ExitCode::JsonError));
  codes.insert(static_cast<int>(ExitCode::IoError));
  codes.insert(static_cast<int>(ExitCode::RlpError));

  EXPECT_EQ(codes.size(), 8);  // All 8 codes should be unique
}

}  // namespace
}  // namespace div0::cli
