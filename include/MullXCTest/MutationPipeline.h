#ifndef MULL_XCTEST_MUTATION_PIPELINE_H
#define MULL_XCTEST_MUTATION_PIPELINE_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Option/OptTable.h>
#include <mull/Config/Configuration.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Filters/Filters.h>
#include <mull/AST/ASTMutationFilter.h>
#include <mull/FunctionUnderTest.h>
#include <mull/MutationPoint.h>
#include <mull/MutationsFinder.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/Program/Program.h>
#include <vector>

namespace mull_xctest {
struct MutationPipelineConfig {
  llvm::Optional<std::string> DumpLLVM;
  bool EnableSyntaxFilter;
};

class MutationPipeline {
  std::vector<std::string> inputObjects;
  const std::vector<llvm::StringRef> &targetExecutables;
  mull::Diagnostics &diagnostics;
  const mull::Configuration &config;
  const MutationPipelineConfig &pipelineConfig;
  mull::SingleTaskExecutor singleTask;
  struct mull::Filters &filters;
  mull::MutationsFinder &mutationsFinder;
  std::unique_ptr<mull::MutationFilter> syntaxFilterOwner;
  mull::ASTMutationStorage astMutationStorage;

public:
  MutationPipeline(std::vector<std::string> inputObjects,
                   const std::vector<llvm::StringRef> &targetExecutables,
                   struct mull::Filters &filters,
                   mull::MutationsFinder &mutationsFinder,
                   mull::Diagnostics &diagnostics,
                   const mull::Configuration &config,
                   const MutationPipelineConfig &pipelineConfig)
      : inputObjects(inputObjects), targetExecutables(targetExecutables),
        filters(filters), mutationsFinder(mutationsFinder),
        diagnostics(diagnostics), config(config), singleTask(diagnostics),
        pipelineConfig(pipelineConfig),
        astMutationStorage(diagnostics) {}
  /// @return list of compiled object files
  std::vector<std::string> run();

private:
  std::vector<mull::FunctionUnderTest>
  getFunctionsUnderTest(mull::Program &program);
  std::vector<mull::FunctionUnderTest>
  filterFunctions(std::vector<mull::FunctionUnderTest> functions);
  std::vector<mull::MutationPoint *> findMutationPoints(mull::Program &program);
  void setupSyntaxFilter(std::vector<mull::MutationPoint *> &mutationPoints);
  std::vector<mull::MutationPoint *>
  filterMutations(std::vector<mull::MutationPoint *> mutationPoints);
  void selectInstructions(std::vector<mull::FunctionUnderTest> &functions);
  void applyMutation(mull::Program &program,
                     std::vector<mull::MutationPoint *> &mutationPoints);
};
} // namespace mull_xctest

#endif
