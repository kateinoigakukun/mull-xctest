#include "MullXCTest/MutantMetadata.h"
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
  std::vector<std::unique_ptr<mull::MutationPoint>> pointsOwner;
  std::vector<std::unique_ptr<mull::Mutator>> mutators;
  factory.init();
  auto result = ExtractMutantInfo(InputFile, factory, mutators, pointsOwner);
  if (!result) {
    llvm::report_fatal_error(result.takeError());
  }

  if (result->empty()) {
    llvm::outs() << "no mutant info found\n";
    return 0;
  }

  for (auto &mutant : *result) {
    llvm::outs() << mutant->getMutationPoints().front()->dump() << "\n";
  }
  return 0;
}
