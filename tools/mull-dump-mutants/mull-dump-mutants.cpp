#include "MullXCTest/MutantMetadata.h"
#include "MullXCTest/MutantSerialization.h"
#include <llvm/Support/CommandLine.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Reporters/IDEReporter.h>
#include <mull/Version.h>
#include <mull/MutationPoint.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <vector>
#include <sstream>

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
  factory.init();

  auto binaryOrErr = object::createBinary(InputFile);
  if (!binaryOrErr) {
    std::stringstream errorMessage;
    errorMessage << "createBinary failed: \""
    << toString(binaryOrErr.takeError()) << "\".";
    diagnostics.error(errorMessage.str());
    return 1;
  }
  const auto *machOObjectFile =
  dyn_cast<object::MachOObjectFile>(binaryOrErr->getBinary());
  if (!machOObjectFile) {
    diagnostics.error("input file is not mach-o object file");
    return 1;
  }
  auto sectionName = MULL_MUTANTS_INFO_SECTION_NAME_STR;
  Expected<object::SectionRef> section =
  machOObjectFile->getSection(sectionName);
  if (!section) {
    diagnostics.error("section " MULL_MUTANTS_INFO_SECTION_NAME_STR "not found");
    llvm::consumeError(section.takeError());
    return 1;
  }
  
  Expected<StringRef> contentsData = section->getContents();
  if (!contentsData) {
    std::stringstream errorMessage;
    errorMessage << "section->getContents failed: \""
    << toString(contentsData.takeError()) << "\".";
    diagnostics.error(errorMessage.str());
  }
  MutantDeserializer deserializer(contentsData.get(), factory);
  
  std::vector<std::unique_ptr<mull::Mutant>> mutants;
  while (!deserializer.isEOF()) {
    std::unique_ptr<mull::MutationPoint> point = deserializer.deserialize();
    if (!point) {
      diagnostics.error("failed to deserialize mutants.");
      break;
    }
    std::string id = point->getUserIdentifier();
    pointsOwner.push_back(std::move(point));
    std::vector<mull::MutationPoint *> points;
    points.push_back(pointsOwner.back().get());
    mutants.push_back(std::make_unique<Mutant>(id, points));
  }

  for (auto &mutant : mutants) {
    llvm::dbgs() << mutant->getMutationPoints().front()->dump() << "\n";
  }
  return 0;
}
