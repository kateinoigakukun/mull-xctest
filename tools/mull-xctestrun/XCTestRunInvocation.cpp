#include "XCTestRunInvocation.h"
#include "MullXCTest/MutantMetadata.h"
#include "MullXCTest/MutantSerialization.h"
#include "MullXCTest/XCResultFile.h"
#include "MullXCTest/XCTestRunFile.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <mull/MutationResult.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Toolchain/Runner.h>
#include <reproc++/reproc.hpp>
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

ExecutionResult
RunXcodeBuildTest(Diagnostics &diagnostics, const std::string &xctestrunFile,
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
  if (timeout > 0) {
    arguments.push_back("-test-timeouts-enabled");
    arguments.push_back("YES");
    arguments.push_back("-maximum-test-execution-time-allowance");
    arguments.push_back(std::to_string(timeout / 1000));
  }

  std::vector<std::pair<std::string, std::string>> env;
  env.reserve(environment.size());
  for (auto &e : environment) {
    env.emplace_back(e, "1");
  }

  reproc::options options;
  options.env.extra = reproc::env(env);
  options.redirect.path = "";

  std::vector<std::string> allArguments{"/usr/bin/xcodebuild"};
  std::copy(std::begin(arguments), std::end(arguments),
            std::back_inserter(allArguments));
  auto start = std::chrono::high_resolution_clock::now();

  reproc::process process;
  std::error_code ec = process.start(allArguments, options);
  if (ec) {
    std::stringstream errorMessage;
    errorMessage << "Cannot run executable: " << ec.message() << '\n';
    diagnostics.error(errorMessage.str());
  }

  int status;
  std::tie(status, ec) = process.wait(reproc::milliseconds(timeout));
  ExecutionStatus executionStatus = Failed;
  if (ec == std::errc::timed_out) {
    process.kill();
    executionStatus = Timedout;
  } else if (status == 0) {
    executionStatus = Passed;
  } else {
    executionStatus = Failed;
  }

  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  ExecutionResult result;
  result.runningTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  result.exitStatus = status;
  result.status = executionStatus;

  return result;
}

void GenerateSingleTargetXCRunFile(const std::string &srcRunFile,
                                   const std::string &localRunFile,
                                   const std::string &srcTestTarget) {
  llvm::sys::fs::copy_file(srcRunFile, localRunFile);
  XCTestRunFile runFile(localRunFile);
  auto maybeTargets = runFile.getTargets();
  if (!maybeTargets) {
    llvm::report_fatal_error(llvm::toString(maybeTargets.takeError()));
  }
  for (auto target : *maybeTargets) {
    if (target != srcTestTarget) {
      runFile.deleteTestTarget(target);
    }
  }
}

using Mutants = const std::vector<std::unique_ptr<Mutant>>;

void GenerateXCRunFile(const std::string &srcRunFile,
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
    runFile.setBlueprintName(newTarget, mutant->getIdentifier());
  }
  runFile.deleteTestTarget(srcTestTarget);
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
        targetName(targetName), xcodebuildArgs(xcodebuildArgs),
        baseline(baseline) {}

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

static std::string ResultBundlePath(std::string resultBundleDir,
                                    std::string targetName, int taskID) {
  llvm::SmallString<128> resultBundlePath(resultBundleDir);
  llvm::sys::path::append(resultBundlePath, targetName + "-" +
                                                std::to_string(taskID) +
                                                ".xcresult");
  return resultBundlePath.str().str();
}

void MutantExecutionTask::operator()(iterator begin, iterator end, Out &storage,
                                     progress_counter &counter) {
  Runner runner(diagnostics);
  const std::string localRunFile =
      xctestrunFile + ".mull-xctrn-" + std::to_string(taskID) + ".xctestrun";

  GenerateXCRunFile(xctestrunFile, localRunFile, targetName, begin, end);
  std::string resultBundlePath =
      ResultBundlePath(resultBundleDir, targetName, taskID);
  ExecutionResult result;
  result = RunXcodeBuildTest(diagnostics, localRunFile, resultBundlePath,
                             xcodebuildArgs, {}, baseline.runningTime * 10,
                             configuration.captureMutantOutput);
  XCResultFile resultFile(resultBundlePath);
  auto failureTargets = resultFile.getFailureTestTargets();
  if (!failureTargets) {
    diagnostics.debug("no failure targets");
  }
  for (auto it = begin; it != end; ++it, counter.increment()) {
    ExecutionResult targetResult;
    targetResult.exitStatus = result.status;
    if (failureTargets && failureTargets->find(it->get()->getIdentifier()) !=
                              failureTargets->end()) {
      targetResult.status = Failed;
    } else {
      targetResult.status = Passed;
    }
    storage.push_back(
        std::make_unique<MutationResult>(targetResult, it->get()));
  }
}

} // namespace

std::unique_ptr<Result> XCTestRunInvocation::run() {
  auto mutants = extractMutantInfo();
  Runner runner(diagnostics);

  std::string singleTargetRunFile =
      runConfig.xctestrunFile.str() + ".mull-xctrn-base.xctestrun";
  GenerateSingleTargetXCRunFile(runConfig.xctestrunFile.str(),
                                singleTargetRunFile, runConfig.testTarget);

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    baseline = RunXcodeBuildTest(diagnostics, singleTargetRunFile, llvm::None,
                                 runConfig.xcodebuildArgs, {}, config.timeout,
                                 config.captureMutantOutput);
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(i, config, diagnostics, singleTargetRunFile,
                       runConfig.resultBundleDir, runConfig.testTarget,
                       runConfig.xcodebuildArgs, baseline);
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
XCTestRunInvocation::extractMutantInfo() {
  XCTestRunFile file(runConfig.xctestrunFile.str());
  auto products = file.getDependentProductPaths(runConfig.testTarget);
  if (!products) {
    diagnostics.error(llvm::toString(products.takeError()));
    return {};
  }

  std::vector<std::unique_ptr<mull::Mutant>> output;
  for (auto product : *products) {
    auto binaryPath = GetBundleBinaryPath(product);
    auto result = ExtractMutantInfo(binaryPath, factory, allPoints);
    if (!result) {
      continue;
    }
    std::move(result->begin(), result->end(), std::back_inserter(output));
  }

  return std::move(output);
}

}; // namespace mull_xctest
