#include "MullXCTest/MutantSerialization.h"
#include "gtest/gtest.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Program.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/MutationPoint.h>
#include <mull/Mutators/CXX/ArithmeticMutators.h>
#include <mull/SourceLocation.h>

using namespace mull_xctest;
using namespace mull;
using namespace llvm;

TEST(MutantSerializationTest, metadata) {
  Diagnostics diags;
  MutatorsFactory factory(diags);

  std::string entriesString;
  raw_string_ostream stream(entriesString);
  MutantSerializer serializer(stream);
  serializer.serializeMetadata();

  MutantDeserializer deserializer(StringRef(entriesString), factory);
  ASSERT_FALSE(deserializer.consumeMetadata());
}

TEST(MutantSerializationTest, singlePoint) {
  Diagnostics diags;
  MutatorsFactory factory(diags);
  factory.init();

  cxx::AddToSub mutator;
  SourceLocation loc("unit_dir", "unit_file_path", "dir", "file_path", 1, 2);
  std::string userIdentifier = mutator.getUniqueIdentifier() + ':' + loc.filePath + ':' +
                               std::to_string(loc.line) + ':' + std::to_string(loc.column);
  Mutant mutant(userIdentifier, mutator.getUniqueIdentifier(), loc, true);
  std::string entriesString;
  raw_string_ostream stream(entriesString);
  MutantSerializer serializer(stream);
  serializer.serialize(&mutant);

  MutantDeserializer deserializer(StringRef(entriesString), factory);
  auto restored = deserializer.deserialize();

  ASSERT_EQ(mutant.getMutatorIdentifier(), restored->getMutatorIdentifier());
  ASSERT_EQ(mutant.getIdentifier(), restored->getIdentifier());
}
