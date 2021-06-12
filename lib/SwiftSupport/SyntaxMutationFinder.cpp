#include "MullXCTest/SwiftSupport/SyntaxMutationFinder.h"
#include "MullXCTest/PthreadTaskExecutor.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Parallelization/Progress.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull-c/AST/ASTMutationStorage.h>

using namespace mull_xctest::swift;
using namespace mull_xctest;

extern "C" void *mull_swift_IndexSource(const char *sourcePath,
                                        CASTMutationStorage storage,
                                        int *errorCode);

class IndexSwiftSourceTask {
public:
  using In = std::set<SourceFilePath>;
  using Out = std::vector<
      std::pair<SourceFilePath, std::unique_ptr<ASTMutationStorage>>>;
  using iterator = In::iterator;
  mull::Diagnostics &diags;

  IndexSwiftSourceTask(mull::Diagnostics &diags) : diags(diags) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  mull::progress_counter &counter);
};

void IndexSwiftSourceTask::operator()(iterator begin, iterator end,
                                      Out &out,
                                      mull::progress_counter &counter) {
  for (auto it = begin; it != end; it++, counter.increment()) {
    std::string sourcePath = *it;
    int errorCode;
    auto storage = std::make_unique<ASTMutationStorage>(diags);
    mull_swift_IndexSource(sourcePath.c_str(), storage.get(), &errorCode);
    out.emplace_back(sourcePath, std::move(storage));
  }
}

void
SyntaxMutationFinder::findMutations(std::set<SourceFilePath> &sources,
                                    mull::ASTMutationStorage &astMutationStorage,
                                    mull::Diagnostics &diagnostics,
                                    const mull::Configuration &config) {
  std::vector<std::pair<SourceFilePath, std::unique_ptr<ASTMutationStorage>>>
      mutationsAsVector;
  std::vector<IndexSwiftSourceTask> tasks;
  for (int i = 0; i < config.parallelization.workers; i++) {
    tasks.emplace_back(diagnostics);
  }
  constexpr size_t desiredStackSize = 8 << 20;
  mull_xctest::PthreadTaskExecutor indexer(diagnostics, "Syntax Index",
                                           desiredStackSize, sources,
                                           mutationsAsVector, tasks);
  indexer.execute();

  for (auto &pair : mutationsAsVector) {
    for (auto mutation : pair.second->storage) {
      astMutationStorage.storage.emplace(mutation.first, mutation.second);
    }
  }
}
