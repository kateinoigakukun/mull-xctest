#ifndef MULL_XCTEST_MUTANT_SERIALIZATION_H
#define MULL_XCTEST_MUTANT_SERIALIZATION_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <mull/Mutant.h>
#include <mull/Mutators/MutatorsFactory.h>

namespace mull_xctest {

using MutantList = std::vector<std::unique_ptr<mull::Mutant>>;

llvm::Expected<MutantList> ExtractMutantInfo(
    std::string binaryPath, mull::MutatorsFactory &factory, mull::Diagnostics &);

} // namespace mull_xctest

#endif
