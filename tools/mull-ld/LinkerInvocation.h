#ifndef LinkerInvocation_h
#define LinkerInvocation_h

#include "LinkerOptions.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/Option/OptTable.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/Filters.h>
#include <mull/FunctionUnderTest.h>
#include <mull/MutationPoint.h>
#include <mull/MutationsFinder.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Program/Program.h>
#include <vector>

namespace mull_xctest {
void link(std::vector<std::string> objectFiles, LinkerOptions &linkerOpts,
          const mull::Configuration &config, mull::Diagnostics &diagnostics);
} // namespace mull_xctest

#endif
