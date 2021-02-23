#include "llvm/Support/CommandLine.h"
#include "MullXCTest/Version.h"

int main(int argc, char **argv) {
    llvm::cl::SetVersionPrinter(mull_xctest::printVersionInformation);
    bool validOptions = llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());
    return 0;
}
