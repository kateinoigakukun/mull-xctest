#include "MullXCTest/SwiftSupport/SyntaxMutationFinder.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Parallelization/Progress.h>
#include <mull/Parallelization/TaskExecutor.h>

using namespace mull_xctest::swift;

class IndexSwiftSourceTask {
public:
  using In = std::vector<SourceFilePath>;
  using Out = std::vector<
      std::pair<SourceFilePath, std::unique_ptr<SourceUnitStorage>>>;
  using iterator = In::iterator;

  IndexSwiftSourceTask() {}

  void operator()(iterator begin, iterator end, Out &storage,
                  mull::progress_counter &counter);
};

void IndexSwiftSourceTask::operator()(iterator begin, iterator end,
                                      Out &storage,
                                      mull::progress_counter &counter) {
  for (auto it = begin; it != end; it++, counter.increment()) {
    std::string sourcePath = *it;
    std::unique_ptr<SourceUnitStorage> unit =
        SourceUnitStorage::create(sourcePath);
    if (!unit)
      continue;
    storage.emplace_back(sourcePath, std::move(unit));
  }
}

SourceStorage
SyntaxMutationFinder::findMutations(std::vector<SourceFilePath> sources,
                                    mull::Diagnostics &diagnostics,
                                    const mull::Configuration &config) {
  std::vector<std::pair<SourceFilePath, std::unique_ptr<SourceUnitStorage>>>
      mutationsAsVector;
  std::vector<IndexSwiftSourceTask> tasks(config.parallelization.workers);
  mull::TaskExecutor indexer(diagnostics, "Syntax Index", sources,
                             mutationsAsVector, tasks);
  indexer.execute();

  SourceStorage storage;
  for (auto &pair : mutationsAsVector) {
    storage.saveMutations(pair.first, std::move(pair.second));
  }
  return std::move(storage);
}
