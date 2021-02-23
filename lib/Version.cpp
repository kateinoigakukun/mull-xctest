#include "MullXCTest/Version.h"

#include <llvm/Support/raw_ostream.h>

namespace mull_xctest {

const char *MullXCTestVersionString() { return "@PROJECT_VERSION@"; }
const char *MullXCTestCommitString() { return "@GIT_COMMIT@"; }
const char *MullXCTestBuildDateString() { return "@BUILD_DATE@"; }

const char *llvmVersionString() { return "@LLVM_VERSION@"; }

void printVersionInformation(llvm::raw_ostream &out) {
  out << "Version: " << MullXCTestVersionString() << "\n";
  out << "Commit: " << MullXCTestCommitString() << "\n";
  out << "Date: " << MullXCTestBuildDateString() << "\n";
  out << "LLVM: " << llvmVersionString() << "\n";
}

} // namespace mull
