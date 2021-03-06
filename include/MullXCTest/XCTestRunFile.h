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
  bool setBlueprintName(std::string targetName, const std::string &name);
  llvm::Expected<std::vector<std::string>>
  getDependentProductPaths(std::string targetName);
  llvm::Expected<std::vector<std::string>> getTargets();
  bool duplicateTestTarget(std::string srcTargetName, std::string newTargetName);
  bool deleteTestTarget(std::string targetName);
};

} // namespace mull_xctest

#endif
