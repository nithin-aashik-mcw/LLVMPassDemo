#include "ConstantPropagation.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses ConstantPropagationPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("ConstantPropagationPass", F.getName());
    return PreservedAnalyses::all();
}

void registerConstantPropagationPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "cp-pass") {
            FPM.addPass(ConstantPropagationPass());
            return true;
        }
        return false;
    });
    PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
        MPM.addPass(createModuleToFunctionPassAdaptor(ConstantPropagationPass()));
    });
}
