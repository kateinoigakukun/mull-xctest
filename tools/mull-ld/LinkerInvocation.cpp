#include "LinkerInvocation.h"
#include <llvm/Option/ArgList.h>
#include <llvm/Support/FileUtilities.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Toolchain/Runner.h>
#include <sstream>

using namespace mull_xctest;
using namespace mull;

void mull_xctest::link(std::vector<std::string> objectFiles,
                       LinkerOptions &linkerOpts,
                       const mull::Configuration &config,
                       mull::Diagnostics &diagnostics) {
  Runner runner(diagnostics);
  llvm::opt::ArgStringList rawArgs;
  linkerOpts.collectObjectLinkOpts(rawArgs);

  llvm::SmallString<64> filelistPath;
  llvm::sys::fs::createTemporaryFile("mull-ld-filelist", "", filelistPath);
  llvm::FileRemover cleaner(filelistPath);
  std::error_code ec;
  llvm::raw_fd_ostream filelist(filelistPath, ec);

  for (auto objectFile : objectFiles) {
    filelist << objectFile << "\n";
  }
  filelist.flush();

  std::vector<std::string> arguments;
  for (auto arg : rawArgs) {
    arguments.emplace_back(arg);
  }

  std::string filelistPathStr(filelistPath);
  linkerOpts.appendFilelist(filelistPathStr, arguments);

  ExecutionResult result = runner.runProgram(config.linker, arguments, {},
                                             config.linkerTimeout, true, std::nullopt);
  std::stringstream commandStream;
  commandStream << config.linker;
  for (std::string &argument : arguments) {
    commandStream << ' ' << argument;
  }
  std::string command = commandStream.str();
  if (result.status != Passed) {
    std::stringstream message;
    message << "Cannot link program\n";
    message << "status: " << result.getStatusAsString() << "\n";
    message << "time: " << result.runningTime << "ms\n";
    message << "exit: " << result.exitStatus << "\n";
    message << "command: " << command << "\n";
    message << "stdout: " << result.stdoutOutput << "\n";
    message << "stderr: " << result.stderrOutput << "\n";
    diagnostics.error(message.str());
  }
  diagnostics.debug("Link command: " + command);
}
