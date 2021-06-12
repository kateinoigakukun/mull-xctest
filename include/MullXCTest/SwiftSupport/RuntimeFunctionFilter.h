#ifndef MULL_XCTEST_SWIFT_SUPPORT_RUNTIME_FUNCTION_FILTER_H
#define MULL_XCTEST_SWIFT_SUPPORT_RUNTIME_FUNCTION_FILTER_H

#include <mull/AST/ASTMutationStorage.h>
#include <mull/Config/Configuration.h>
#include <mull/Filters/MutationFilter.h>
#include <set>

namespace mull_xctest {
namespace swift {

class RuntimeFunctionFilter : public mull::MutationFilter {
public:
  RuntimeFunctionFilter(mull::Diagnostics &diagnostics);
  bool shouldSkip(mull::MutationPoint *point) override;
  std::string name() override;

private:
  mull::Diagnostics &diagnostics;
};
} // namespace swift
} // namespace mull_xctest

#endif
