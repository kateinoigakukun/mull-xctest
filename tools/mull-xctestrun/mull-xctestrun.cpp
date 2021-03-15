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

opt<std::string> TestTarget("test-target", desc("test target name"), Optional,
                            value_desc("name"));

opt<std::string> ResultBundleDir("result-bundle-dir",
                                 desc("test result bundle directory"), Optional,
                                 value_desc("directory path"));

opt<std::string> LogDir("log-dir", desc("log output directory"), Optional,
                        value_desc("directory path"));

opt<unsigned> Timeout("timeout", desc("Timeout per test run (milliseconds)"),
                      Optional, value_desc("number"), init(60 * 1000));

opt<unsigned> Workers("workers", desc("How many threads to use"), Optional,
                      value_desc("number"));

opt<bool> DebugEnabled("debug",
                       desc("Enables Debug Mode: more logs are printed"),
                       Optional, init(false));

int main(int argc, char **argv) {
  bool validOptions =
      llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  mull::Diagnostics diagnostics;
  mull::MutatorsFactory factory(diagnostics);
  mull::Configuration configuration;
  configuration.debugEnabled = DebugEnabled;
  configuration.timeout = Timeout;
  configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
  if (Workers) {
    configuration.parallelization.workers = Workers;
    configuration.parallelization.mutantExecutionWorkers = Workers;
  }
  mull_xctest::XCTestRunConfig runConfig;
  runConfig.xctestrunFile = TestRunFile;
  runConfig.testTarget = TestTarget;

  llvm::SmallString<128> resultBundleDir(ResultBundleDir);
  if (resultBundleDir.empty()) {
    llvm::sys::fs::createUniqueDirectory("mull-xcresult", resultBundleDir);
  }
  runConfig.resultBundleDir = resultBundleDir.str().str();
  runConfig.xcodebuildArgs = XcodeBuildArgs;
  runConfig.logPath = LogDir;

  if (configuration.debugEnabled) {
    diagnostics.enableDebugMode();
  }

  factory.init();

  mull_xctest::XCTestRunInvocation invocation(factory, diagnostics,
                                              configuration, runConfig);
  mull::IDEReporter reporter(diagnostics);
  auto results = invocation.run();
  reporter.reportResults(*results);
  return 0;
}
