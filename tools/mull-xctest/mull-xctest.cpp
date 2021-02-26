#include "XCTestInvocation.h"
#include <llvm/Support/CommandLine.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Reporters/IDEReporter.h>
#include <mull/Version.h>
#include <vector>

using namespace llvm::cl;
using namespace llvm;

opt<std::string> InputFile(Positional, desc("<input file>"), Required,
                           value_desc("path"));

list<std::string> ActiveMutant("active-mutant",
                               desc("Active mutant identifiers"), ZeroOrMore,
                               value_desc("identifier"));

int main(int argc, char **argv) {
  bool validOptions =
      llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  mull::Diagnostics diagnostics;
  mull::Configuration configuration;
  // FIXME: Link input objects with real linker
  configuration.executable = "echo";
  configuration.linker = getenv("MULL_XCTEST_LINKER");
  configuration.debugEnabled = true;
  configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
  configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
  configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
  diagnostics.enableDebugMode();

  mull_xctest::XCTestInvocation invocation(InputFile, diagnostics,
                                           configuration);
  mull::IDEReporter reporter(diagnostics);
  auto results = invocation.run();
  reporter.reportResults(*results);
  return 0;
}
