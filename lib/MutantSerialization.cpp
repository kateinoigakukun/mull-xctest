#include "MullXCTest/MutantSerialization.h"
#include "MullXCTest/MutantMetadata.h"
#include <llvm/Support/Endian.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <mull/MutationPoint.h>
#include <mull/Mutators/Mutator.h>

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

bool MutantDeserializer::consumeMetadata() {
  std::string readMagic = readString();
  int readVersion = readInt();
  return !(readMagic == magic && readVersion == version);
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

llvm::Expected<MutantList> mull_xctest::ExtractMutantInfo(
    std::string binaryPath,
    mull::MutatorsFactory &factory,
    std::vector<std::unique_ptr<mull::Mutator>> &mutators,
    std::vector<std::unique_ptr<mull::MutationPoint>> &pointsOwner) {

  using namespace llvm;
  auto binaryOrErr = object::createBinary(binaryPath);
  if (!binaryOrErr) {
    return binaryOrErr.takeError();
  }
  const auto *machOObjectFile =
      dyn_cast<object::MachOObjectFile>(binaryOrErr->getBinary());
  if (!machOObjectFile) {
    return llvm::make_error<StringError>(Twine("input file is not mach-o object file"), llvm::inconvertibleErrorCode());
  }
  Expected<object::SectionRef> section =
      machOObjectFile->getSection(MULL_MUTANTS_INFO_SECTION_NAME_STR);
  if (!section) {
    return section.takeError();
  }

  Expected<StringRef> contentsData = section->getContents();
  if (!contentsData) {
    return contentsData.takeError();
  }
  MutantDeserializer deserializer(contentsData.get(), factory);

  std::vector<std::unique_ptr<mull::Mutant>> mutants;
  while (!deserializer.isEOF()) {
    std::unique_ptr<mull::MutationPoint> point = deserializer.deserialize();
    if (!point) {
      return llvm::make_error<StringError>(Twine("failed to deserialize mutants."), llvm::inconvertibleErrorCode());
    }

    std::string id = point->getUserIdentifier();
    pointsOwner.push_back(std::move(point));
    std::vector<mull::MutationPoint *> points;
    points.push_back(pointsOwner.back().get());
    mutants.push_back(std::make_unique<mull::Mutant>(id, points));
  }
  return std::move(mutants);
}
