#include "MullXCTest/MutationPipeline.h"
#include <llvm/ADT/Optional.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/OptTable.h>
#include <llvm/Option/Option.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/FilePathFilter.h>
#include <mull/Filters/Filter.h>
#include <mull/Filters/Filters.h>
#include <mull/Filters/JunkMutationFilter.h>
#include <mull/Filters/NoDebugInfoFilter.h>
#include <mull/Filters/GitDiffFilter.h>
#include <mull/MutationsFinder.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Toolchain/Runner.h>
#include <sstream>
#include <unistd.h>

namespace mull_xctest {
namespace libtool {

using namespace llvm;

enum {
  OPT_INVALID = 0,
#define OPTION(_1, _2, ID, _4, _5, _6, _7, _8, _9, _10, _11, _12) OPT_##ID,
#include "Options.inc"
#undef OPTION
};

#define PREFIX(NAME, VALUE) const char *NAME[] = VALUE;
#include "Options.inc"
#undef PREFIX

static const ::llvm::opt::OptTable::Info optInfo[] = {
#define OPTION(X1, X2, ID, KIND, GROUP, ALIAS, X7, X8, X9, X10, X11, X12)      \
  {X1,                                                                         \
   X2,                                                                         \
   X10,                                                                        \
   X11,                                                                        \
   mull_xctest::libtool::OPT_##ID,                                             \
   opt::Option::KIND##Class,                                                   \
   X9,                                                                         \
   X8,                                                                         \
   mull_xctest::libtool::OPT_##GROUP,                                          \
   mull_xctest::libtool::OPT_##ALIAS,                                          \
   X7,                                                                         \
   X12},
#include "Options.inc"
#undef OPTION
};

} // namespace libtool

class LibtoolOptTable : public llvm::opt::OptTable {
public:
  LibtoolOptTable() : OptTable(libtool::optInfo) {}
};
} // namespace mull_xctest

using namespace llvm::cl;

OptionCategory MullLibtoolCategory("mull-libtool Options");

opt<std::string> Libtool("libtool", desc("libtool program"),
                         value_desc("string"), Optional,
                         cat(MullLibtoolCategory));

opt<bool> DebugEnabled("debug",
                       desc("Enables Debug Mode: more logs are printed"),
                       Optional, init(false), cat(MullLibtoolCategory));

opt<std::string>
    DumpLLVM("dump-llvm",
             desc("Dump intermediate LLVM IRs in the specified directory"),
             Optional, cat(MullLibtoolCategory));

opt<bool> EnableSyntaxFilter("enable-syntax",
                             desc("Enables syntax filter for Swift"), Optional,
                             init(true), cat(MullLibtoolCategory));

opt<unsigned> Workers("workers", desc("How many threads to use"), Optional,
                      value_desc("number"), cat(MullLibtoolCategory));

list<std::string>
    ExcludePaths("exclude-path",
                 desc("File/directory paths to ignore (supports regex)"),
                 ZeroOrMore, value_desc("regex"), cat(MullLibtoolCategory));

list<std::string>
    IncludePaths("include-path",
                 desc("File/directory paths to whitelist (supports regex)"),
                 ZeroOrMore, value_desc("regex"), cat(MullLibtoolCategory));

opt<std::string>
    CoverageInfo("coverage-info",
                 desc("Path to the coverage info file (LLVM's profdata)"),
                 value_desc("string"), Optional, init(std::string()),
                 cat(MullLibtoolCategory));

list<std::string>
    TargetExecutables("target-executable",
                      desc("Executable file path to collect coverage info"),
                      ZeroOrMore, value_desc("path"), cat(MullLibtoolCategory));


opt<std::string> GitDiffRef("git-diff-ref",
                            desc("Git branch to run diff against (enables incremental testing)"),
                            Optional,
                            value_desc("git commit"),
                            cat(MullLibtoolCategory));

opt<std::string> GitProjectRoot("git-project-root",
                                desc("Path to project's Git root (used together with -git-diff-ref)"),
                                Optional,
                                value_desc("git project root"),
                                cat(MullLibtoolCategory));

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

static void validateInputFiles(const std::vector<std::string> &inputFiles) {
  for (const auto inputFile : inputFiles) {
    if (access(inputFile.c_str(), R_OK) != 0) {
      perror(inputFile.c_str());
      exit(1);
    }
  }
}

llvm::Optional<std::string> getLibtoolPath(mull::Diagnostics &diagnositcs) {
  if (!Libtool.empty()) {
    return std::string(Libtool);
  }
  mull::Runner runner(diagnositcs);
  auto result =
      runner.runProgram("/usr/bin/xcrun", {"-find", "libtool"}, {}, -1, true, std::nullopt);
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

  if (!GitDiffRef.getValue().empty()) {
    if (GitProjectRoot.getValue().empty()) {
      std::stringstream debugMessage;
      debugMessage
          << "-git-diff-ref option has been provided but the path to the Git project root has not "
             "been specified via -git-project-root. The incremental testing will be disabled.";
      diagnostics.warning(debugMessage.str());
    } else if (!llvm::sys::fs::is_directory(GitProjectRoot.getValue())) {
      std::stringstream debugMessage;
      debugMessage << "directory provided by -git-project-root does not exist, ";
      debugMessage << "the incremental testing will be disabled: ";
      debugMessage << GitProjectRoot.getValue();
      diagnostics.warning(debugMessage.str());
    } else {
      std::string gitProjectRoot = GitProjectRoot.getValue();
      std::string gitDiffBranch = GitDiffRef.getValue();
      diagnostics.info(std::string("Incremental testing using Git Diff is enabled.\n")
                       + "- Git ref: " + gitDiffBranch + "\n"
                       + "- Git project root: " + gitProjectRoot);
      mull::GitDiffFilter *gitDiffFilter =
          mull::GitDiffFilter::createFromGitDiff(diagnostics, gitProjectRoot, gitDiffBranch);

      if (gitDiffFilter) {
        filterStorage.emplace_back(gitDiffFilter);
        filters.instructionFilters.push_back(gitDiffFilter);
      }
    }
  }
}

void bootstrapConfiguration(mull::Configuration &configuration,
                            mull::Diagnostics &diagnostics) {
  if (Workers) {
    configuration.parallelization.workers = Workers;
  } else {
    configuration.parallelization =
        mull::ParallelizationConfig::defaultConfig();
  }
  configuration.debugEnabled = DebugEnabled;
  configuration.coverageInfo = CoverageInfo;
  configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
  configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
}

void bootstrapPipelineConfiguration(
    mull_xctest::MutationPipelineConfig &configuration,
    mull::Diagnostics &diagnostics) {
  if (DumpLLVM.empty()) {
    configuration.DumpLLVM = llvm::None;
  } else {
    configuration.DumpLLVM = DumpLLVM;
  }
  configuration.EnableSyntaxFilter = EnableSyntaxFilter;
  if (EnableSyntaxFilter) {
    std::string status =
        configuration.EnableSyntaxFilter ? "enabled" : "disabled";
    diagnostics.debug("syntax filter is " + status);
  }
}

void PrintHelpMessage(char *commandName) {
  llvm::outs() << "USAGE: " << commandName
               << " [linker options] -Xmull [option] -Xmull [option] ...\n\n";
  llvm::outs() << "OPTIONS:\n\n";

  llvm::StringMap<Option *> opts;
  size_t maxArgLen = 0;

  for (auto &option : TopLevelSubCommand->OptionsMap) {
    if (option.getValue()->getOptionHiddenFlag() == ReallyHidden ||
        option.getValue()->getOptionHiddenFlag() == Hidden)
      continue;
    opts.insert(std::make_pair(option.first(), option.second));
    maxArgLen = std::max(maxArgLen, option.second->getOptionWidth());
  }

  for (auto &option : opts) {
    option.getValue()->printOptionInfo(maxArgLen);
  }
}

/// \returns true if an error occurred.
static bool readFilelistFile(llvm::StringRef filepath,
                             std::vector<std::string> &output) {
  auto filelistOrErr = llvm::MemoryBuffer::getFile(filepath);
  if (!filelistOrErr) {
    return true;
  }
  llvm::SmallVector<llvm::StringRef, 4> lines;
  filelistOrErr.get()->getBuffer().split(lines, '\n', /*MaxSplit*/ -1,
                                         /*KeepEmpty*/ false);
  for (auto line : lines) {
    output.push_back(line.rtrim().str());
  }
  return false;
}

void collectObjectFiles(const llvm::opt::InputArgList &args,
                        std::vector<std::string> &objectFiles) {
  using namespace mull_xctest;
  for (auto input : args.filtered(libtool::OPT_INPUT)) {
    objectFiles.push_back(input->getValue());
  }
  for (auto arg : args.filtered(libtool::OPT_filelist)) {
    readFilelistFile(arg->getValue(), objectFiles);
  }
}

void collectObjectLinkOpts(const llvm::opt::InputArgList &args,
                           llvm::opt::ArgStringList &output) {
  using namespace mull_xctest;
  for (auto it = args.begin(); it != args.end(); ++it) {
    auto arg = *it;
    if (!arg->getOption().matches(libtool::OPT_INPUT) &&
        !arg->getOption().matches(libtool::OPT_filelist)) {
      arg->render(args, output);
    }
  }
}

void runLibtool(std::vector<std::string> objectFiles,
                llvm::opt::InputArgList &originalArgs,
                const std::string libtool,
                mull::Diagnostics &diagnostics) {
  mull::Runner runner(diagnostics);
  llvm::opt::ArgStringList rawArgs;

  collectObjectLinkOpts(originalArgs, rawArgs);

  llvm::SmallString<64> filelistPath;
  llvm::sys::fs::createTemporaryFile("mull-libtool-filelist", "", filelistPath);
  llvm::FileRemover cleaner(filelistPath);
  std::error_code ec;
  llvm::raw_fd_ostream filelist(filelistPath, ec);

  for (auto objectFile : objectFiles) {
    filelist << objectFile << "\n";
  }
  filelist.flush();

  std::vector<std::string> arguments;
  for (auto arg : rawArgs) {
    arguments.emplace_back(arg);
  }

  std::string filelistPathStr(filelistPath);
  arguments.push_back("-filelist");
  arguments.push_back(filelistPathStr);

  mull::ExecutionResult result = runner.runProgram(libtool, arguments, {},
                                                   -1, true, std::nullopt);
  std::stringstream commandStream;
  commandStream << libtool;
  for (std::string &argument : arguments) {
    commandStream << ' ' << argument;
  }
  std::string command = commandStream.str();
  if (result.status != mull::Passed) {
    std::stringstream message;
    message << "Cannot link program\n";
    message << "status: " << result.getStatusAsString() << "\n";
    message << "time: " << result.runningTime << "ms\n";
    message << "exit: " << result.exitStatus << "\n";
    message << "command: " << command << "\n";
    message << "stdout: " << result.stdoutOutput << "\n";
    message << "stderr: " << result.stderrOutput << "\n";
    diagnostics.error(message.str());
  }
  diagnostics.debug("Link command: " + command);
}

int main(int argc, char **argv) {
  llvm::cl::HideUnrelatedOptions(MullLibtoolCategory);

  llvm::ArrayRef<const char *> args(argv + 1, argv + argc);
  std::vector<const char *> mullArgs{*argv};
  std::vector<const char *> libtoolArgs;
  filterMullOptions(args, mullArgs, libtoolArgs);

  bool validOptions = llvm::cl::ParseCommandLineOptions(
      mullArgs.size(), mullArgs.data(), "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  mull_xctest::LibtoolOptTable optTable;

  unsigned missingIndex;
  unsigned missingCount;
  auto parsedArgs = optTable.ParseArgs(libtoolArgs, missingIndex, missingCount);

  mull::Diagnostics diagnostics;
  const auto libtool = getLibtoolPath(diagnostics);
  if (!libtool) {
    diagnostics.error("no real libtool found");
  }

  std::vector<std::string> inputObjects;
  collectObjectFiles(parsedArgs, inputObjects);

  validateInputFiles(inputObjects);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  std::vector<std::unique_ptr<mull::Filter>> filterStorage;
  mull::Filters filters;
  mull::Configuration configuration;
  mull_xctest::MutationPipelineConfig pipelineConfig;

  bootstrapConfiguration(configuration, diagnostics);

  bootstrapPipelineConfiguration(pipelineConfig, diagnostics);

  bootstrapFilters(filters, diagnostics, filterStorage);
  mull::MutatorsFactory factory(diagnostics);
  std::vector<std::string> groups = {
      "swift_comparison", "cxx_arithmetic", "cxx_arithmetic_assignment",
      "cxx_boundary",     "swift_logical",
  };
  mull::MutationsFinder mutationsFinder(factory.mutators(groups),
                                        configuration);

  std::vector<llvm::StringRef> targetExecutables(TargetExecutables.begin(),
                                                 TargetExecutables.end());
  mull_xctest::MutationPipeline pipeline(inputObjects, targetExecutables,
                                         filters, mutationsFinder, diagnostics,
                                         configuration, pipelineConfig);

  std::vector<std::string> objectFiles = pipeline.run();

  runLibtool(objectFiles, parsedArgs, libtool.getValue(), diagnostics);
  llvm::llvm_shutdown();
}
