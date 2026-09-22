#include "DeadCodeElimination.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses DeadCodeEliminationPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("DeadCodeEliminationPass", F.getName());
    return PreservedAnalyses::all();
}

void registerDeadCodeEliminationPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "dce-pass") {
            FPM.addPass(DeadCodeEliminationPass());
            return true;
        }
        return false;
    });
    PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
        MPM.addPass(createModuleToFunctionPassAdaptor(DeadCodeEliminationPass()));
    });
}
