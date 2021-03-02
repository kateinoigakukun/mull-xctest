#include "LinkerOptions.h"
#include <llvm/Option/ArgList.h>
#include <llvm/Support/Debug.h>

using namespace llvm;
using namespace llvm::opt;

namespace mull_xctest {

namespace ld64 {

#define PREFIX(NAME, VALUE) const char *NAME[] = VALUE;
#include "ld64/Options.inc"
#undef PREFIX

static const ::llvm::opt::OptTable::Info optInfo[] = {
#define OPTION(X1, X2, ID, KIND, GROUP, ALIAS, X7, X8, X9, X10, X11, X12)      \
  {X1,                                                                         \
   X2,                                                                         \
   X10,                                                                        \
   X11,                                                                        \
   mull_xctest::ld64::OPT_##ID,                                                \
   opt::Option::KIND##Class,                                                   \
   X9,                                                                         \
   X8,                                                                         \
   mull_xctest::ld64::OPT_##GROUP,                                             \
   mull_xctest::ld64::OPT_##ALIAS,                                             \
   X7,                                                                         \
   X12},
#include "ld64/Options.inc"
#undef OPTION
};

} // namespace ld64

LD64OptTable::LD64OptTable() : LinkerOptTable(ld64::optInfo) {}

void LD64OptTable::collectObjectFiles(
    const llvm::opt::InputArgList &args,
    std::vector<llvm::StringRef> &objectFiles) {
  for (auto input : args.filtered(ld64::OPT_INPUT)) {
    objectFiles.push_back(input->getValue());
  }
}

void LD64OptTable::collectObjectLinkOpts(const llvm::opt::InputArgList &args,
                                         llvm::opt::ArgStringList &output) {
  for (auto it = args.begin(); it != args.end(); ++it) {
    auto arg = *it;
    if (arg->getOption().matches(ld64::OPT_add_ast_path)) {
      // workaround: add_ast_path is not supported in lld
      // but it takes a value, so render it.
      arg->renderAsInput(args, output);
      ++it;
      arg = *it;
      arg->renderAsInput(args, output);
    } else if (arg->getOption().matches(ld64::OPT_bitcode_bundle)) {
      // -bitcode_bundle and -bundle can't be appeared at once
      continue;
    } else if (!arg->getOption().matches(ld64::OPT_INPUT)) {
      arg->render(args, output);
    }
  }
}

namespace clang {

#define PREFIX(NAME, VALUE) const char *NAME[] = VALUE;
#include "clang/Options.inc"
#undef PREFIX

static const ::llvm::opt::OptTable::Info optInfo[] = {
#define OPTION(X1, X2, ID, KIND, GROUP, ALIAS, X7, X8, X9, X10, X11, X12)      \
  {X1,                                                                         \
   X2,                                                                         \
   X10,                                                                        \
   X11,                                                                        \
   mull_xctest::clang::OPT_##ID,                                               \
   opt::Option::KIND##Class,                                                   \
   X9,                                                                         \
   X8,                                                                         \
   mull_xctest::clang::OPT_##GROUP,                                            \
   mull_xctest::clang::OPT_##ALIAS,                                            \
   X7,                                                                         \
   X12},
#include "clang/Options.inc"
#undef OPTION
};

} // namespace clang

ClangOptTable::ClangOptTable() : LinkerOptTable(clang::optInfo) {}

void ClangOptTable::collectObjectFiles(
    const llvm::opt::InputArgList &args,
    std::vector<llvm::StringRef> &objectFiles) {
  for (auto input : args.filtered(clang::OPT_INPUT)) {
    objectFiles.push_back(input->getValue());
  }
}

void ClangOptTable::collectObjectLinkOpts(const llvm::opt::InputArgList &args,
                                          llvm::opt::ArgStringList &output) {
  for (auto arg : args) {
    if (!arg->getOption().matches(clang::OPT_INPUT)) {
      arg->render(args, output);
    }
  }
}

LinkerOptTable *GetLinkerOptTable(std::string flavor) {
  if (flavor == "ld64") {
    return new LD64OptTable();
  } else if (flavor == "clang") {
    return new ClangOptTable();
  } else {
    return nullptr;
  }
}

} // namespace mull_xctest
