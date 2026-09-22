#include "Algebraic.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses AlgebraicPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("AlgebraicPass", F.getName());
    return PreservedAnalyses::all();
}

void registerAlgebraicPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "a-pass") {
            FPM.addPass(AlgebraicPass());
            return true;
        }
        return false;
    });
    PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
        MPM.addPass(createModuleToFunctionPassAdaptor(AlgebraicPass()));
    });
}
