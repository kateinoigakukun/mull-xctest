#include "MullXCTest/SwiftSupport/RuntimeFunctionFilter.h"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <mull/MutationPoint.h>

using namespace mull_xctest;
using namespace mull_xctest::swift;

RuntimeFunctionFilter::RuntimeFunctionFilter(mull::Diagnostics &diagnostics)
    : diagnostics(diagnostics) {}

std::string RuntimeFunctionFilter::name() {
  return "Swift runtime function filter";
}

bool RuntimeFunctionFilter::shouldSkip(mull::MutationPoint *point) {
  if (point->getMutator()->mutatorKind() !=
      mull::MutatorKind::CXX_RemoveVoidCall) {
    return false;
  }
  llvm::Instruction *instruction =
      llvm::dyn_cast<llvm::Instruction>(point->getOriginalValue());
  assert(instruction);
  if (instruction->getOpcode() != llvm::Instruction::Call) {
    return false;
  }
  llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(instruction);
  if (!call->getCalledFunction()) {
    return false;
  }

  llvm::Function *fn = call->getCalledFunction();
  bool isUserSwiftFunc = fn->getName().startswith("$s");
  return !isUserSwiftFunc;
}
