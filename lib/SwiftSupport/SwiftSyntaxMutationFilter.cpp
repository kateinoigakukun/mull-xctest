#include "MullXCTest/SwiftSupport/SwiftSyntaxMutationFilter.h"
#include <llvm/IR/Instruction.h>
#include <mull/MutationPoint.h>

#include <cassert>
#include <sstream>

using namespace mull_xctest::swift;

std::string SwiftSyntaxMutationFilter::name() {
  return "Syntax mutation filter for Swift";
}

bool SwiftSyntaxMutationFilter::shouldSkip(mull::MutationPoint *point) {
  llvm::Instruction *instruction =
      llvm::dyn_cast<llvm::Instruction>(point->getOriginalValue());
  assert(instruction);

  const llvm::DebugLoc &debugInfo = instruction->getDebugLoc();
  if (!debugInfo) {
    return true;
  }

  int line = debugInfo.getLine();
  int column = debugInfo.getCol();

  if (line <= 0 || column <= 0) {
    assert(0 && "ASTMutationFilter: Unknown edge case.");
  }

  auto sourceLocation =
      mull::SourceLocation::locationFromInstruction(instruction);
  bool accept = storage.hasMutation(sourceLocation.filePath, sourceLocation.line,
                                    sourceLocation.column,
                                    point->getMutator()->mutatorKind());
  std::stringstream message;
  message << (accept ? "Accepted " : "Filtered ") << point->dump() << "\n";
  diagnostics.debug(message.str());
  return !accept;
}
