//
//  LInkerInvocation.hpp
//  mull-ld
//
//  Created by kateinoigakukun on 2021/02/24.
//

#ifndef LInkerInvocation_hpp
#define LInkerInvocation_hpp

#include <mull/MutationPoint.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Config/Configuration.h>
#include <mull/Program/Program.h>
#include <mull/Parallelization/TaskExecutor.h>
#include <mull/FunctionUnderTest.h>
#include <mull/Filters/Filters.h>
#include <mull/MutationsFinder.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace mull_xctest {
class LinkerInvocation {
    std::vector<llvm::StringRef> inputObjects;
    mull::Diagnostics &diagnostics;
    const mull::Configuration &config;
    std::vector<std::string> originalArgs;
    mull::SingleTaskExecutor singleTask;
    struct mull::Filters &filters;
    mull::MutationsFinder &mutationsFinder;
    
public:
    LinkerInvocation(std::vector<llvm::StringRef> inputObjects,
                     struct mull::Filters &filters,
                     mull::MutationsFinder &mutationsFinder,
                     std::vector<std::string> originalArgs,
                     mull::Diagnostics &diagnostics,
                     const mull::Configuration &config) :
        inputObjects(inputObjects), filters(filters),
        mutationsFinder(mutationsFinder), originalArgs(originalArgs),
        diagnostics(diagnostics), config(config),
        singleTask(diagnostics) {}
    void run();
private:
    std::vector<mull::MutationPoint *> findMutationPoints(mull::Program &program);
    std::vector<mull::MutationPoint *> filterMutations(std::vector<mull::MutationPoint *> mutationPoints);
    void selectInstructions(std::vector<mull::FunctionUnderTest> &functions);
    void applyMutation(mull::Program &program, std::vector<mull::MutationPoint *> &mutationPoints);
    void link(std::vector<std::string> objectFiles);
};
}

#endif /* LInkerInvocation_hpp */
