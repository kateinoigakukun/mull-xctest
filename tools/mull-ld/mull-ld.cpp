#include "LinkerInvocation.h"
#include "LinkerOptions.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/OptTable.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/FilePathFilter.h>
#include <mull/Filters/Filter.h>
#include <mull/Filters/Filters.h>
#include <mull/Filters/JunkMutationFilter.h>
#include <mull/Filters/NoDebugInfoFilter.h>
#include <mull/MutationsFinder.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Toolchain/Runner.h>
#include <string>
#include <unistd.h>
#include <vector>

using namespace llvm::cl;

OptionCategory MullLDCategory("mull-ld");

opt<std::string> Linker("linker", desc("Linker program"), value_desc("string"),
                        Optional, cat(MullLDCategory));

opt<std::string> LinkerFlavor("linker-flavor",
                              desc("Linker flavor (clang|ld64)"),
                              value_desc("string"), Optional, init("ld64"),
                              cat(MullLDCategory));

opt<bool> DebugEnabled("debug",
                       desc("Enables Debug Mode: more logs are printed"),
                       Optional, init(false), cat(MullLDCategory));

opt<std::string>
    DumpLLVM("dump-llvm",
             desc("Dump intermediate LLVM IRs in the specified directory"),
             Optional, cat(MullLDCategory));

opt<bool> EnableSyntaxFilter("enable-syntax",
                             desc("Enables syntax filter for Swift"), Optional,
                             init(true), cat(MullLDCategory));

opt<unsigned> Workers("workers", desc("How many threads to use"), Optional,
                      value_desc("number"), cat(MullLDCategory));

list<std::string>
    ExcludePaths("exclude-path",
                 desc("File/directory paths to ignore (supports regex)"),
                 ZeroOrMore, value_desc("regex"), cat(MullLDCategory));

list<std::string>
    IncludePaths("include-path",
                 desc("File/directory paths to whitelist (supports regex)"),
                 ZeroOrMore, value_desc("regex"), cat(MullLDCategory));


opt<std::string> CoverageInfo(
    "coverage-info",
    desc("Path to the coverage info file (LLVM's profdata)"),
    value_desc("string"),
    Optional,
    init(std::string()), cat(MullLDCategory));

list<std::string>
    TargetExecutables("target-executable",
                      desc("Executable file path to collect coverage info"),
                      ZeroOrMore, value_desc("path"), cat(MullLDCategory));

void filterMullOptions(const llvm::ArrayRef<const char *> &args,
                       std::vector<const char *> &mullArgs,
                       std::vector<const char *> &linkerArgs) {
  size_t index = 0;
  while (index < args.size()) {
    if (strcmp(args[index], "-Xmull") == 0) {
      mullArgs.push_back(args[index + 1]);
      index++;
    } else {
      linkerArgs.push_back(args[index]);
    }
    index++;
  }
}

void extractBitcodeFiles(std::vector<std::string> &args,
                         std::vector<llvm::StringRef> &bitcodeFiles) {
  for (const auto &rawArg : args) {
    llvm::StringRef arg(rawArg);
    if (arg.endswith(".o")) {
      bitcodeFiles.push_back(arg);
    }
  }
}

static void validateInputFiles(const std::vector<std::string> &inputFiles) {
  for (const auto inputFile : inputFiles) {
    if (access(inputFile.c_str(), R_OK) != 0) {
      perror(inputFile.c_str());
      exit(1);
    }
  }
}

static void validateConfiguration(const mull::Configuration &configuration,
                                  mull::Diagnostics &diags) {
  if (configuration.linker.empty()) {
    diags.error(
        "No linker specified. Please set --linker option in MULL_XCTEST_ARGS"
        "environment variable.");
  }
}

llvm::Optional<std::string> getLinkerPath(mull::Diagnostics &diagnositcs) {
  if (!Linker.empty()) {
    return std::string(Linker);
  }
  mull::Runner runner(diagnositcs);
  auto result =
      runner.runProgram("/usr/bin/xcrun", {"-find", "ld"}, {}, 3000, true);
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

void bootstrapFilters(
    mull::Filters &filters, mull::Diagnostics &diagnostics,
    std::vector<std::unique_ptr<mull::Filter>> &filterStorage) {
  auto *noDebugInfoFilter = new mull::NoDebugInfoFilter;
  auto *filePathFilter = new mull::FilePathFilter;
  filterStorage.emplace_back(noDebugInfoFilter);
  filterStorage.emplace_back(filePathFilter);

  filters.mutationFilters.push_back(noDebugInfoFilter);
  filters.functionFilters.push_back(noDebugInfoFilter);
  filters.instructionFilters.push_back(noDebugInfoFilter);

  filters.mutationFilters.push_back(filePathFilter);
  filters.functionFilters.push_back(filePathFilter);

  for (const auto &regex : ExcludePaths) {
    filePathFilter->exclude(regex);
  }
  for (const auto &regex : IncludePaths) {
    filePathFilter->include(regex);
  }
}

void bootstrapConfiguration(mull::Configuration &configuration,
                            mull::Diagnostics &diagnostics) {
  if (const auto linker = getLinkerPath(diagnostics)) {
    configuration.linker = linker.getValue();
  } else {
    diagnostics.error("no real linker found");
  }
  if (Workers) {
    configuration.parallelization.workers = Workers;
  } else {
    configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
  }
  configuration.debugEnabled = DebugEnabled;
  configuration.coverageInfo = CoverageInfo;
  configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
  configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
}

void bootstrapInvocationConfiguration(
    mull_xctest::InvocationConfig &configuration,
    mull::Diagnostics &diagnostics) {
  if (DumpLLVM.empty()) {
    configuration.DumpLLVM = llvm::None;
  } else {
    configuration.DumpLLVM = DumpLLVM;
  }
  configuration.EnableSyntaxFilter = EnableSyntaxFilter;
  if (EnableSyntaxFilter.hasArgStr()) {
    std::string status =
        configuration.EnableSyntaxFilter ? "enabled" : "disabled";
    diagnostics.debug("syntax filter is " + status);
  }
}

int main(int argc, char **argv) {
  mull::Diagnostics diagnostics;
  llvm::ArrayRef<const char *> args(argv + 1, argv + argc);
  std::vector<const char *> mullArgs { *argv };
  std::vector<const char *> linkerArgs { *argv };
  filterMullOptions(args, mullArgs, linkerArgs);


  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;

  bool validOptions = llvm::cl::ParseCommandLineOptions(
      mullArgs.size(), mullArgs.data(), "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  if (DebugEnabled) {
    diagnostics.enableDebugMode();
  }

  mull_xctest::LinkerOptTable *optTable =
      mull_xctest::GetLinkerOptTable(LinkerFlavor);

  unsigned missingIndex;
  unsigned missingCount;
  auto parsedArgs = optTable->ParseArgs(linkerArgs, missingIndex, missingCount);
  mull_xctest::LinkerOptions options(*optTable, parsedArgs);

  std::vector<std::string> inputObjects;
  options.collectObjectFiles(inputObjects);

  validateInputFiles(inputObjects);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  std::vector<std::unique_ptr<mull::Filter>> filterStorage;
  mull::Filters filters;
  mull::Configuration configuration;
  mull_xctest::InvocationConfig invocationConfig;

  bootstrapConfiguration(configuration, diagnostics);

  validateConfiguration(configuration, diagnostics);
  bootstrapInvocationConfiguration(invocationConfig, diagnostics);

  bootstrapFilters(filters, diagnostics, filterStorage);
  mull::MutatorsFactory factory(diagnostics);
  std::vector<std::string> groups = {
      "cxx_comparison", "cxx_arithmetic", "cxx_arithmetic_assignment",
      "cxx_boundary",   "swift_logical",
  };
  mull::MutationsFinder mutationsFinder(factory.mutators(groups),
                                        configuration);

  std::vector<llvm::StringRef> targetExecutables(TargetExecutables.begin(), TargetExecutables.end());
  mull_xctest::LinkerInvocation invocation(
      inputObjects, targetExecutables, filters, mutationsFinder, options, diagnostics,
      configuration, invocationConfig);
  invocation.run();
  llvm::llvm_shutdown();
  return 0;
}
