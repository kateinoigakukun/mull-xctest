#include "gtest/gtest.h"
#include "MullXCTest/MutantSerialization.h"
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Mutators/CXX/ArithmeticMutators.h>
#include <mull/MutationPoint.h>
#include <mull/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/FileUtilities.h>

using namespace mull_xctest;
using namespace mull;
using namespace llvm;

const char fixture[] = "";

TEST(MutantSerializationTest, singlePoint) {
    Diagnostics diags;
    MutatorsFactory factory(diags);
    factory.init();

    cxx::AddToSub mutator;
    SourceLocation loc("unit_dir", "unit_file_path", "dir", "file_path", 1, 2);
    MutationPoint point(&mutator, "replacement", loc, "diagnostics");
    std::string entriesString;
    raw_string_ostream stream(entriesString);
    MutantSerializer serializer(stream);
    serializer.serialize(&point);

    MutantDeserializer deserializer(StringRef(entriesString), factory);
    auto restored = deserializer.deserialize();

    ASSERT_EQ(point.getMutator()->mutatorKind(), restored->getMutator()->mutatorKind());
    ASSERT_EQ(point.getUserIdentifier(), restored->getUserIdentifier());
}