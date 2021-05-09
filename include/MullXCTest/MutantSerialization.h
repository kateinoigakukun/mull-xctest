#ifndef MULL_XCTEST_MUTANT_SERIALIZATION_H
#define MULL_XCTEST_MUTANT_SERIALIZATION_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <mull/Mutant.h>
#include <mull/Mutators/MutatorsFactory.h>

namespace mull_xctest {

using MutantList = std::vector<std::unique_ptr<mull::Mutant>>;

llvm::Expected<MutantList> ExtractMutantInfo(
    std::string binaryPath, mull::MutatorsFactory &factory, mull::Diagnostics &);

class MutantSerializer {
  llvm::raw_ostream &output;
  void writeInt(int);
  void writeString(const std::string &);

public:
  MutantSerializer(llvm::raw_ostream &output) : output(output) {}
  void serializeMetadata();
  void serialize(mull::Mutant *point);
};

class MutantDeserializer {
  llvm::StringRef buffer;
  mull::MutatorsFactory &factory;

  size_t cursor = 0;
  uint8_t readByte();
  int readInt();
  std::string readString();

public:
  MutantDeserializer(llvm::StringRef buffer, mull::MutatorsFactory &factory)
      : buffer(buffer), factory(factory) {}
  bool isEOF() const;

  /// \returns true if an error occurred.
  bool consumeMetadata();
  std::unique_ptr<mull::Mutant> deserialize();
};

} // namespace mull_xctest

#endif
