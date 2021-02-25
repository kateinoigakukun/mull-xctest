#include "llvm/Support/CommandLine.h"
#include "mull/Version.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Tasks/LoadBitcodeFromBinaryTask.h"
#include "mull/Parallelization/TaskExecutor.h"
#include "ebc/EmbeddedFile.h"
#include "ebc/BitcodeRetriever.h"
#include "ebc/BitcodeContainer.h"
#include <vector>
#include <sstream>
#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

using namespace llvm::cl;

opt<std::string> InputFile(
  Positional,
  desc("<input file>"),
  Required,
  value_desc("path"));

list<std::string> ActiveMutant(
  "active-mutant",
  desc("Active mutant identifiers"),
  ZeroOrMore,
  value_desc("identifier"));

int main(int argc, char **argv) {
    bool validOptions = llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
    if (!validOptions) {
      return 1;
    }

    mull::Diagnostics diagnostics;
    reproc::process process;
    auto executable = "/Applications/Xcode-12.4.app/Contents/Developer/usr/bin/xctest";

    std::vector<std::string> allArguments{ executable, InputFile };
    std::vector<std::pair<std::string, std::string>> env;
    env.reserve(ActiveMutant.size());
    for (auto &identifier : ActiveMutant) {
      env.emplace_back(identifier, "1");
    }
    reproc::options options;
    options.env.extra = reproc::env(env);

    std::error_code ec = process.start(allArguments, options);
    int status;
    std::tie(status, ec) = process.wait(reproc::milliseconds(10000));

    return status;
}
