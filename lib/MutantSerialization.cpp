#include "MullXCTest/MutantSerialization.h"
#include "MullXCTest/MutantMetadata.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Support/Endian.h>
#include <mull/MutationPoint.h>
#include <mull/Mutators/Mutator.h>
#include "MutantExtractor.h"

#include <sstream>

using namespace mull_xctest;

static const char magic[] = "mull";
static const char version = 1;

void MutantSerializer::writeInt(int value) {
  char bytes[4];
  llvm::support::endian::write32be(bytes, value);
  output.write(bytes, 4);
}
void MutantSerializer::writeString(const std::string &str) {
  output.write(str.c_str(), str.size());
  output.write('\0');
}
void MutantSerializer::serializeMetadata() {
  writeString(magic);
  writeInt(version);
}
void MutantSerializer::serialize(mull::Mutant *mutant) {
  writeString(mutant->getIdentifier());
  writeString(mutant->getMutatorIdentifier());

  const auto &loc = mutant->getSourceLocation();
  writeString(loc.directory);
  writeString(loc.filePath);
  writeString(loc.unitDirectory);
  writeString(loc.unitFilePath);
  writeInt(loc.line);
  writeInt(loc.column);
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

bool MutantDeserializer::consumeMetadata() {
  std::string readMagic = readString();
  int readVersion = readInt();
  return !(readMagic == magic && readVersion == version);
}

std::unique_ptr<mull::Mutant> MutantDeserializer::deserialize() {
  auto identifier = readString();
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

  return std::make_unique<mull::Mutant>(identifier, mutatorID, loc, true);
}

llvm::Expected<MutantList> mull_xctest::ExtractMutantInfo(
    std::string binaryPath, mull::MutatorsFactory &factory,
    mull::Diagnostics &diags) {
  mull::MutantExtractor extractor(diags);
  return std::move(extractor.extractMutants(binaryPath));
}
