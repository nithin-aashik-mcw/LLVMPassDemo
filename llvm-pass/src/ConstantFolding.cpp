#include "ConstantFolding.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses ConstantFoldingPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("ConstantFoldingPass", F.getName());
    return PreservedAnalyses::all();
}

void registerConstantFoldingPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "cf-pass") {
            FPM.addPass(ConstantFoldingPass());
            return true;
        }
        return false;
    });
    PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
        MPM.addPass(createModuleToFunctionPassAdaptor(ConstantFoldingPass()));
    });
}
