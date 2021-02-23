#include "llvm/Support/CommandLine.h"
#include "mull/Version.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Parallelization/Tasks/LoadBitcodeFromBinaryTask.h"
#include "mull/Parallelization/TaskExecutor.h"
#include "ebc/EmbeddedFile.h"
#include "ebc/BitcodeRetriever.h"
#include "ebc/BitcodeContainer.h"
#include <vector>

int main(int argc, char **argv) {
    mull::Diagnostics diagnostics;
    llvm::cl::SetVersionPrinter(mull::printVersionInformation);
    bool validOptions = llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs());

    std::vector<std::unique_ptr<ebc::EmbeddedFile>> embeddedFiles;

    mull::SingleTaskExecutor extractBitcodeBuffers(diagnostics);
    extractBitcodeBuffers.execute("Extracting bitcode from executable", [&] {
        ebc::BitcodeRetriever bitcodeRetriever("");
        for (auto &bitcodeInfo : bitcodeRetriever.GetBitcodeInfo())
        {
            auto &container = bitcodeInfo.bitcodeContainer;
            if (container)
            {
                for (auto &file : container->GetRawEmbeddedFiles())
                {
                    embeddedFiles.push_back(std::move(file));
                }
            }
            else
            {
                diagnostics.warning(std::string("No bitcode: ") + bitcodeInfo.arch);
            }
        }
    });

    return 0;
}
