#include "XCTestRunInvocation.h"
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

ExecutionResult RunProgram(Diagnostics &diagnostics, const std::string &program,
                           const std::vector<std::string> &arguments,
                           const std::vector<std::string> &environment,
                           long long int timeout, const std::string &logPath) {
  using namespace std::string_literals;

  std::vector<std::pair<std::string, std::string>> env;
  env.reserve(environment.size());
  for (auto &e : environment) {
    env.emplace_back(e, "1");
  }

  reproc::options options;
  options.env.extra = reproc::env(env);
  if (!logPath.empty()) {
    options.redirect.path = logPath.c_str();
  } else {
    options.redirect.err.type = reproc::redirect::type::pipe;
  }

  std::vector<std::string> allArguments{program};
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

ExecutionResult
RunXcodeBuildTest(Diagnostics &diagnostics, const std::string &xctestrunFile,
                  const llvm::Optional<std::string> &resultBundlePath,
                  const std::vector<std::string> &extraArgs,
                  const std::vector<std::string> &environment,
                  long long int timeout, const std::string &logPath) {
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

  return RunProgram(diagnostics, "/usr/bin/xcodebuild", arguments, environment,
                    timeout, logPath);
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
                      const XCTestRunConfig &runConfig,
                      ExecutionResult &baseline)
      : taskID(taskID), configuration(configuration), diagnostics(diagnostics),
        xctestrunFile(xctestrunFile), runConfig(runConfig), baseline(baseline) {
  }

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  const int taskID;
  const Configuration &configuration;
  const XCTestRunConfig &runConfig;
  Diagnostics &diagnostics;
  const std::string xctestrunFile;
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

  GenerateXCRunFile(xctestrunFile, localRunFile, runConfig.testTarget, begin,
                    end);
  std::string resultBundlePath =
      ResultBundlePath(runConfig.resultBundleDir, runConfig.testTarget, taskID);
  ExecutionResult result;
  llvm::SmallString<128> logPath(runConfig.logPath);
  if (!logPath.empty())
    llvm::sys::path::append(logPath, std::to_string(taskID) + ".log");

  int count = std::distance(begin, end);
  result = RunXcodeBuildTest(
      diagnostics, localRunFile, resultBundlePath, runConfig.xcodebuildArgs, {},
      baseline.runningTime * count * 10, logPath.str().str());
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
  if (mutants.empty()) {
    return std::make_unique<Result>(
        std::move(mutants), std::vector<std::unique_ptr<MutationResult>>{});
  }

  Runner runner(diagnostics);

  std::string singleTargetRunFile =
      runConfig.xctestrunFile.str() + ".mull-xctrn-base.xctestrun";
  GenerateSingleTargetXCRunFile(runConfig.xctestrunFile.str(),
                                singleTargetRunFile, runConfig.testTarget);

  ExecutionResult baseline;
  singleTask.execute("Baseline run", [&]() {
    llvm::SmallString<128> logPath(runConfig.logPath);
    if (!logPath.empty())
      llvm::sys::path::append(logPath, "baseline.log");

    baseline = RunXcodeBuildTest(diagnostics, singleTargetRunFile, llvm::None,
                                 runConfig.xcodebuildArgs, {}, config.timeout,
                                 logPath.str().str());
  });

  std::vector<std::unique_ptr<MutationResult>> mutationResults;
  std::vector<MutantExecutionTask> tasks;
  tasks.reserve(config.parallelization.mutantExecutionWorkers);
  for (int i = 0; i < config.parallelization.mutantExecutionWorkers; i++) {
    tasks.emplace_back(i, config, diagnostics, singleTargetRunFile, runConfig,
                       baseline);
  }
  TaskExecutor<MutantExecutionTask> mutantRunner(diagnostics, "Running mutants",
                                                 mutants, mutationResults,
                                                 std::move(tasks));
  mutantRunner.execute();

  return std::make_unique<Result>(std::move(mutants),
                                  std::move(mutationResults));
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
    auto result = ExtractMutantInfo(binaryPath, factory, diagnostics);
    if (!result) {
      continue;
    }
    std::move(result->begin(), result->end(), std::back_inserter(output));
  }

  return std::move(output);
}

}; // namespace mull_xctest
