#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/MutationFilter.h>
#include <mull/MutationPoint.h>
#include <llvm/IR/Instruction.h>
// #include "SwiftRawSyntax.h"

#include <cassert>

namespace mull_xctest {

class SyntaxNode {
//   std::unique_ptr<swiftparse_client_node_t> node;
};

class SwiftASTStorage {
  
};

class SwiftASTMutationFilter: public mull::MutationFilter {
public:
  SwiftASTMutationFilter(mull::Diagnostics &diagnostics, SwiftASTStorage &astStorage);
  bool shouldSkip(mull::MutationPoint *point) override ;
  std::string name() override;

private:
  mull::Diagnostics &diagnostics;
  SwiftASTStorage &astStorage;
};
}


using namespace mull_xctest;


bool SwiftASTMutationFilter::shouldSkip(mull::MutationPoint *point) {
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

  return true;
}


