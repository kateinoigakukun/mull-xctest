#ifndef MULL_XCTEST_XCTEST_RUN_FILE_H
#define MULL_XCTEST_XCTEST_RUN_FILE_H

#include <llvm/Support/Error.h>
#include <string>
#include <vector>

namespace mull_xctest {

class XCTestRunFile {
  std::string filePath;

public:
  XCTestRunFile(std::string filePath) : filePath(filePath) {}
  bool addEnvironmentVariable(std::string targetName, const std::string &key,
                              const std::string &value);
  llvm::Expected<std::vector<std::string>>
  getDependentProductPaths(std::string targetName);
};

} // namespace mull_xctest

#endif
