#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "mull/Version.h"
#include "mull/Bitcode.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Tasks/LoadBitcodeFromBinaryTask.h"
#include "mull/Parallelization/TaskExecutor.h"
#include "mull/Config/Configuration.h"
#include "ebc/EmbeddedFile.h"
#include "ebc/BitcodeRetriever.h"
#include "ebc/BitcodeContainer.h"
#include "MullXCTest/Tasks/ExtractRawEmbeddedFileFromBinary.h"
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
    std::vector<llvm::StringRef> inputBitcodes;
    std::vector<std::unique_ptr<ebc::EmbeddedFile>> embeddedFiles;

    std::vector<const char *> args(&argv[0], &argv[argc]);
    extractBitcodeFiles(args, inputBitcodes);
    validateInputFiles(inputBitcodes);

    mull::Configuration configuration;
    configuration.parallelization = mull::ParallelizationConfig::defaultConfig();

    std::vector<mull_xctest::ExtractEmbeddedFileTask> extractTasks;
    for (int i = 0; i < configuration.parallelization.workers; i++) {
      extractTasks.emplace_back(diagnostics);
    }
    mull::TaskExecutor<mull_xctest::ExtractEmbeddedFileTask> extractExecutor(
        diagnostics, "Extracting embeded files", inputBitcodes, embeddedFiles, std::move(extractTasks));
    extractExecutor.execute();


    std::vector<std::unique_ptr<llvm::LLVMContext>> contexts;
    std::vector<mull::LoadBitcodeFromBinaryTask> tasks;
    for (int i = 0; i < configuration.parallelization.workers; i++) {
      auto context = std::make_unique<llvm::LLVMContext>();
      tasks.emplace_back(mull::LoadBitcodeFromBinaryTask(diagnostics, *context));
      contexts.push_back(std::move(context));
    }
    std::vector<std::unique_ptr<mull::Bitcode>> bitcode;
    mull::TaskExecutor<mull::LoadBitcodeFromBinaryTask> loadExecutor(
        diagnostics, "Loading bitcode files", embeddedFiles, bitcode, std::move(tasks));
    loadExecutor.execute();

    return 0;
}
