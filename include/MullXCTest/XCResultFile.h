#ifndef MULL_XCTEST_XCRESULT_FILE_H
#define MULL_XCTEST_XCRESULT_FILE_H

#include <llvm/Support/Error.h>
#include <set>
#include <string>

namespace mull_xctest {

class XCResultFile {
  std::string resultFile;

public:
  XCResultFile(std::string resultFile) : resultFile(resultFile) {}
  llvm::Expected<std::set<std::string>> getFailureTestTargets();
};
} // namespace mull_xctest

#endif
