#include "LinkerInvocation.h"
#include "MullXCTest/SwiftSupport/SyntaxMutationFilter.h"
#include "MullXCTest/SwiftSupport/SyntaxMutationFinder.h"
#include "MullXCTest/Tasks/EmbedMutantInfoTask.h"
#include "MullXCTest/Tasks/ExtractEmbeddedFileTask.h"
#include "MullXCTest/Tasks/LoadBitcodeFromBufferTask.h"
#include <llvm/Option/ArgList.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Path.h>
#include <mull/Filters/MutationFilter.h>
#include <mull/Mutant.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Filters/FunctionFilter.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Program/Program.h>
#include <mull/Toolchain/Runner.h>
#include <mull/Toolchain/Toolchain.h>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace mull_xctest;
using namespace mull;

static void dumpLLVM(Program &program, std::string destDir) {
  auto &bitcode = program.bitcode();
  for (auto it = bitcode.begin(); it != bitcode.end(); ++it) {
    llvm::Module *module = it->get()->getModule();
    auto filename = llvm::sys::path::filename(module->getSourceFileName());
    llvm::SmallString<64> filepath(destDir);
    llvm::sys::path::append(filepath, filename);
    llvm::sys::path::replace_extension(filepath, "ll");
    std::error_code ec;
    llvm::raw_fd_ostream os(filepath, ec);
    module->print(os, nullptr);
  }
}

void LinkerInvocation::run() {
  const auto workers = config.parallelization.workers;
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;

  // Step 1: Extract bitcode section buffer from object files
  std::vector<mull_xctest::ExtractEmbeddedFileTask> extractTasks;
  for (int i = 0; i < workers; i++) {
    extractTasks.emplace_back(diagnostics);
  }
  TaskExecutor<mull_xctest::ExtractEmbeddedFileTask> extractExecutor(
      diagnostics, "Extracting embeded bitcode", inputObjects, bitcodeBuffers,
      std::move(extractTasks));
  extractExecutor.execute();

  // Step 2: Load LLVM bitcode module from extracted raw buffer
  std::vector<std::unique_ptr<llvm::LLVMContext>> contexts;
  std::vector<mull_xctest::LoadBitcodeFromBufferTask> tasks;
  for (int i = 0; i < workers; i++) {
    auto context = std::make_unique<llvm::LLVMContext>();
    tasks.emplace_back(diagnostics, *context);
    contexts.push_back(std::move(context));
  }
  std::vector<std::unique_ptr<mull::Bitcode>> bitcode;
  mull::TaskExecutor<mull_xctest::LoadBitcodeFromBufferTask> loadExecutor(
      diagnostics, "Loading bitcode files", bitcodeBuffers, bitcode,
      std::move(tasks));
  loadExecutor.execute();

  mull::Program program(std::move(bitcode));

  // Step 3: Find mutation points from LLVM modules
  auto mutationPoints = findMutationPoints(program);

  if (invocationConfig.EnableSyntaxFilter) {
    setupSyntaxFilter(mutationPoints);
  }

  auto filteredMutations = filterMutations(std::move(mutationPoints));
  std::vector<std::unique_ptr<Mutant>> mutants;
  singleTask.execute("Deduplicate mutants", [&]() {
    std::unordered_map<std::string, std::vector<MutationPoint *>> mapping;
    for (MutationPoint *point : filteredMutations) {
      mapping[point->getUserIdentifier()].push_back(point);
    }
    for (auto &pair : mapping) {
      mutants.push_back(std::make_unique<Mutant>(pair.first, pair.second));
    }
    std::sort(std::begin(mutants), std::end(mutants), MutantComparator());
  });

  if (filteredMutations.empty()) {
    link(inputObjects);
    return;
  }

  // Step 4. Apply mutations
  applyMutation(program, filteredMutations);

  if (invocationConfig.DumpLLVM.hasValue()) {
    dumpLLVM(program, invocationConfig.DumpLLVM.getValue());
  }

  // Step 5. Compile LLVM modules to object files
  mull::Toolchain toolchain(diagnostics, config);
  std::vector<OriginalCompilationTask> compilationTasks;
  compilationTasks.reserve(workers);
  for (int i = 0; i < workers; i++) {
    compilationTasks.emplace_back(toolchain);
  }
  std::vector<std::string> objectFiles;
  TaskExecutor<OriginalCompilationTask> mutantCompiler(
      diagnostics, "Compiling original code", program.bitcode(), objectFiles,
      std::move(compilationTasks));
  mutantCompiler.execute();

  // Step 6. Link compiled object files with real linker
  link(objectFiles);
}


std::vector<mull::FunctionUnderTest> LinkerInvocation::getFunctionsUnderTest(Program &program) {
  std::vector<FunctionUnderTest> functionsUnderTest;
  for (auto &bitcode : program.bitcode()) {
    for (llvm::Function &function : *bitcode->getModule()) {
      functionsUnderTest.emplace_back(&function, bitcode.get());
    }
  }
  return functionsUnderTest;
}

std::vector<mull::FunctionUnderTest> LinkerInvocation::filterFunctions(std::vector<mull::FunctionUnderTest> functions) {
    std::vector<FunctionUnderTest> filteredFunctions(std::move(functions));

    for (auto filter : filters.functionFilters) {
      std::vector<FunctionFilterTask> tasks;
      tasks.reserve(config.parallelization.workers);
      for (int i = 0; i < config.parallelization.workers; i++) {
        tasks.emplace_back(*filter);
      }

      std::string label = std::string("Applying function filter: ") + filter->name();
      std::vector<FunctionUnderTest> tmp;
      TaskExecutor<FunctionFilterTask> filterRunner(
          diagnostics, label, filteredFunctions, tmp, std::move(tasks));
      filterRunner.execute();
      filteredFunctions = std::move(tmp);
    }

    return filteredFunctions;
}

std::vector<MutationPoint *>
LinkerInvocation::findMutationPoints(Program &program) {
  std::vector<FunctionUnderTest> functionsUnderTest = getFunctionsUnderTest(program);
  std::vector<FunctionUnderTest> filteredFunctions = filterFunctions(functionsUnderTest);
  selectInstructions(filteredFunctions);
  return std::move(mutationsFinder.getMutationPoints(diagnostics, program,
                                                     filteredFunctions));
}

void LinkerInvocation::selectInstructions(
    std::vector<FunctionUnderTest> &functions) {
  std::vector<InstructionSelectionTask> tasks;
  tasks.reserve(config.parallelization.workers);
  for (int i = 0; i < config.parallelization.workers; i++) {
    tasks.emplace_back(filters.instructionFilters);
  }

  std::vector<int> Nothing;
  TaskExecutor<InstructionSelectionTask> filterRunner(
      diagnostics, "Instruction selection", functions, Nothing,
      std::move(tasks));
  filterRunner.execute();
}

void LinkerInvocation::setupSyntaxFilter(
    std::vector<MutationPoint *> &mutationPoints) {
  std::set<std::string> sourcePaths;
  for (auto it = mutationPoints.begin(); it != mutationPoints.end(); ++it) {
    MutationPoint *point = *it;
    sourcePaths.insert(point->getSourceLocation().filePath);
  }

  using namespace mull_xctest::swift;
  SyntaxMutationFinder finder;

  auto storage = finder.findMutations(sourcePaths, diagnostics, config);

  auto *syntaxFilter =
      new SyntaxMutationFilter(diagnostics, std::move(storage));
  syntaxFilterOwner = std::unique_ptr<mull::MutationFilter>(syntaxFilter);
  filters.mutationFilters.push_back(syntaxFilter);
}

std::vector<MutationPoint *>
LinkerInvocation::filterMutations(std::vector<MutationPoint *> mutationPoints) {
  std::vector<MutationPoint *> mutations = std::move(mutationPoints);

  for (auto filter : filters.mutationFilters) {
    std::vector<MutationFilterTask> tasks;
    tasks.reserve(config.parallelization.workers);
    for (int i = 0; i < config.parallelization.workers; i++) {
      tasks.emplace_back(*filter);
    }

    std::string label = std::string("Applying filter: ") + filter->name();
    std::vector<MutationPoint *> tmp;
    TaskExecutor<MutationFilterTask> filterRunner(diagnostics, label, mutations,
                                                  tmp, std::move(tasks));
    filterRunner.execute();
    mutations = std::move(tmp);
  }

  return mutations;
}

void LinkerInvocation::applyMutation(
    Program &program, std::vector<MutationPoint *> &mutationPoints) {
  singleTask.execute("Prepare mutations", [&]() {
    for (auto point : mutationPoints) {
      point->getBitcode()->addMutation(point);
    }
  });

  auto workers = config.parallelization.workers;

  std::vector<int> Nothing;
  TaskExecutor<EmbedMutantInfoTask> embedMutantInfo(
      diagnostics, "Embedding mutation information", program.bitcode(), Nothing,
      std::vector<EmbedMutantInfoTask>(workers));
  embedMutantInfo.execute();

  TaskExecutor<CloneMutatedFunctionsTask> cloneFunctions(
      diagnostics, "Cloning functions for mutation", program.bitcode(), Nothing,
      std::vector<CloneMutatedFunctionsTask>(workers));
  cloneFunctions.execute();

  TaskExecutor<DeleteOriginalFunctionsTask> deleteOriginalFunctions(
      diagnostics, "Removing original functions", program.bitcode(), Nothing,
      std::vector<DeleteOriginalFunctionsTask>(workers));
  deleteOriginalFunctions.execute();

  TaskExecutor<InsertMutationTrampolinesTask> redirectFunctions(
      diagnostics, "Redirect mutated functions", program.bitcode(), Nothing,
      std::vector<InsertMutationTrampolinesTask>(workers));
  redirectFunctions.execute();

  TaskExecutor<ApplyMutationTask> applyMutations(
      diagnostics, "Applying mutations", mutationPoints, Nothing,
      {ApplyMutationTask()});
  applyMutations.execute();
}

void LinkerInvocation::link(std::vector<std::string> objectFiles) {
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
                                             config.linkerTimeout, true);
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
