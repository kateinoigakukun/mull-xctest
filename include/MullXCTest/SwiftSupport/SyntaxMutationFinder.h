#ifndef MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FINDER_H
#define MULL_XCTEST_SWIFT_SUPPORT_SYNTAX_MUTATION_FINDER_H

#include "MullXCTest/SwiftSupport/SourceStorage.h"
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/AST/ASTMutationStorage.h>
#include <set>

namespace mull_xctest {
namespace swift {

class SyntaxMutationFinder {
public:
  void findMutations(std::set<SourceFilePath> &sources,
                     mull::ASTMutationStorage &astMutationStorage,
                     mull::Diagnostics &diagnostics,
                     const mull::Configuration &config);
};
} // namespace swift
} // namespace mull_xctest

#endif
