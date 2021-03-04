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
                                  const llvm::Optional<std::string> &resultBundlePath,
                                  const std::vector<std::string> &extraArgs,
                                  const std::vector<std::string> &environment,
                                  long long int timeout, bool captureOutput) {
  std::vector<std::string> arguments;
  arguments.push_back("test-without-building");
  std::copy(extraArgs.begin(), extraArgs.end(), std::back_inserter(arguments));
  arguments.push_back("-xctestrun");
  arguments.push_back(xctestrunFile);
  if (resultBundlePath) {
    arguments.push_back("-resultBundlePath");
    arguments.push_back(resultBundlePath.getValue());
  }
  return runner.runProgram("/usr/bin/xcodebuild", arguments, environment,
                           timeout, captureOutput);
}

using Mutants = const std::vector<std::unique_ptr<Mutant>>;

std::string GenerateXCRunFile(const std::string &srcRunFile,
                              const std::string &localRunFile,
                              const std::string &srcTestTarget,
                              const Mutants::const_iterator &mutants_begin,
                              const Mutants::const_iterator &mutants_end) {
  llvm::sys::fs::copy_file(srcRunFile, localRunFile);
  XCTestRunFile runFile(localRunFile);
  for (auto it = mutants_begin; it != mutants_end; ++it) {
    auto &mutant = *it;
    std::string newTarget = srcTestTarget + "-" + mutant->getIdentifier();
    runFile.duplicateTestTarget(srcTestTarget, newTarget);
    runFile.addEnvironmentVariable(newTarget, mutant->getIdentifier(), "1");
  }
  runFile.deleteTestTarget(srcTestTarget);
  return localRunFile;
}

class MutantExecutionTask {
public:
  using In = const std::vector<std::unique_ptr<Mutant>>;
  using Out = std::vector<std::unique_ptr<MutationResult>>;
  using iterator = In::const_iterator;

  MutantExecutionTask(const int taskID, const Configuration &configuration,
                      Diagnostics &diagnostics, const std::string xctestrunFile,
                      const std::string resultBundleDir,
                      const std::string targetName,
                      const std::vector<std::string> xcodebuildArgs,
                      ExecutionResult &baseline)
      : taskID(taskID), configuration(configuration), diagnostics(diagnostics),
        xctestrunFile(xctestrunFile), resultBundleDir(resultBundleDir),
        targetName(targetName),
        xcodebuildArgs(xcodebuildArgs), baseline(baseline) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  const int taskID;
  const Configuration &configuration;
  Diagnostics &diagnostics;
  const std::string xctestrunFile;
  const std::string resultBundleDir;
  const std::string targetName;
  const std::vector<std::string> xcodebuildArgs;
  ExecutionResult &baseline;
};

static std::string ResultBundlePath(std::string resultBundleDir, std::string targetName, int taskID) {
  llvm::SmallString<128> resultBundlePath(resultBundleDir);
  llvm::sys::path::append(resultBundlePath, targetName + "-" + std::to_string(taskID) + ".xcresult");
  return resultBundlePath.str().str();
}

void MutantExecutionTask::operator()(iterator begin, iterator end, Out &storage,
                                     progress_counter &counter) {
  Runner runner(diagnostics);
  const std::string localRunFile =
      xctestrunFile + ".mull-xctrn-" + std::to_string(taskID) + ".xctestrun";
  
  GenerateXCRunFile(xctestrunFile, localRunFile, targetName, begin, end);
  std::string resultBundlePath = ResultBundlePath(resultBundleDir, targetName, taskID);
  ExecutionResult result;
//  if (mutant->isCovered()) {
  result = RunXcodeBuildTest(runner, localRunFile, resultBundlePath,
                             xcodebuildArgs, {}, -1,
                             configuration.captureMutantOutput);
  for (auto it = begin; it != end; ++it, counter.increment()) {
  }
//  } else {
//    result.status = NotCovered;
//  }
  // TODO: Retrieve results from .xcresult
  // storage.push_back(std::make_unique<MutationResult>(result, mutant.get()));
}

} // namespace

std::unique_ptr<Result> XCTestRunInvocation::run() {
  auto mutants = extractMutantInfo(mutatorsOwner, allPoints);
  Runner runner(diagnostics);

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    baseline =
        RunXcodeBuildTest(runner, xctestrunFile.str(), llvm::None, xcodebuildArgs, {},
                          config.timeout, config.captureMutantOutput);
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(i, config, diagnostics, xctestrunFile.str(), resultBundleDir, testTarget,
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
