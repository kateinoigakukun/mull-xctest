//
//  EmbedMutantInfoTask.hpp
//  mullXCTest
//
//  Created by kateinoigakukun on 2021/02/25.
//

#ifndef MULL_XCTEST_TASKS_EMBED_MUTANT_INFO_TASK_H
#define MULL_XCTEST_TASKS_EMBED_MUTANT_INFO_TASK_H

#include "mull/Bitcode.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Progress.h"
#include <vector>

namespace mull_xctest {


class EmbedMutantInfoTask {
public:
  using In = std::vector<std::unique_ptr<mull::Bitcode>>;
  using Out = std::vector<int>;
  using iterator = In::iterator;

  EmbedMutantInfoTask() {}

  void operator()(iterator begin, iterator end, Out &storage, mull::progress_counter &counter);

};

}


#endif
