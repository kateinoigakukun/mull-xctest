#include "XCTestInvocation.h"
#include "MullXCTest/MutantSerialization.h"
#include <llvm/Support/Path.h>
#include <mull/MutationResult.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Toolchain/Runner.h>
#include <sstream>

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
                      const std::string testBundle, ExecutionResult &baseline,
                      const std::vector<std::string> XCTestArgs)
      : configuration(configuration), diagnostics(diagnostics),
        executable(executable), testBundle(testBundle), baseline(baseline),
        XCTestArgs(XCTestArgs) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  const Configuration &configuration;
  Diagnostics &diagnostics;
  const std::string executable;
  const std::string testBundle;
  const std::vector<std::string> XCTestArgs;
  ExecutionResult &baseline;
};

void MutantExecutionTask::operator()(iterator begin, iterator end, Out &storage,
                                     progress_counter &counter) {
  Runner runner(diagnostics);
  std::vector<std::string> arguments;
  arguments.push_back("xctest");
  arguments.insert(arguments.end(), XCTestArgs.begin(), XCTestArgs.end());
  arguments.push_back(testBundle);

  for (auto it = begin; it != end; ++it, counter.increment()) {
    auto &mutant = *it;
    ExecutionResult result;
    if (mutant->isCovered()) {
      result =
          runner.runProgram(executable, arguments, {mutant->getIdentifier()},
                            baseline.runningTime * 10,
                            configuration.captureMutantOutput, std::nullopt);
    } else {
      result.status = NotCovered;
    }
    if (result.status == Timedout) {
      std::stringstream message;
      message << "Timedout runningTime=" << result.runningTime << "\n";
      message << "         exitStatus=" << result.exitStatus << "\n";
      message << "         stderrOutput=" << result.stderrOutput << "\n";
      message << "         stdoutOutput=" << result.stdoutOutput << "\n";
    }
    storage.push_back(std::make_unique<MutationResult>(result, mutant.get()));
  }
}
} // namespace

std::unique_ptr<Result> XCTestInvocation::run() {
  auto xcrun = "/usr/bin/xcrun";
  auto xctest = "xctest";
  auto mutants = extractMutantInfo();
  Runner runner(diagnostics);

  singleTask.execute("Warm up run", [&]() {
    runner.runProgram(xcrun, {xctest, testBundle.str()}, {}, config.timeout,
                      config.captureMutantOutput, std::nullopt);
  });

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    baseline =
        runner.runProgram(xcrun, {xctest, testBundle.str()}, {}, config.timeout,
                          config.captureMutantOutput, std::nullopt);
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(config, diagnostics, xcrun, testBundle.str(), baseline,
                       XCTestArgs);
  }
  TaskExecutor<MutantExecutionTask> mutantRunner(diagnostics, "Running mutants",
                                                 mutants, mutationResults,
                                                 std::move(tasks));
  mutantRunner.execute();

  return std::make_unique<Result>(std::move(mutants),
                                  std::move(mutationResults));
}

MutantList XCTestInvocation::extractMutantInfo() {

  auto filename = llvm::sys::path::filename(testBundle);
  auto basename = filename.rsplit(".").first;
  SmallString<64> binaryPath(testBundle);
  llvm::sys::path::append(binaryPath, "Contents", "MacOS", basename);

  auto result = ExtractMutantInfo(binaryPath.str().str(), factory, diagnostics);
  if (!result) {
    llvm::consumeError(result.takeError());
    return {};
  }
  return std::move(result.get());
}

}; // namespace mull_xctest
