#ifndef MULL_XCTEST_XCRESULT_FILE_H
#define MULL_XCTEST_XCRESULT_FILE_H

#include <string>
#include <set>
#include <llvm/Support/Error.h>

namespace mull_xctest {

class XCResultFile {
  std::string resultFile;
public:
  XCResultFile(std::string resultFile) : resultFile(resultFile) {}
  llvm::Expected<std::set<std::string>>
  getFailureTestTargets();
};
}

#endif
