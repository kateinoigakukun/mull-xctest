#ifndef MULL_XCTEST_TASKS_EXTRACT_EMBEDDED_FILE_TASK_H
#define MULL_XCTEST_TASKS_EXTRACT_EMBEDDED_FILE_TASK_H

#include <llvm/IR/LLVMContext.h>
#include "ebc/EmbeddedFile.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Progress.h"
#include <vector>

namespace mull_xctest {


class ExtractEmbeddedFileTask {
public:
  using In = std::vector<llvm::StringRef>;
  using Out = std::vector<std::unique_ptr<ebc::EmbeddedFile>>;
  using iterator = In::iterator;

    ExtractEmbeddedFileTask(mull::Diagnostics &diagnostics)
      : diagnostics(diagnostics) {}

  void operator()(iterator begin, iterator end, Out &storage, mull::progress_counter &counter);

private:
  mull::Diagnostics &diagnostics;
};

}

#endif
