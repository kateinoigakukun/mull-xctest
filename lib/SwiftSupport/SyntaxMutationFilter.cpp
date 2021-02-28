#include <mull/MutationPoint.h>
#include <llvm/IR/Instruction.h>
#include "MullXCTest/SwiftSupport/SyntaxMutationFilter.h"

#include <cassert>

using namespace mull_xctest::swift;

std::string SyntaxMutationFilter::name() {
  return "Syntax mutation filter for Swift";
}

bool SyntaxMutationFilter::shouldSkip(mull::MutationPoint *point) {
  llvm::Instruction *instruction = llvm::dyn_cast<llvm::Instruction>(point->getOriginalValue());
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

  auto sourceLocation = mull::SourceLocation::locationFromInstruction(instruction);
  return !storage.hasMutation(sourceLocation.filePath, sourceLocation.line, sourceLocation.column,
                              point->getMutator()->mutatorKind());
}
