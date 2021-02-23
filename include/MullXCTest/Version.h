#ifndef MULL_XCTEST_VERSION
#define MULL_XCTEST_VERSION

namespace llvm
{
    class raw_ostream;
}

namespace mull_xctest
{

    const char *MullXCTestVersionString();
    const char *MullXCTestCommitString();
    const char *MullXCTestBuildDateString();

    const char *llvmVersionString();

    void printVersionInformation(llvm::raw_ostream &out);

} // namespace mull

#endif
