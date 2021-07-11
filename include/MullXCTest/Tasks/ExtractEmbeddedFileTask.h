#ifndef MULL_XCTEST_TASKS_EXTRACT_EMBEDDED_FILE_TASK_H
#define MULL_XCTEST_TASKS_EXTRACT_EMBEDDED_FILE_TASK_H

#include <llvm/Support/MemoryBuffer.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Parallelization/Progress.h>
#include <vector>

namespace mull_xctest {

std::unique_ptr<llvm::MemoryBuffer>
extractEmbeddedFile(llvm::StringRef inputFile,
                    mull::Diagnostics &diagnostics);

class ExtractEmbeddedFileTask {
public:
  using In = std::vector<std::string>;
  using Out = std::vector<std::unique_ptr<llvm::MemoryBuffer>>;
  using iterator = In::iterator;

  ExtractEmbeddedFileTask(mull::Diagnostics &diagnostics)
      : diagnostics(diagnostics) {}

  void operator()(iterator begin, iterator end, Out &storage,
                  mull::progress_counter &counter);

private:
  mull::Diagnostics &diagnostics;
};

} // namespace mull_xctest

#endif
