#include "MullXCTest/MutantMetadata.h"
#include "MullXCTest/MutantSerialization.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <mull/MutationPoint.h>
#include <set>

using namespace mull_xctest;
using namespace mull;
using namespace llvm;

static void collectGlobalList(llvm::Module &module,
                              llvm::SmallVectorImpl<llvm::WeakTrackingVH> &list,
                              llvm::StringRef name) {
  if (auto *existing = module.getGlobalVariable(name)) {
    auto *globals = llvm::cast<llvm::ConstantArray>(existing->getInitializer());
    for (auto &use : globals->operands()) {
      auto *global = use.get();
      list.push_back(global);
    }
    existing->eraseFromParent();
  }

  std::for_each(
      list.begin(), list.end(), [](const llvm::WeakTrackingVH &global) {
        assert(!llvm::isa<llvm::GlobalValue>(global) ||
               !llvm::cast<llvm::GlobalValue>(global)->isDeclaration() &&
                   "all globals in the 'used' list must be definitions");
      });
}

static void emitGlobalList(Module &module,
                           ArrayRef<llvm::WeakTrackingVH> handles,
                           StringRef name, StringRef section,
                           llvm::GlobalValue::LinkageTypes linkage,
                           llvm::Type *eltTy, bool isConstant) {
  // Do nothing if the list is empty.
  if (handles.empty())
    return;
  auto alignment = module.getDataLayout().getPointerSize();

  // We have an array of value handles, but we need an array of constants.
  SmallVector<llvm::Constant *, 8> elts;
  elts.reserve(handles.size());
  for (auto &handle : handles) {
    auto elt = cast<llvm::Constant>(&*handle);
    if (elt->getType() != eltTy)
      elt = llvm::ConstantExpr::getBitCast(elt, eltTy);
    elts.push_back(elt);
  }

  auto varTy = llvm::ArrayType::get(eltTy, elts.size());
  auto init = llvm::ConstantArray::get(varTy, elts);
  auto var =
      new llvm::GlobalVariable(module, varTy, isConstant, linkage, init, name);
  var->setSection(section);
  var->setAlignment(llvm::MaybeAlign(alignment));
}

std::unique_ptr<llvm::Module> CreateMetadataModule(std::vector<std::unique_ptr<Mutant>> &mutants,
                                                   llvm::LLVMContext &context) {
  auto module = std::make_unique<llvm::Module>("mull-xctest.metadata", context);

  if (mutants.empty()) {
    module->appendModuleInlineAsm(".section " MULL_MUTANTS_INFO_SECTION);
    return std::move(module);
  }

  std::string entriesString;
  llvm::raw_string_ostream stream(entriesString);
  MutantSerializer serializer(stream);
  std::set<std::string> entries;
  for (auto &mutant : mutants) {
    serializer.serialize(mutant.get());
  }

  auto constantContent = llvm::ConstantDataArray::getString(
      module->getContext(), entriesString, /*AddNull=*/false);
  auto *var = new llvm::GlobalVariable(*module, constantContent->getType(),
                                       true, llvm::GlobalValue::PrivateLinkage,
                                       constantContent, "_mull_mutants");
  var->setSection(MULL_MUTANTS_INFO_SECTION);
  var->setAlignment(llvm::Align());

  llvm::SmallVector<llvm::WeakTrackingVH, 4> LLVMUsed;
  collectGlobalList(*module, LLVMUsed, "llvm.used");
  LLVMUsed.push_back(var);
  emitGlobalList(*module, LLVMUsed, "llvm.used", "llvm.metadata",
                 llvm::GlobalValue::AppendingLinkage,
                 llvm::Type::getInt8PtrTy(module->getContext()), false);
  return std::move(module);
}
