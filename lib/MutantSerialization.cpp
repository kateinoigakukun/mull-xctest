#include "MullXCTest/MutantSerialization.h"
#include "MutantExtractor.h"

using namespace mull_xctest;

llvm::Expected<MutantList> mull_xctest::ExtractMutantInfo(
    std::string binaryPath, mull::MutatorsFactory &factory,
    mull::Diagnostics &diags) {
  mull::MutantExtractor extractor(diags);
  return std::move(extractor.extractMutants(binaryPath));
}
