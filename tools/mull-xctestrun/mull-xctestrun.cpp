#include "XCTestRunInvocation.h"
#include <llvm/Support/CommandLine.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Reporters/IDEReporter.h>
#include <mull/Version.h>
#include <vector>

using namespace llvm::cl;

opt<std::string> TestRunFile(Positional, desc("<xctestrun file>"), Required,
                             value_desc("path"));

list<std::string> XcodeBuildArgs("Xxcodebuild",
                                 desc("Pass <arg> to the xcodebuild"),
                                 ZeroOrMore, value_desc("arg"));

opt<std::string> TestTarget("test-target", desc("test target name"), Required,
                            value_desc("name"));

opt<unsigned> Timeout("timeout", desc("Timeout per test run (milliseconds)"),
                      Optional, value_desc("number"),
                      init(mull::MullDefaultTimeoutMilliseconds));

int main(int argc, char **argv) {
  bool validOptions =
      llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  mull::Diagnostics diagnostics;
  mull::MutatorsFactory factory(diagnostics);
  mull::Configuration configuration;
  configuration.debugEnabled = true;
  configuration.timeout = Timeout;
  configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
  diagnostics.enableDebugMode();

  factory.init();
  mull_xctest::XCTestRunInvocation invocation(TestRunFile, TestTarget,
                                              XcodeBuildArgs, factory,
                                              diagnostics, configuration);
  mull::IDEReporter reporter(diagnostics);
  auto results = invocation.run();
  reporter.reportResults(*results);
  return 0;
}
