#include "ConstantFolding.h"

#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;

PreservedAnalyses ConstantFoldingPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("ConstantFoldingPass", F.getName());

    const auto &DL = F.getDataLayout();

    auto Changed = false;
    for (auto &I : make_early_inc_range(instructions(F))) {
        auto *C = ConstantFoldInstruction(&I, DL);
        if (!C)
            continue;

        LOG_INFO() << "  " << I << "  ->  " << *C << "\n";
        I.replaceAllUsesWith(C);
        I.eraseFromParent();
        Changed = true;
    }

    if (!Changed)
        return PreservedAnalyses::all();

    auto PA = PreservedAnalyses();
    PA.preserveSet<CFGAnalyses>();
    return PA;
}
void registerConstantFoldingPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
