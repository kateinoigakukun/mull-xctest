#ifndef MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FILTER_H
#define MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FILTER_H

#include "MullXCTest/SwiftSupport/SourceStorage.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/MutationFilter.h>

namespace mull_xctest {

class SyntaxMutationIndexer {
  virtual void index(const std::string &sourcePath, SourceStorage &storage) = 0;
};

namespace swift {

class SwiftSyntaxMutationFilter : public mull::MutationFilter {
public:
  SwiftSyntaxMutationFilter(mull::Diagnostics &diagnostics, SourceStorage storage)
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
