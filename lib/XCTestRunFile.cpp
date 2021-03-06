#include "MullXCTest/XCTestRunFile.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>

using namespace mull_xctest;

static llvm::Optional<std::string>
execCommand(llvm::StringRef exec, llvm::ArrayRef<llvm::StringRef> Argv) {
  llvm::SmallString<64> OutFile;
  llvm::sys::fs::createTemporaryFile("mull-xctestrun-pb", "", OutFile);
  llvm::FileRemover OutRemover(OutFile);
  llvm::Optional<llvm::StringRef> Redirects[3] = {
      /*stdin=*/{""}, /*stdout=*/{OutFile}, /*stderr=*/{""}};

  int Ret = llvm::sys::ExecuteAndWait(exec, Argv,
                                      /*Env=*/llvm::None, Redirects,
                                      /*SecondsToWait=*/10);
  if (Ret != 0) {
    return llvm::None;
  }

  auto Buf = llvm::MemoryBuffer::getFile(OutFile);
  if (!Buf) {

    return llvm::None;
  }
  llvm::StringRef Path = Buf->get()->getBuffer().trim();
  if (Path.empty()) {
    return llvm::None;
  }
  return Path.str();
}

static void escapeColon(std::string &value) {
  std::string colon = ":";
  std::string replacement = "\\:";
  std::string::size_type pos = 0;
  while ((pos = value.find(colon, pos)) != std::string::npos) {
    value.replace(pos, colon.length(), replacement);
    pos += replacement.length();
  }
}

bool XCTestRunFile::addEnvironmentVariable(std::string targetName,
                                           const std::string &key,
                                           const std::string &value) {
  escapeColon(targetName);
  std::string escapedKey = key;
  escapeColon(escapedKey);
  std::string command = "Add " + targetName +
                        ":TestingEnvironmentVariables:" + escapedKey +
                        " string 1";
  int Ret = llvm::sys::ExecuteAndWait(
      "/usr/libexec/PlistBuddy",
      {"/usr/libexec/PlistBuddy", "-x", "-c", command, filePath},
      /*Env=*/llvm::None, llvm::None,
      /*SecondsToWait=*/10);
  if (Ret != 0) {
    return true;
  }
  return false;
}

bool XCTestRunFile::setBlueprintName(std::string targetName, const std::string &name) {
  escapeColon(targetName);
  std::string command = "Set " + targetName +
                        ":BlueprintName " + name;
  int Ret = llvm::sys::ExecuteAndWait(
      "/usr/libexec/PlistBuddy",
      {"/usr/libexec/PlistBuddy", "-x", "-c", command, filePath},
      /*Env=*/llvm::None, llvm::None,
      /*SecondsToWait=*/10);
  if (Ret != 0) {
    return true;
  }
  return false;
}

llvm::Expected<std::vector<std::string>>
XCTestRunFile::getDependentProductPaths(std::string targetName) {
  std::string keyPath = targetName + ".DependentProductPaths";
  llvm::SmallString<64> outFile;
  llvm::sys::fs::createTemporaryFile("mull-xctestrun-pb", "", outFile);
  llvm::FileRemover outRemover(outFile);
  llvm::ArrayRef<llvm::StringRef> args = {
      "/usr/bin/plutil", "-extract", keyPath, "json", filePath, "-o", outFile};
  std::string ErrMsg;
  int Ret = llvm::sys::ExecuteAndWait("/usr/bin/plutil", args,
                                      /*Env=*/llvm::None, {},
                                      /*SecondsToWait=*/0,
                                      0, &ErrMsg);
  if (Ret != 0) {
    llvm::dbgs() << "ErrMsg: " << ErrMsg << "\n";
    return llvm::make_error<llvm::StringError>(
        "failed to extract product paths from xctestrun file.",
        llvm::inconvertibleErrorCode());
  }
  auto fileBuf = llvm::MemoryBuffer::getFile(outFile);
  if (!fileBuf) {
    return llvm::make_error<llvm::StringError>("failed open plutil result file",
                                               fileBuf.getError());
  }

  llvm::Expected<llvm::json::Value> result =
      llvm::json::parse(fileBuf.get()->getBuffer());
  if (!result) {
    return result.takeError();
  }

  auto *productPaths = result->getAsArray();
  assert(productPaths);

  std::vector<std::string> results;
  llvm::SmallString<128> testRoot(filePath);
  llvm::sys::path::remove_filename(testRoot);
  std::map<std::string, std::string> metaSymMap;
  metaSymMap.emplace("__TESTROOT__", testRoot);

  for (auto path : *productPaths) {
    auto maybeValue = path.getAsString();
    if (!maybeValue) {
      continue;
    }
    std::string stringVal = maybeValue->str();
    for (auto target : metaSymMap) {
      const std::string &key = target.first;
      const std::string &replacement = target.second;
      std::string::size_type pos = 0;
      while ((pos = stringVal.find(key, pos)) != std::string::npos) {
        stringVal.replace(pos, key.length(), replacement);
        pos += replacement.length();
      }
    }
    results.push_back(stringVal);
  }
  return std::move(results);
}

llvm::Expected<std::vector<std::string>>
XCTestRunFile::getTargets() {
  llvm::SmallString<64> outFile;
  llvm::sys::fs::createTemporaryFile("mull-xctestrun-tgt", "", outFile);
  llvm::FileRemover outRemover(outFile);
  llvm::ArrayRef<llvm::StringRef> args = {
      "/usr/bin/plutil", "-convert", "json", filePath, "-o", outFile};
  std::string ErrMsg;
  int Ret = llvm::sys::ExecuteAndWait("/usr/bin/plutil", args,
                                      /*Env=*/llvm::None, {},
                                      /*SecondsToWait=*/0,
                                      0, &ErrMsg);
  if (Ret != 0) {
    llvm::dbgs() << "ErrMsg: " << ErrMsg << "\n";
    return llvm::make_error<llvm::StringError>(
        "failed to convert xctestrun file to json.",
        llvm::inconvertibleErrorCode());
  }
  auto fileBuf = llvm::MemoryBuffer::getFile(outFile);
  if (!fileBuf) {
    return llvm::make_error<llvm::StringError>("failed open plutil result file",
                                               fileBuf.getError());
  }

  llvm::Expected<llvm::json::Value> result =
      llvm::json::parse(fileBuf.get()->getBuffer());
  if (!result) {
    return result.takeError();
  }

  auto *productPaths = result->getAsObject();
  assert(productPaths);

  std::vector<std::string> results;

  for (auto pair : *productPaths) {
    std::string targetName = pair.first.str();
    if (targetName == "__xctestrun_metadata__") {
      continue;
    }
    results.push_back(targetName);
  }
  return std::move(results);
}

bool XCTestRunFile::duplicateTestTarget(std::string srcTargetName, std::string newTargetName) {
  escapeColon(srcTargetName);
  escapeColon(newTargetName);
  std::string command = "Copy " + srcTargetName + " " + newTargetName;
  int Ret = llvm::sys::ExecuteAndWait(
      "/usr/libexec/PlistBuddy",
      {"/usr/libexec/PlistBuddy", "-x", "-c", command, filePath},
      /*Env=*/llvm::None, llvm::None,
      /*SecondsToWait=*/10);
  if (Ret != 0) {
    return true;
  }
  return false;
}


bool XCTestRunFile::deleteTestTarget(std::string targetName) {
  escapeColon(targetName);
  std::string command = "Delete " + targetName;
  int Ret = llvm::sys::ExecuteAndWait(
      "/usr/libexec/PlistBuddy",
      {"/usr/libexec/PlistBuddy", "-x", "-c", command, filePath},
      /*Env=*/llvm::None, llvm::None,
      /*SecondsToWait=*/10);
  if (Ret != 0) {
    return true;
  }
  return false;
}
