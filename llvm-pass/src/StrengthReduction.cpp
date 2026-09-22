#include "StrengthReduction.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses StrengthReductionPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("StrengthReductionPass", F.getName());
    return PreservedAnalyses::all();
}

void registerStrengthReductionPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "sr-pass") {
            FPM.addPass(StrengthReductionPass());
            return true;
        }
        return false;
    });
    PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
        MPM.addPass(createModuleToFunctionPassAdaptor(StrengthReductionPass()));
    });
}
