#include "MullXCTest/MutantSerialization.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Support/CommandLine.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/MutationPoint.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Reporters/IDEReporter.h>
#include <mull/Version.h>
#include <sstream>
#include <vector>

using namespace llvm::cl;
using namespace llvm;
using namespace mull;
using namespace mull_xctest;

static std::string dumpMutant(mull::Mutant &mutant) {
  std::stringstream ss;
  ss << "Mutation Point: " << mutant.getMutatorIdentifier() << " "
     << mutant.getSourceLocation().filePath << ":"
     << mutant.getSourceLocation().line << ":"
     << mutant.getSourceLocation().column;
  return ss.str();
}

opt<std::string> InputFile(Positional, desc("<input file>"), Required,
                           value_desc("path"));

int main(int argc, char **argv) {
  bool validOptions =
      llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
  if (!validOptions) {
    return 1;
  }

  mull::Diagnostics diagnostics;
  mull::MutatorsFactory factory(diagnostics);
  factory.init();
  auto result = ExtractMutantInfo(InputFile, factory, diagnostics);
  if (!result) {
    diagnostics.error(llvm::toString(result.takeError()));
  }

  if (result->empty()) {
    llvm::outs() << "no mutant info found\n";
    return 0;
  }

  for (auto &mutant : *result) {
    llvm::outs() << dumpMutant(*mutant) << "\n";
  }
  return 0;
}
