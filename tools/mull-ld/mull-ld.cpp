#include "LinkerInvocation.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/Filters.h>
#include <mull/MutationsFinder.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Toolchain/Runner.h>
#include <string>
#include <unistd.h>
#include <vector>

void extractBitcodeFiles(std::vector<std::string> &args,
                         std::vector<llvm::StringRef> &bitcodeFiles) {
  for (const auto &rawArg : args) {
    llvm::StringRef arg(rawArg);
    if (arg.endswith(".o")) {
      bitcodeFiles.push_back(arg);
    }
  }
}

static void validateInputFiles(const std::vector<llvm::StringRef> &inputFiles) {
  for (const auto inputFile : inputFiles) {
    if (access(inputFile.str().c_str(), R_OK) != 0) {
      perror(inputFile.str().c_str());
      exit(1);
    }
  }
}

static void validateConfiguration(const mull::Configuration &configuration,
                                  mull::Diagnostics &diags) {
  if (configuration.linker.empty()) {
    diags.error("No linker specified. Please set MULL_XCTEST_LINKER "
                "environment variable.");
  }
}

llvm::Optional<std::string> getLinkerPath(mull::Diagnostics &diagnositcs) {
  if (const auto env = getenv("MULL_XCTEST_LINKER")) {
    return std::string(env);
  }
  mull::Runner runner(diagnositcs);
  auto result =
      runner.runProgram("/usr/bin/xcrun", {"-find", "ld"}, {}, -1, true);
  if (result.status != mull::Passed) {
    diagnositcs.error("failed to run xcrun");
    return llvm::None;
  }
  std::string resultOutput = result.stdoutOutput;
  if (resultOutput.back() == '\n') {
    resultOutput.pop_back();
  }
  return resultOutput;
}

int main(int argc, char **argv) {
  mull::Diagnostics diagnostics;
  std::vector<llvm::StringRef> inputObjects;
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;

  std::vector<std::string> args(argv + 1, argv + argc);
  extractBitcodeFiles(args, inputObjects);
  validateInputFiles(inputObjects);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  mull::Filters filters;
  mull::Configuration configuration;
  // FIXME: Link input objects with real linker
  configuration.executable = "echo";
  if (const auto linker = getLinkerPath(diagnostics)) {
    configuration.linker = linker.getValue();
  } else {
    diagnostics.error("no real linker found");
  }
  configuration.debugEnabled = true;
  configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
  configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
  diagnostics.enableDebugMode();

  validateConfiguration(configuration, diagnostics);
  mull::MutatorsFactory factory(diagnostics);
  mull::MutationsFinder mutationsFinder(factory.mutators({"cxx_comparison"}),
                                        configuration);

  mull_xctest::LinkerInvocation invocation(
      inputObjects, filters, mutationsFinder, args, diagnostics, configuration);
  invocation.run();
  llvm::llvm_shutdown();
  return 0;
}
