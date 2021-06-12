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

bool isUnrecoverableFunction(llvm::Function *f) {
  const std::vector<std::string> functions = {
    "$ss12precondition__4file4lineySbyXK_SSyXKs12StaticStringVSutFfA0_SSycfu_",
    "$ss17_assertionFailure__4file4line5flagss5NeverOs12StaticStringV_SSAHSus6UInt32VtF",
    "$ss16assertionFailure_4file4lineySSyXK_s12StaticStringVSutF",
  };
  for (auto funcName : functions) {
    if (f->getName().equals(funcName)) {
      return true;
    }
  }
  return false;
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
  if (!isUserSwiftFunc) {
    return true;
  }
  if (isUnrecoverableFunction(fn)) {
    return true;
  }
  return false;
}
