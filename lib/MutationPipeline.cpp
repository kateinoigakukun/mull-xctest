#include "MullXCTest/MutationPipeline.h"
#include "MullXCTest/SwiftSupport/RuntimeFunctionFilter.h"
#include "MullXCTest/SwiftSupport/SyntaxMutationFinder.h"
#include "MullXCTest/Tasks/ExtractEmbeddedFileTask.h"
#include "MullXCTest/Tasks/LoadBitcodeFromBufferTask.h"
#include <llvm/Option/ArgList.h>
#include <llvm/ProfileData/Coverage/CoverageMapping.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Path.h>
#include <mull/Filters/FunctionFilter.h>
#include <mull/Filters/MutationFilter.h>
#include <mull/Mutant.h>
#include <mull/Mutators/MutatorsFactory.h>
#include <mull/Parallelization/Parallelization.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Program/Program.h>
#include <mull/Toolchain/Runner.h>
#include <mull/Toolchain/Toolchain.h>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

std::vector<std::string> MutationPipeline::run() {
  const auto workers = config.parallelization.workers;
  std::vector<std::string> outputObjectFiles;
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bitcodeBuffers;
  std::vector<std::unique_ptr<mull::Bitcode>> bitcode;

  // Step 1: Extract bitcode section buffer from object files
  for (std::string inputFile : inputObjects) {
    std::unique_ptr<llvm::MemoryBuffer> buffer = extractEmbeddedFile(inputFile, diagnostics);
    if (!buffer) {
      // if failed to extract, pass it to linker directly
      outputObjectFiles.push_back(inputFile);
      continue;
    }
    std::unique_ptr<mull::Bitcode> bc = loadBitcodeFromBuffer(std::move(buffer), diagnostics);
    if (!bc) {
      // if failed to load, pass it to linker directly
      outputObjectFiles.push_back(inputFile);
      continue;
    }
    bitcode.push_back(std::move(bc));
  }

  mull::Program program(std::move(bitcode));

  // Step 3: Find mutation points from LLVM modules
  auto mutationPoints = findMutationPoints(program);

  setupSwiftFilter();
  if (pipelineConfig.EnableSyntaxFilter) {
    setupSyntaxFilter(mutationPoints);
  }

  auto filteredMutations = filterMutations(std::move(mutationPoints));
  if (filteredMutations.empty()) {
    return inputObjects;
  }

  std::vector<std::unique_ptr<Mutant>> mutants;
  singleTask.execute("Deduplicate mutants", [&]() {
    std::unordered_map<std::string, std::vector<MutationPoint *>> mapping;
    for (MutationPoint *point : filteredMutations) {
      mapping[point->getUserIdentifier()].push_back(point);
    }
    for (auto &pair : mapping) {
      std::string identifier = pair.first;
      MutationPoint *anyPoint = pair.second.front();
      std::string mutatorIdentifier = anyPoint->getMutatorIdentifier();
      const SourceLocation &sourceLocation = anyPoint->getSourceLocation();
      bool covered = false;
      for (MutationPoint *point : pair.second) {
        if (point->isCovered()) {
          covered = true;
          break;
        }
      }
      mutants.push_back(std::make_unique<Mutant>(identifier, mutatorIdentifier,
                                                 sourceLocation, covered));
    }
    std::sort(std::begin(mutants), std::end(mutants), MutantComparator());
  });

  // Step 4. Apply mutations
  applyMutation(program, filteredMutations);

  if (pipelineConfig.DumpLLVM.hasValue()) {
    dumpLLVM(program, pipelineConfig.DumpLLVM.getValue());
  }

  // Step 5. Compile LLVM modules to object files
  mull::Toolchain toolchain(diagnostics, config);
  std::vector<OriginalCompilationTask> compilationTasks;
  compilationTasks.reserve(workers);
  for (int i = 0; i < workers; i++) {
    compilationTasks.emplace_back(toolchain);
  }

  TaskExecutor<OriginalCompilationTask> mutantCompiler(
      diagnostics, "Compiling original code", program.bitcode(), outputObjectFiles,
      std::move(compilationTasks));
  mutantCompiler.execute();
  return outputObjectFiles;
}

static std::unique_ptr<llvm::coverage::CoverageMapping>
loadCoverage(const Configuration &configuration,
             const std::vector<llvm::StringRef> &targetExecutables,
             Diagnostics &diagnostics) {
  if (configuration.coverageInfo.empty()) {
    return nullptr;
  }

  llvm::Expected<std::unique_ptr<llvm::coverage::CoverageMapping>>
      maybeMapping = llvm::coverage::CoverageMapping::load(
          targetExecutables, configuration.coverageInfo);
  if (!maybeMapping) {
    std::string error;
    llvm::raw_string_ostream os(error);
    llvm::logAllUnhandledErrors(maybeMapping.takeError(), os,
                                "Cannot read coverage info: ");
    diagnostics.warning(os.str());
    return nullptr;
  }
  return std::move(maybeMapping.get());
}

std::vector<mull::FunctionUnderTest>
MutationPipeline::getFunctionsUnderTest(Program &program) {
  std::vector<FunctionUnderTest> functionsUnderTest;
  std::unique_ptr<llvm::coverage::CoverageMapping> coverage =
      loadCoverage(config, targetExecutables, diagnostics);

  if (coverage) {
    std::unordered_map<std::string, std::unordered_set<std::string>>
        scopedFunctions;
    std::unordered_set<std::string> unscopedFunctions;
    for (auto &it : coverage->getCoveredFunctions()) {
      if (!it.ExecutionCount) {
        continue;
      }
      std::string scope;
      std::string name = it.Name;
      size_t idx = name.find(':');
      if (idx != std::string::npos) {
        scope = name.substr(0, idx);
        name = name.substr(idx + 1);
      }
      if (scope.empty()) {
        unscopedFunctions.insert(name);
      } else {
        scopedFunctions[scope].insert(name);
      }
    }
    for (auto &bitcode : program.bitcode()) {
      for (llvm::Function &function : *bitcode->getModule()) {
        bool covered = false;
        std::string name = function.getName().str();
        if (unscopedFunctions.count(name)) {
          covered = true;
        } else {
          std::string filepath =
              SourceLocation::locationFromFunction(&function).unitFilePath;
          std::string scope = llvm::sys::path::filename(filepath).str();
          if (scopedFunctions[scope].count(name)) {
            covered = true;
          }
        }
        if (covered) {
          functionsUnderTest.emplace_back(&function, bitcode.get());
        } else if (config.includeNotCovered) {
          functionsUnderTest.emplace_back(&function, bitcode.get(), false);
        }
      }
    }
  } else {
    for (auto &bitcode : program.bitcode()) {
      for (llvm::Function &function : *bitcode->getModule()) {
        functionsUnderTest.emplace_back(&function, bitcode.get());
      }
    }
  }
  return functionsUnderTest;
}

std::vector<mull::FunctionUnderTest> MutationPipeline::filterFunctions(
    std::vector<mull::FunctionUnderTest> functions) {
  std::vector<FunctionUnderTest> filteredFunctions(std::move(functions));

  for (auto filter : filters.functionFilters) {
    std::vector<FunctionFilterTask> tasks;
    tasks.reserve(config.parallelization.workers);
    for (int i = 0; i < config.parallelization.workers; i++) {
      tasks.emplace_back(*filter);
    }

    std::string label =
        std::string("Applying function filter: ") + filter->name();
    std::vector<FunctionUnderTest> tmp;
    TaskExecutor<FunctionFilterTask> filterRunner(
        diagnostics, label, filteredFunctions, tmp, std::move(tasks));
    filterRunner.execute();
    filteredFunctions = std::move(tmp);
  }

  return filteredFunctions;
}

std::vector<MutationPoint *>
MutationPipeline::findMutationPoints(Program &program) {
  std::vector<FunctionUnderTest> functionsUnderTest;
  singleTask.execute("Gathering functions under test", [&]() {
    functionsUnderTest = getFunctionsUnderTest(program);
  });
  std::vector<FunctionUnderTest> filteredFunctions =
      filterFunctions(functionsUnderTest);
  selectInstructions(filteredFunctions);
  return mutationsFinder.getMutationPoints(diagnostics, program,
                                           filteredFunctions);
}

void MutationPipeline::selectInstructions(
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

void MutationPipeline::setupSwiftFilter() {
  auto *runtimeFnFilter = new swift::RuntimeFunctionFilter(diagnostics);
  filterOwner.emplace_back(runtimeFnFilter);
  filters.mutationFilters.push_back(runtimeFnFilter);
}

void MutationPipeline::setupSyntaxFilter(
    std::vector<MutationPoint *> &mutationPoints) {
  std::set<std::string> sourcePaths;
  for (auto it = mutationPoints.begin(); it != mutationPoints.end(); ++it) {
    MutationPoint *point = *it;
    sourcePaths.insert(point->getSourceLocation().filePath);
  }

  using namespace mull_xctest::swift;
  SyntaxMutationFinder finder;

  finder.findMutations(sourcePaths, astMutationStorage, diagnostics, config);

  auto *astFilter =
      new mull::ASTMutationFilter(diagnostics, astMutationStorage);

  filterOwner.emplace_back(astFilter);
  filters.mutationFilters.push_back(astFilter);
}

std::vector<MutationPoint *>
MutationPipeline::filterMutations(std::vector<MutationPoint *> mutationPoints) {
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

void MutationPipeline::applyMutation(
    Program &program, std::vector<MutationPoint *> &mutationPoints) {
  singleTask.execute("Prepare mutations", [&]() {
    for (auto point : mutationPoints) {
      point->getBitcode()->addMutation(point);
    }
  });

  auto workers = config.parallelization.workers;

  std::vector<int> Nothing;

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
