#include "MullXCTest/Tasks/ExtractEmbeddedFileTask.h"
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/MachOUniversal.h>
#include <sstream>

using namespace llvm;

namespace mull_xctest {

static ArrayRef<unsigned char>
getBitcodeSectionContent(const object::MachOObjectFile *object) {
  for (auto it = object->section_begin(); it != object->section_end(); ++it) {
    auto section = *it;
    auto sectionName = section.getName();
    if (!sectionName) {
      consumeError(sectionName.takeError());
      continue;
    }
    auto segmentName =
        object->getSectionFinalSegmentName(it->getRawDataRefImpl());

    if (!(segmentName.equals("__LLVM") && sectionName->equals("__bitcode"))) {
      continue;
    }
    auto content = object->getSectionContents(it->getRawDataRefImpl());
    if (!content) {
      return {};
    }
    return content.get();
  }
  return {};
}

std::unique_ptr<llvm::MemoryBuffer>
extractEmbeddedFile(llvm::StringRef inputFile,
                    mull::Diagnostics &diagnostics) {
  auto binaryOrErr = object::createBinary(inputFile);
  if (!binaryOrErr) {
    std::stringstream errorMessage;
    errorMessage << "createBinary failed: \""
                 << toString(binaryOrErr.takeError()) << "\".";
    diagnostics.warning(errorMessage.str());
    return nullptr;
  }
  const auto *machOObjectFile =
      dyn_cast<object::MachOObjectFile>(binaryOrErr->getBinary());
  if (!machOObjectFile) {
    std::stringstream errorMessage;
    errorMessage << "input file is not mach-o object file: "
                 << inputFile.str();
    diagnostics.warning(errorMessage.str());
    return nullptr;
  }

  Expected<ArrayRef<unsigned char>> contentOrErr =
      getBitcodeSectionContent(machOObjectFile);
  if (!contentOrErr) {
    std::stringstream errorMessage;
    errorMessage << "getBitcodeSectionContent failed: \""
                 << toString(contentOrErr.takeError()) << "\".";
    diagnostics.warning(errorMessage.str());
    return nullptr;
  }
  StringRef contentData(reinterpret_cast<const char *>(contentOrErr->data()),
                        contentOrErr->size());
  return MemoryBuffer::getMemBufferCopy(contentData);
}

void ExtractEmbeddedFileTask::operator()(iterator begin, iterator end,
                                         Out &storage,
                                         mull::progress_counter &counter) {
  for (auto it = begin; it != end; it++, counter.increment()) {
    llvm::StringRef inputFile = *it;
    auto binaryOrErr = object::createBinary(inputFile);
    if (!binaryOrErr) {
      std::stringstream errorMessage;
      errorMessage << "createBinary failed: \""
                   << toString(binaryOrErr.takeError()) << "\".";
      diagnostics.warning(errorMessage.str());
      continue;
    }
    const auto *machOObjectFile =
        dyn_cast<object::MachOObjectFile>(binaryOrErr->getBinary());
    if (!machOObjectFile) {
      std::stringstream errorMessage;
      errorMessage << "input file is not mach-o object file: "
                   << inputFile.str();
      diagnostics.warning(errorMessage.str());
      continue;
    }

    Expected<ArrayRef<unsigned char>> contentOrErr =
        getBitcodeSectionContent(machOObjectFile);
    if (!contentOrErr) {
      std::stringstream errorMessage;
      errorMessage << "getBitcodeSectionContent failed: \""
                   << toString(contentOrErr.takeError()) << "\".";
      diagnostics.warning(errorMessage.str());
      continue;
    }
    StringRef contentData(reinterpret_cast<const char *>(contentOrErr->data()),
                          contentOrErr->size());
    storage.push_back(MemoryBuffer::getMemBufferCopy(contentData));
  }
}

} // namespace mull_xctest
