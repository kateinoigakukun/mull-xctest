#include "XCTestRunInvocation.h"
#include "MullXCTest/MutantMetadata.h"
#include "MullXCTest/MutantSerialization.h"
#include "XCTestRunFile.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <mull/MutationResult.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Toolchain/Runner.h>
#include <set>
#include <sstream>

using namespace llvm;
using namespace mull;

namespace mull_xctest {

namespace {

std::string GetBundleBinaryPath(std::string &bundlePath) {
  llvm::SmallString<128> binaryPath(bundlePath);
  auto filename = llvm::sys::path::filename(bundlePath);
  auto basename = filename.rsplit(".").first;
  llvm::sys::path::append(binaryPath, basename);
  return binaryPath.str().str();
}

ExecutionResult RunXcodeBuildTest(Runner &runner,
                                  const std::string &xctestrunFile,
                                  const std::vector<std::string> &extraArgs,
                                  const std::vector<std::string> &environment,
                                  long long int timeout, bool captureOutput) {
  std::vector<std::string> arguments;
  arguments.push_back("test-without-building");
  std::copy(extraArgs.begin(), extraArgs.end(), std::back_inserter(arguments));
  arguments.push_back("-xctestrun");
  arguments.push_back(xctestrunFile);
  return runner.runProgram("/usr/bin/xcodebuild", arguments, environment,
                           timeout, captureOutput);
}

class MutantExecutionTask {
public:
  using In = const std::vector<std::unique_ptr<Mutant>>;
  using Out = std::vector<std::unique_ptr<MutationResult>>;
  using iterator = In::const_iterator;

  MutantExecutionTask(const int taskID, const Configuration &configuration,
                      Diagnostics &diagnostics, const std::string xctestrunFile,
                      const std::string targetName,
                      const std::vector<std::string> xcodebuildArgs,
                      ExecutionResult &baseline)
      : taskID(taskID), configuration(configuration), diagnostics(diagnostics),
        xctestrunFile(xctestrunFile), targetName(targetName),
        xcodebuildArgs(xcodebuildArgs), baseline(baseline) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  const int taskID;
  const Configuration &configuration;
  Diagnostics &diagnostics;
  const std::string xctestrunFile;
  const std::string targetName;
  const std::vector<std::string> xcodebuildArgs;
  ExecutionResult &baseline;
};

void MutantExecutionTask::operator()(iterator begin, iterator end, Out &storage,
                                     progress_counter &counter) {
  Runner runner(diagnostics);
  const std::string localRunFile =
      xctestrunFile + ".mull-xctrn-" + std::to_string(taskID);
  for (auto it = begin; it != end; ++it, counter.increment()) {
    auto &mutant = *it;
    ExecutionResult result;
    if (mutant->isCovered()) {
      llvm::sys::fs::copy_file(xctestrunFile, localRunFile);
      XCTestRunFile runFile(localRunFile);
      runFile.addEnvironmentVariable(targetName, mutant->getIdentifier(), "1");
      result = RunXcodeBuildTest(runner, localRunFile, xcodebuildArgs, {},
                                 baseline.runningTime * 10,
                                 configuration.captureMutantOutput);
    } else {
      result.status = NotCovered;
    }
    storage.push_back(std::make_unique<MutationResult>(result, mutant.get()));
  }
}

} // namespace

std::unique_ptr<Result> XCTestRunInvocation::run() {
  auto mutants = extractMutantInfo(mutatorsOwner, allPoints);
  Runner runner(diagnostics);

  singleTask.execute("Warm up run", [&]() {
    RunXcodeBuildTest(runner, xctestrunFile.str(), xcodebuildArgs, {},
                      config.timeout, config.captureMutantOutput);
  });

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    baseline =
        RunXcodeBuildTest(runner, xctestrunFile.str(), xcodebuildArgs, {},
                          config.timeout, config.captureMutantOutput);
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(i, config, diagnostics, xctestrunFile.str(), testTarget,
                       xcodebuildArgs, baseline);
  }
  TaskExecutor<MutantExecutionTask> mutantRunner(diagnostics, "Running mutants",
                                                 mutants, mutationResults,
                                                 std::move(tasks));
  mutantRunner.execute();

  std::vector<MutationPoint *> filteredMutations{};

  return std::make_unique<Result>(
      std::move(mutants), std::move(mutationResults), filteredMutations);
}

std::vector<std::unique_ptr<mull::Mutant>>
XCTestRunInvocation::extractMutantInfo(
    std::vector<std::unique_ptr<mull::Mutator>> &mutators,
    std::vector<std::unique_ptr<mull::MutationPoint>> &pointsOwner) {

  XCTestRunFile file(xctestrunFile.str());
  auto products = file.getDependentProductPaths(testTarget);
  if (!products) {
    diagnostics.error(llvm::toString(products.takeError()));
    return {};
  }

  std::vector<std::unique_ptr<mull::Mutant>> output;
  for (auto product : *products) {
    auto binaryPath = GetBundleBinaryPath(product);
    auto result = ExtractMutantInfo(binaryPath, factory, mutators, pointsOwner);
    if (!result) {
      continue;
    }
    std::move(result->begin(), result->end(), std::back_inserter(output));
  }

  return std::move(output);
}

}; // namespace mull_xctest
