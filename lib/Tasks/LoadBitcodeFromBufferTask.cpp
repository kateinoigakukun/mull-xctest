#include "MullXCTest/Tasks/LoadBitcodeFromBufferTask.h"

#include <mull/BitcodeLoader.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Parallelization/Progress.h>

#include <llvm/Support/MemoryBuffer.h>

namespace mull_xctest {

std::unique_ptr<mull::Bitcode>
loadBitcodeFromBuffer(std::unique_ptr<llvm::MemoryBuffer> buffer,
                      mull::Diagnostics &diagnostics) {
  auto context = std::make_unique<llvm::LLVMContext>();
  auto module =
      mull::loadModuleFromBuffer(*context, *buffer.get(), diagnostics);

  if (module == nullptr) {
    diagnostics.warning(
        "Bitcode module could not be loaded. Possible reason: the bitcode "
        "file could be built with a newer version of LLVM than is used by "
        "Mull.");
    return nullptr;
  }

  /// How can I check that -g flag (debug info enable) was set, from llvm pass
  /// https://stackoverflow.com/a/21713717/598057
  if (module->getNamedMetadata("llvm.dbg.cu") == nullptr) {
    diagnostics.warning("Bitcode module does not have debug information.");
  }

  assert(module && "Could not load module");

  auto bitcode =
      std::make_unique<mull::Bitcode>(std::move(context), std::move(module));
  return bitcode;
}

void LoadBitcodeFromBufferTask::operator()(iterator begin, iterator end,
                                           Out &storage,
                                           mull::progress_counter &counter) {
  for (auto it = begin; it != end; it++, counter.increment()) {

    auto context = std::make_unique<llvm::LLVMContext>();
    auto module =
        mull::loadModuleFromBuffer(*context, *std::move(*it), diagnostics);

    if (module == nullptr) {
      diagnostics.warning(
          "Bitcode module could not be loaded. Possible reason: the bitcode "
          "file could be built with a newer version of LLVM than is used by "
          "Mull.");
      continue;
    }

    /// How can I check that -g flag (debug info enable) was set, from llvm pass
    /// https://stackoverflow.com/a/21713717/598057
    if (module->getNamedMetadata("llvm.dbg.cu") == nullptr) {
      diagnostics.warning("Bitcode module does not have debug information.");
    }

    assert(module && "Could not load module");

    auto bitcode =
        std::make_unique<mull::Bitcode>(std::move(context), std::move(module));
    storage.push_back(std::move(bitcode));
  }
}
} // namespace mull_xctest
