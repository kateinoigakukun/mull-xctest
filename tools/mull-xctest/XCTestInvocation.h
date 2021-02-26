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

  std::vector<std::unique_ptr<mull::Mutator>> mutatorsOwner;
  std::vector<std::unique_ptr<mull::MutationPoint>> allPoints;

public:
  XCTestInvocation(llvm::StringRef testBundle, mull::MutatorsFactory &factory,
                   mull::Diagnostics &diagnostics,
                   const mull::Configuration &config)
      : testBundle(testBundle), factory(factory), diagnostics(diagnostics),
        config(config), singleTask(diagnostics) {}
  std::unique_ptr<mull::Result> run();

private:
  std::vector<std::unique_ptr<mull::Mutant>> extractMutantInfo(
      std::vector<std::unique_ptr<mull::Mutator>> &mutators,
      std::vector<std::unique_ptr<mull::MutationPoint>> &allPoints);
};
} // namespace mull_xctest

#endif
