#ifndef MULL_LD_LINKER_OPTIONS_H
#define MULL_LD_LINKER_OPTIONS_H

#include <lld/Common/LLVM.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Option/OptTable.h>
#include <llvm/Option/Option.h>

namespace mull_xctest {

class LinkerOptTable : public llvm::opt::OptTable {
protected:
  LinkerOptTable(llvm::ArrayRef<Info> OptionInfos) : OptTable(OptionInfos) {}

public:
  virtual void collectObjectFiles(const llvm::opt::InputArgList &,
                                  std::vector<llvm::StringRef> &) = 0;
  virtual void collectObjectLinkOpts(const llvm::opt::InputArgList &,
                                     llvm::opt::ArgStringList &output) = 0;
};

LinkerOptTable *GetLinkerOptTable(std::string flavor);

class LinkerOptions {
  LinkerOptTable &optTable;
  const llvm::opt::InputArgList &args;

public:
  LinkerOptions(LinkerOptTable &optTable, const llvm::opt::InputArgList &args)
      : optTable(optTable), args(args) {}

  void collectObjectFiles(std::vector<llvm::StringRef> &objects) {
    optTable.collectObjectFiles(args, objects);
  }

  void collectObjectLinkOpts(llvm::opt::ArgStringList &output) {
    optTable.collectObjectLinkOpts(args, output);
  }
};

class LD64OptTable : public LinkerOptTable {

public:
  LD64OptTable();
  virtual void collectObjectFiles(const llvm::opt::InputArgList &,
                                  std::vector<llvm::StringRef> &) override;
  virtual void collectObjectLinkOpts(const llvm::opt::InputArgList &,
                                     llvm::opt::ArgStringList &output) override;
};

namespace ld64 {

enum {
  OPT_INVALID = 0,
#define OPTION(_1, _2, ID, _4, _5, _6, _7, _8, _9, _10, _11, _12) OPT_##ID,
#include "ld64/Options.inc"
#undef OPTION
};

} // namespace ld64

class ClangOptTable : public LinkerOptTable {
public:
  ClangOptTable();
  virtual void collectObjectFiles(const llvm::opt::InputArgList &,
                                  std::vector<llvm::StringRef> &) override;
  virtual void collectObjectLinkOpts(const llvm::opt::InputArgList &,
                                     llvm::opt::ArgStringList &output) override;
};

namespace clang {

enum ClangFlags {
  DriverOption = (1 << 4),
  LinkerInput = (1 << 5),
  NoArgumentUnused = (1 << 6),
  Unsupported = (1 << 7),
  CoreOption = (1 << 8),
  CLOption = (1 << 9),
  CC1Option = (1 << 10),
  CC1AsOption = (1 << 11),
  NoDriverOption = (1 << 12),
  Ignored = (1 << 13)
};

enum {
  OPT_INVALID = 0,
#define OPTION(_1, _2, ID, _4, _5, _6, _7, _8, _9, _10, _11, _12) OPT_##ID,
#include "clang/Options.inc"
#undef OPTION
};

} // namespace clang

} // namespace mull_xctest
#endif
