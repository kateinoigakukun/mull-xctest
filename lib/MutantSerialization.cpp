#include "MullXCTest/MutantSerialization.h"
#include <llvm/Support/Endian.h>
#include <mull/MutationPoint.h>
#include <mull/Mutators/Mutator.h>

using namespace mull_xctest;

void MutantSerializer::writeInt(int value) {
  char bytes[4];
  llvm::support::endian::write32be(bytes, value);
  output.write(bytes, 4);
}
void MutantSerializer::writeString(const std::string &str) {
  output.write(str.c_str(), str.size());
  output.write('\0');
}
void MutantSerializer::serialize(mull::MutationPoint *point) {
  writeString(point->getMutator()->getUniqueIdentifier());

  const auto &loc = point->getSourceLocation();
  writeString(loc.directory);
  writeString(loc.filePath);
  writeString(loc.unitDirectory);
  writeString(loc.unitFilePath);
  writeInt(loc.line);
  writeInt(loc.column);

  writeString(point->getReplacement());
  writeString(point->getDiagnostics());
}

uint8_t MutantDeserializer::readByte() {
  uint8_t byte = *(buffer.data() + cursor);
  cursor += 1;
  return byte;
}

std::string MutantDeserializer::readString() {
  const char *bytes = buffer.data() + cursor;
  auto str = std::string(bytes);
  cursor += str.size() + 1;
  return str;
}

bool MutantDeserializer::isEOF() const { return cursor >= buffer.size(); }

int MutantDeserializer::readInt() {
  int value = llvm::support::endian::read32be(buffer.data() + cursor);
  cursor += 4;
  return value;
}

std::unique_ptr<mull::MutationPoint> MutantDeserializer::deserialize() {
  auto mutatorID = readString();
  auto &mapping = factory.getMutatorsMapping();
  auto mutatorEntry = mapping.find(mutatorID);
  if (mutatorEntry == mapping.end()) {
    return nullptr;
  }
  mull::Mutator *mutator = mutatorEntry->second.get();
  std::string directory = readString();
  std::string filePath = readString();
  std::string unitDirectory = readString();
  std::string unitFilePath = readString();
  int line = readInt();
  int column = readInt();
  mull::SourceLocation loc(unitDirectory, unitFilePath, directory, filePath,
                           line, column);
  auto replacement = readString();
  auto diagnostics = readString();

  return std::make_unique<mull::MutationPoint>(mutator, replacement, loc,
                                               diagnostics);
}
