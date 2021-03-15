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
struct XCTestRunConfig {
  llvm::StringRef xctestrunFile;
  std::string testTarget;
  std::string resultBundleDir;
  std::vector<std::string> xcodebuildArgs;
  std::string logPath;
};
class XCTestRunInvocation {
  mull::MutatorsFactory &factory;
  mull::Diagnostics &diagnostics;
  const mull::Configuration &config;
  const XCTestRunConfig &runConfig;
  mull::SingleTaskExecutor singleTask;

  std::vector<std::unique_ptr<mull::MutationPoint>> allPoints;

public:
  XCTestRunInvocation(mull::MutatorsFactory &factory,
                      mull::Diagnostics &diagnostics,
                      const mull::Configuration &config,
                      const XCTestRunConfig &runConfig)
      : factory(factory), diagnostics(diagnostics), config(config),
        runConfig(runConfig), singleTask(diagnostics) {}
  std::unique_ptr<mull::Result> run();

private:
  std::vector<std::unique_ptr<mull::Mutant>>
  extractMutantInfo(std::vector<std::pair<std::string, mull::Mutant *>> &);
};
} // namespace mull_xctest

#endif
