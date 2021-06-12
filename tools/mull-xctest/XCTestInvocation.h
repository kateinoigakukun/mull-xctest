#ifndef XCTestInvocation_h
#define XCTestInvocation_h

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
class XCTestInvocation {
  const llvm::StringRef testBundle;
  mull::MutatorsFactory &factory;
  mull::Diagnostics &diagnostics;
  const mull::Configuration &config;
  mull::SingleTaskExecutor singleTask;
  const std::vector<std::string> XCTestArgs;

public:
  XCTestInvocation(llvm::StringRef testBundle, mull::MutatorsFactory &factory,
                   mull::Diagnostics &diagnostics,
                   const mull::Configuration &config,
                   const std::vector<std::string> XCTestArgs)
      : testBundle(testBundle), factory(factory), diagnostics(diagnostics),
        config(config), singleTask(diagnostics), XCTestArgs(XCTestArgs) {}
  std::unique_ptr<mull::Result> run();

private:
  std::vector<std::unique_ptr<mull::Mutant>> extractMutantInfo();
};
} // namespace mull_xctest

#endif
