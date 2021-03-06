#ifndef XCTestRunInvocation_h
#define XCTestRunInvocation_h

#include <llvm/ADT/StringRef.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Mutant.h>
#include <mull/MutationPoint.h>
#include <mull/MutationResult.h>
#include <mull/Mutators/Mutator.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Result.h>

namespace mull_xctest {
class XCTestRunInvocation {
  const llvm::StringRef xctestrunFile;
  const std::string testTarget;
  std::string resultBundleDir;
  const std::vector<std::string> xcodebuildArgs;
  mull::MutatorsFactory &factory;
  mull::Diagnostics &diagnostics;
  const mull::Configuration &config;
  mull::SingleTaskExecutor singleTask;

  std::vector<std::unique_ptr<mull::MutationPoint>> allPoints;

public:
  XCTestRunInvocation(const llvm::StringRef testBundle,
                      const std::string testTarget,
                      std::string resultBundleDir,
                      const std::vector<std::string> xcodebuildArgs,
                      mull::MutatorsFactory &factory,
                      mull::Diagnostics &diagnostics,
                      const mull::Configuration &config)
      : xctestrunFile(testBundle), testTarget(testTarget),
        resultBundleDir(resultBundleDir),
        xcodebuildArgs(xcodebuildArgs), factory(factory),
        diagnostics(diagnostics), config(config), singleTask(diagnostics) {}
  std::unique_ptr<mull::Result> run();

private:
  std::vector<std::unique_ptr<mull::Mutant>> extractMutantInfo();
};
} // namespace mull_xctest

#endif
