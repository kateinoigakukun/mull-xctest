#include "XCTestRunInvocation.h"
#include <llvm/Support/CommandLine.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Reporters/IDEReporter.h>
#include <mull/Version.h>
#include <vector>
#include <libxml/xmlversion.h>

using namespace llvm::cl;
using namespace llvm;

opt<std::string> TestRunFile(Positional, desc("<xctestrun file>"), Required,
                             value_desc("path"));

list<std::string> XcodeBuildArgs(
  "Xxcodebuild",
  desc("Pass <arg> to the xcodebuild"),
  ZeroOrMore,
  value_desc("arg"));

opt<std::string> TestTarget("test-target", desc("test target name"), Required,
                            value_desc("name"));

int main(int argc, char **argv) {
  bool validOptions =
      llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  LIBXML_TEST_VERSION

  mull::Diagnostics diagnostics;
  mull::MutatorsFactory factory(diagnostics);
  mull::Configuration configuration;
  configuration.debugEnabled = true;
  configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
  configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
  configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
  diagnostics.enableDebugMode();

  factory.init();
  mull_xctest::XCTestRunInvocation invocation(TestRunFile, TestTarget, XcodeBuildArgs,
                                              factory, diagnostics,
                                              configuration);
  mull::IDEReporter reporter(diagnostics);
  auto results = invocation.run();
  reporter.reportResults(*results);
  return 0;
}