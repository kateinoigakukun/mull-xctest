#ifndef MULL_XCTEST_SWIFT_SUPPORT_SOURCE_UNIT_STORAGE_H
#define MULL_XCTEST_SWIFT_SUPPORT_SOURCE_UNIT_STORAGE_H

#include <mull/Mutators/Mutator.h>
#include <string>
#include <unordered_map>

namespace mull_xctest {

using SourceFilePath = std::string;

class SourceUnitStorage {
  void *swiftStorage;

public:
  SourceUnitStorage(void *swiftStorage) : swiftStorage(swiftStorage) {}
  SourceUnitStorage(const SourceUnitStorage &) = delete;
  static std::unique_ptr<SourceUnitStorage> create(std::string filePath);
  bool hasMutation(int line, int column, mull::MutatorKind kind);
  void dump() const;
};

class SourceStorage {
  std::unordered_map<std::string, std::unique_ptr<SourceUnitStorage>> storage;

public:
  void saveMutations(const std::string &filePath,
                     std::unique_ptr<SourceUnitStorage> storage);
  bool hasMutation(const std::string &filePath, int line, int column,
                   mull::MutatorKind kind);
  void dump() const;
};
} // namespace mull_xctest

#endif
