#ifndef MULL_XCTEST_MUTANT_METADATA_H
#define MULL_XCTEST_MUTANT_METADATA_H

#include <vector>
#include <mull/Mutant.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>

#define MULL_XCTEST_METADATA_TOSTR2(X) #X
#define MULL_XCTEST_METADATA_TOSTR(X) MULL_XCTEST_METADATA_TOSTR2(X)
#define MACHO_SECTION(SEGMENT, SECTION)                                        \
  MULL_XCTEST_METADATA_TOSTR(SEGMENT) ", " MULL_XCTEST_METADATA_TOSTR(SECTION)
#define MULL_MUTANTS_INFO_SECTION_NAME __mull_mutants
#define MULL_MUTANTS_INFO_SECTION_NAME_STR                                     \
  MULL_XCTEST_METADATA_TOSTR(MULL_MUTANTS_INFO_SECTION_NAME)
#define MULL_MUTANTS_INFO_SECTION                                              \
  MACHO_SECTION(__DATA, MULL_MUTANTS_INFO_SECTION_NAME)

std::unique_ptr<llvm::Module>
CreateMetadataModule(std::vector<std::unique_ptr<mull::Mutant>> &mutants,
                     llvm::LLVMContext &context);
#endif
