#include "MullXCTest/SwiftSupport/SourceStorage.h"
#include <llvm/Support/raw_ostream.h>

using namespace mull_xctest;

void SourceStorage::dump() const {
  for (auto &pair : storage) {
    llvm::outs() << pair.first << "\n";
    pair.second->dump();
  }
}

void SourceStorage::saveMutations(
    const std::string &filePath,
    std::unique_ptr<SourceUnitStorage> unitStorage) {
  this->storage.emplace(filePath, std::move(unitStorage));
}

bool SourceStorage::hasMutation(const std::string &filePath, int line,
                                int column, mull::MutatorKind kind) {
  auto found = storage.find(filePath);
  if (found == storage.end()) {
    return false;
  }
  auto &unitStorage = found->second;
  return unitStorage->hasMutation(line, column, kind);
}

extern "C" void *mullHasSyntaxMutation(void *storage, int line, int column,
                                       int rawMutatorKind);
extern "C" void *mullIndexSwiftSource(const unsigned char *sourcePath,
                                      int *errorCode);
extern "C" void mullDumpSourceUnitStorage(void *storage);

std::unique_ptr<SourceUnitStorage>
SourceUnitStorage::create(SourceFilePath filePath) {
  int ec = 0;
  auto sourcePath_c = reinterpret_cast<const unsigned char *>(filePath.c_str());
  void *opaqueStorage = mullIndexSwiftSource(sourcePath_c, &ec);
  if (ec != 0) {
    return nullptr;
  }
  return std::make_unique<SourceUnitStorage>(opaqueStorage);
}

bool SourceUnitStorage::hasMutation(int line, int column,
                                    mull::MutatorKind kind) {
  return mullHasSyntaxMutation(this->swiftStorage, line, column,
                               static_cast<int>(kind));
}

void SourceUnitStorage::dump() const {
  mullDumpSourceUnitStorage(this->swiftStorage);
}
