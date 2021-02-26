#include "XCTestInvocation.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/MachOUniversal.h>
#include <llvm/Support/Path.h>
#include <mull/MutationResult.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Toolchain/Runner.h>
#include <set>
#include <sstream>

using namespace llvm;
using namespace mull;

namespace mull_xctest {

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

static std::string parseMutatorId(const std::string &id) {
  for (auto it = id.begin(); it != id.end(); ++it) {
    if (*it == ':') {
      return std::string(id.begin(), it - 1);
    }
  }
  llvm_unreachable("unexpected mutant id");
}

static SourceLocation parseSourceLoc(const std::string &identifier) {
  StringRef str(identifier);
  StringRef left;
  std::tie(left, str) = str.split(":");
  uint8_t index = 0;
  StringRef file;
  int line;
  int column;
  while (!str.empty()) {
    switch (index) {
    case 0: {
      break;
    }
    case 1: {
      file = left;
      break;
    }
    case 2: {
      line = std::stoi(left.str());
      break;
    }
    default: {
      llvm_unreachable("index should be < 3");
    }
    }
    index += 1;
    std::tie(left, str) = str.split(":");
  }

  assert(index == 3);
  column = std::stoi(left.str());
  StringRef directory, filename;
  std::tie(directory, filename) = file.rsplit("/");
  return SourceLocation(directory.str(), file.str(), directory.str(),
                        file.str(), line, column);
}

class RestoredMutator : public mull::Mutator {
public:
  std::string identifier;
  RestoredMutator(std::string identifier) : identifier(identifier) {}
  std::string getUniqueIdentifier() override { return identifier; }
  std::string getUniqueIdentifier() const override { return identifier; }
  mull::MutatorKind mutatorKind() override {
    return mull::MutatorKind::ScalarValueMutator;
  }
  std::string getDescription() const override { return "no description"; }
  std::vector<MutationPoint *>
  getMutations(Bitcode *bitcode, const FunctionUnderTest &function) override {
    return {};
  }
  void applyMutation(llvm::Function *function,
                     const MutationPointAddress &address,
                     irm::IRMutation *lowLevelMutation) override {}
};

std::vector<std::unique_ptr<mull::Mutant>> XCTestInvocation::extractMutantInfo(
    std::vector<std::unique_ptr<mull::Mutator>> &mutators,
    std::vector<std::unique_ptr<mull::MutationPoint>> &allPoints) {

  auto filename = llvm::sys::path::filename(testBundle);
  auto basename = filename.rsplit(".").first;
  SmallString<64> binaryPath(testBundle);
  llvm::sys::path::append(binaryPath, "Contents", "MacOS", basename);

  auto binaryOrErr = object::createBinary(binaryPath);
  if (!binaryOrErr) {
    std::stringstream errorMessage;
    errorMessage << "createBinary failed: \""
                 << toString(binaryOrErr.takeError()) << "\".";
    diagnostics.error(errorMessage.str());
    return {};
  }
  const auto *machOObjectFile =
      dyn_cast<object::MachOObjectFile>(binaryOrErr->getBinary());
  if (!machOObjectFile) {
    diagnostics.error("input file is not mach-o object file");
    return {};
  }
  Expected<object::SectionRef> section =
      machOObjectFile->getSection("__mull_mutants");
  if (!section) {
    llvm::consumeError(section.takeError());
    return {};
  }

  Expected<StringRef> contentsData = section->getContents();
  if (!contentsData) {
    std::stringstream errorMessage;
    errorMessage << "section->getContents failed: \""
                 << toString(contentsData.takeError()) << "\".";
    diagnostics.error(errorMessage.str());
  }
  llvm::SmallVector<llvm::StringRef, 32> splitIds;
  contentsData->split(splitIds, llvm::StringRef("\0", 1), -1,
                      /*KeepEmpty=*/false);
  std::sort(splitIds.begin(), splitIds.end());

  std::vector<std::unique_ptr<mull::Mutant>> mutants;
  auto uniqueTail = std::unique(splitIds.begin(), splitIds.end());
  for (auto it = splitIds.begin(); it != uniqueTail + 1; ++it) {
    auto mutatorIdentifier = parseMutatorId(it->str());
    auto loc = parseSourceLoc(it->str());
    if (loc.isNull())
      continue;
    mutators.push_back(std::make_unique<RestoredMutator>(mutatorIdentifier));
    std::vector<mull::MutationPoint *> points;
    allPoints.push_back(std::make_unique<MutationPoint>(
        mutators.back().get(), "<dummy_replacement>", it->str(), loc,
        "dummy_diagnostics"));
    points.push_back(allPoints.back().get());
    mutants.push_back(std::make_unique<Mutant>(it->str(), points));
  }
  return std::move(mutants);
}

}; // namespace mull_xctest
