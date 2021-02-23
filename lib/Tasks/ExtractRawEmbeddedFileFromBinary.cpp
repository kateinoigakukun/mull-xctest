#include "MullXCTest/Tasks/ExtractRawEmbeddedFileFromBinary.h"
#include <ebc/BitcodeContainer.h>
#include <ebc/BitcodeRetriever.h>

namespace mull_xctest {

void ExtractEmbeddedFileTask::operator()(iterator begin, iterator end, Out &storage,
                                         mull::progress_counter &counter) {
    for (auto it = begin; it != end; it++, counter.increment()) {
        llvm::StringRef inputFile = *it;
        ebc::BitcodeRetriever bitcodeRetriever(inputFile.str());
        for (auto &bitcodeInfo : bitcodeRetriever.GetBitcodeInfo()) {
          auto &container = bitcodeInfo.bitcodeContainer;
          if (container) {
            for (auto &file : container->GetRawEmbeddedFiles()) {
                storage.push_back(std::move(file));
            }
          } else {
            diagnostics.warning(std::string("No bitcode: ") + bitcodeInfo.arch);
          }
        }
    }
}

}
