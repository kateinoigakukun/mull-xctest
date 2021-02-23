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
#include <vector>
#include <string>
#include <unistd.h>

void extractBitcodeFiles(std::vector<const char *> args,
                         std::vector<llvm::StringRef> &bitcodeFiles) {
    for (const auto rawArg : args) {
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

int main(int argc, char **argv) {
    mull::Diagnostics diagnostics;
    std::vector<llvm::StringRef> inputObjects;
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;

    std::vector<const char *> args(&argv[0], &argv[argc]);
    extractBitcodeFiles(args, inputObjects);
    validateInputFiles(inputObjects);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::TargetRegistry::printRegisteredTargetsForVersion(llvm::dbgs());

    mull::Configuration configuration;
    configuration.parallelization = mull::ParallelizationConfig::defaultConfig();
    configuration.parallelization.workers = 1;
    // FIXME: Link input objects with real linker
    configuration.executable = "echo";

    std::vector<mull_xctest::ExtractEmbeddedFileTask> extractTasks;
    for (int i = 0; i < configuration.parallelization.workers; i++) {
      extractTasks.emplace_back(diagnostics);
    }
    mull::TaskExecutor<mull_xctest::ExtractEmbeddedFileTask> extractExecutor(
        diagnostics, "Extracting embeded bitcode", inputObjects, bitcodeBuffers, std::move(extractTasks));
    extractExecutor.execute();


    std::vector<std::unique_ptr<llvm::LLVMContext>> contexts;
    std::vector<mull_xctest::LoadBitcodeFromBufferTask> tasks;
    for (int i = 0; i < configuration.parallelization.workers; i++) {
      auto context = std::make_unique<llvm::LLVMContext>();
      tasks.emplace_back(diagnostics, *context);
      contexts.push_back(std::move(context));
    }
    std::vector<std::unique_ptr<mull::Bitcode>> bitcode;
    mull::TaskExecutor<mull_xctest::LoadBitcodeFromBufferTask> loadExecutor(
        diagnostics, "Loading bitcode files", bitcodeBuffers, bitcode, std::move(tasks));
    loadExecutor.execute();

    mull::Program program(std::move(bitcode));
    mull::Toolchain toolchain(diagnostics, configuration);
    mull::Filters filters;
    mull::MutatorsFactory factory(diagnostics);
    
    mull::MutationsFinder mutationsFinder(factory.mutators({}), configuration);

    mull::Driver driver(diagnostics, configuration, program, toolchain, filters, mutationsFinder);
    auto result = driver.run();
    llvm::llvm_shutdown();
    return 0;
}
