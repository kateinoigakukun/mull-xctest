#include "MullXCTest/XCResultFile.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Program.h>

using namespace mull_xctest;

namespace {

class Value {
protected:
  const llvm::json::Object &object;
  void assertType() const {
    const auto *typeObj = object.getObject("_type");
    const auto typeName = typeObj->getString("_name");
    assert(typeName.getValue().equals(this->typeIdentifier()));
  }

public:
  virtual llvm::StringRef typeIdentifier() const = 0;
  Value(const llvm::json::Object &object) : object(object) {}
};

class String : public Value {
public:
  String(const llvm::json::Object &object) : Value(object) { assertType(); }
  virtual llvm::StringRef typeIdentifier() const override { return "String"; }
  explicit operator llvm::StringRef() const {
    return object.getString("_value").getValue();
  }
};

template <class T> class Array : public Value {
public:
  Array(const llvm::json::Object &object) : Value(object) { assertType(); }
  virtual llvm::StringRef typeIdentifier() const override { return "Array"; }

  const T operator[](size_t I) const {
    const llvm::json::Object *result =
        object.getArray("_values")->operator[](I).getAsObject();
    return T(*result);
  }
  size_t size() const { return object.getArray("_values")->size(); }
};

class TestFailureIssueSummary : public Value {
public:
  TestFailureIssueSummary(const llvm::json::Object &object) : Value(object) {
    assertType();
  }
  virtual llvm::StringRef typeIdentifier() const override {
    return "TestFailureIssueSummary";
  }

  llvm::Optional<llvm::StringRef> producingTarget() const {
    auto found = object.getObject("producingTarget");
    if (!found) {
      return llvm::None;
    }
    return llvm::StringRef(String(*found));
  }
};

class ResultIssueSummaries : public Value {
public:
  ResultIssueSummaries(const llvm::json::Object &object) : Value(object) {
    assertType();
  }
  virtual llvm::StringRef typeIdentifier() const override {
    return "ResultIssueSummaries";
  }

  llvm::Optional<Array<TestFailureIssueSummary>> testFailureSummaries() {
    auto found = object.getObject("testFailureSummaries");
    if (!found) {
      return llvm::None;
    }
    return Array<TestFailureIssueSummary>(*found);
  }
};

class ActionsInvocationRecord : public Value {
public:
  ActionsInvocationRecord(const llvm::json::Object &object) : Value(object) {
    assertType();
  }
  virtual llvm::StringRef typeIdentifier() const override {
    return "ActionsInvocationRecord";
  }
  ResultIssueSummaries issues() const {
    return ResultIssueSummaries(*object.getObject("issues"));
  }
};

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
} // namespace

void dumpString(llvm::Optional<std::string> content) {
  llvm::dbgs() << content << "\n";
}

llvm::Expected<std::set<std::string>> XCResultFile::getFailureTestTargets() {
  auto jsonOutput =
      execCommand("/usr/bin/xcrun", {"/usr/bin/xcrun", "xcresulttool", "get",
                                     "--path", resultFile, "--format", "json"});
  if (!jsonOutput) {
    return llvm::make_error<llvm::StringError>("failed to execute xcresulttool",
                                               llvm::inconvertibleErrorCode());
  }
  llvm::Expected<llvm::json::Value> json =
      llvm::json::parse(jsonOutput.getValue());
  if (!json) {
    return json.takeError();
  }
  std::string jsonString = jsonOutput.getValue();
  std::set<std::string> results;
  ActionsInvocationRecord record(*json->getAsObject());
  const auto summaries = record.issues().testFailureSummaries();
  if (!summaries) {
    return std::set<std::string>{};
  }
  for (size_t it = 0, end = summaries->size(); it != end; it++) {
    TestFailureIssueSummary summary = summaries->operator[](it);
    llvm::Optional<llvm::StringRef> target = summary.producingTarget();
    if (target) {
      results.insert(target->str());
    }
  }
  return results;
}
