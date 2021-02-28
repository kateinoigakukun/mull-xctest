#ifndef MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FILTER_H
#define MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FILTER_H

#include "MullXCTest/SwiftSupport/SourceStorage.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/MutationFilter.h>

namespace mull_xctest {

namespace swift {

class SyntaxMutationFilter : public mull::MutationFilter {
public:
  SyntaxMutationFilter(mull::Diagnostics &diagnostics,
                       SourceStorage storage)
    : diagnostics(diagnostics), storage(std::move(storage)) {}
  bool shouldSkip(mull::MutationPoint *point) override;
  std::string name() override;

private:
  mull::Diagnostics &diagnostics;
  SourceStorage storage;
};
} // namespace swift

} // namespace mull_xctest

#endif
