#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "mull/Version.h"
#include "mull/Bitcode.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Tasks/LoadBitcodeFromBinaryTask.h"
#include "mull/Parallelization/TaskExecutor.h"
#include "mull/Config/Configuration.h"
#include "mull/Program/Program.h"
#include "mull/Toolchain/Toolchain.h"
#include "mull/Filters/Filters.h"
#include "mull/Driver.h"
#include "mull/Mutators/MutatorsFactory.h"
#include "mull/MutationsFinder.h"
#include "mull/Result.h"
#include "ebc/EmbeddedFile.h"
#include "ebc/BitcodeRetriever.h"
#include "ebc/BitcodeContainer.h"
#include "MullXCTest/Tasks/ExtractEmbeddedFileTask.h"
#include "MullXCTest/Tasks/LoadBitcodeFromBufferTask.h"
#include "LinkerInvocation.h"
#include <vector>
#include <string>
#include <unistd.h>

void extractBitcodeFiles(std::vector<std::string> &args,
                         std::vector<llvm::StringRef> &bitcodeFiles) {
    for (const auto &rawArg : args) {
        llvm::StringRef arg(rawArg);
        if (arg.endswith(".o")) {
            bitcodeFiles.push_back(arg);
        }
    }
}

static void validateInputFiles(const std::vector<llvm::StringRef> &inputFiles) {
    for (const auto inputFile : inputFiles) {
        if (access(inputFile.str().c_str(), R_OK) != 0) {
            perror(inputFile.str().c_str());
            exit(1);
        }
    }
}

static void validateConfiguration(const mull::Configuration &configuration, mull::Diagnostics &diags) {
    if (configuration.linker.empty()) {
        diags.error("No linker specified. Please set MULL_XCTEST_LINKER environment variable.");
    }
}

int main(int argc, char **argv) {
    mull::Diagnostics diagnostics;
    std::vector<llvm::StringRef> inputObjects;
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;

    std::vector<std::string> args(argv + 1, argv + argc);
    extractBitcodeFiles(args, inputObjects);
    validateInputFiles(inputObjects);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    mull::Filters filters;
    mull::Configuration configuration;
    // FIXME: Link input objects with real linker
    configuration.executable = "echo";
    configuration.linker = getenv("MULL_XCTEST_LINKER");
    configuration.debugEnabled = true;
    configuration.linkerTimeout = mull::MullDefaultLinkerTimeoutMilliseconds;
    configuration.timeout = mull::MullDefaultTimeoutMilliseconds;
    diagnostics.enableDebugMode();

    validateConfiguration(configuration, diagnostics);
    mull::MutatorsFactory factory(diagnostics);
    mull::MutationsFinder mutationsFinder(factory.mutators({"cxx_comparison"}), configuration);

    mull_xctest::LinkerInvocation invocation(inputObjects, filters, mutationsFinder,
                                             args, diagnostics, configuration);
    invocation.run();
    llvm::llvm_shutdown();
    return 0;
}
