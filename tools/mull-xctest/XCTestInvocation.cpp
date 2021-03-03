#include "XCTestInvocation.h"
#include "MullXCTest/MutantSerialization.h"
#include <llvm/Support/Path.h>
#include <mull/MutationResult.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Toolchain/Runner.h>

using namespace llvm;
using namespace mull;

namespace mull_xctest {

namespace {

class MutantExecutionTask {
public:
  using In = const std::vector<std::unique_ptr<Mutant>>;
  using Out = std::vector<std::unique_ptr<MutationResult>>;
  using iterator = In::const_iterator;

  MutantExecutionTask(const Configuration &configuration,
                      Diagnostics &diagnostics, const std::string executable,
                      const std::string testBundle, ExecutionResult &baseline)
      : configuration(configuration), diagnostics(diagnostics),
        executable(executable), testBundle(testBundle), baseline(baseline) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  const Configuration &configuration;
  Diagnostics &diagnostics;
  const std::string executable;
  const std::string testBundle;
  ExecutionResult &baseline;
};

void MutantExecutionTask::operator()(iterator begin, iterator end, Out &storage,
                                     progress_counter &counter) {
  Runner runner(diagnostics);
  for (auto it = begin; it != end; ++it, counter.increment()) {
    auto &mutant = *it;
    ExecutionResult result;
    if (mutant->isCovered()) {
      result = runner.runProgram(
          executable, {"xctest", testBundle}, {mutant->getIdentifier()},
          baseline.runningTime * 10, configuration.captureMutantOutput);
    } else {
      result.status = NotCovered;
    }
    storage.push_back(std::make_unique<MutationResult>(result, mutant.get()));
  }
}
}

std::unique_ptr<Result> XCTestInvocation::run() {
  auto xcrun = "/usr/bin/xcrun";
  auto xctest = "xctest";
  auto mutants = extractMutantInfo(mutatorsOwner, allPoints);
  Runner runner(diagnostics);

  singleTask.execute("Warm up run", [&]() {
    runner.runProgram(xcrun, {xctest, testBundle.str()}, {}, config.timeout,
                      config.captureMutantOutput);
  });

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    baseline = runner.runProgram(xcrun, {xctest, testBundle.str()}, {},
                                 config.timeout, config.captureMutantOutput);
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(config, diagnostics, xcrun, testBundle.str(), baseline);
  }
  TaskExecutor<MutantExecutionTask> mutantRunner(diagnostics, "Running mutants",
                                                 mutants, mutationResults,
                                                 std::move(tasks));
  mutantRunner.execute();

  std::vector<MutationPoint *> filteredMutations{};

  return std::make_unique<Result>(
      std::move(mutants), std::move(mutationResults), filteredMutations);
}

MutantList XCTestInvocation::extractMutantInfo(
    std::vector<std::unique_ptr<mull::Mutator>> &mutators,
    std::vector<std::unique_ptr<mull::MutationPoint>> &pointsOwner) {

  auto filename = llvm::sys::path::filename(testBundle);
  auto basename = filename.rsplit(".").first;
  SmallString<64> binaryPath(testBundle);
  llvm::sys::path::append(binaryPath, "Contents", "MacOS", basename);

  auto result = ExtractMutantInfo(binaryPath.str().str(), factory, mutators, pointsOwner);
  if (!result) {
    llvm::consumeError(result.takeError());
    return {};
  }
  return std::move(result.get());
}

}; // namespace mull_xctest
