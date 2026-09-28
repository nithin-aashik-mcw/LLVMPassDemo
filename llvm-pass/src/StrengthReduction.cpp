#include "StrengthReduction.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

Value *reduceStrength(BinaryOperator *BO) {
    if (BO->getOpcode() != Instruction::Mul)
        return nullptr;

    auto *L = BO->getOperand(0);
    auto *R = BO->getOperand(1);
    const APInt *C;
    if (!match(R, m_Power2(C))) {
        if (!match(L, m_Power2(C)))
            return nullptr;
        std::swap(L, R);
    }

    auto *Ty = BO->getType();
    auto K = C->logBase2();
    auto NSW = BO->hasNoSignedWrap() && K < Ty->getScalarSizeInBits() - 1;
    return IRBuilder<>(BO).CreateShl(L, ConstantInt::get(Ty, K), "", BO->hasNoUnsignedWrap(), NSW);
}

} // namespace

PreservedAnalyses StrengthReductionPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("StrengthReductionPass", F.getName());

    auto Changed = false;
    for (auto &I : make_early_inc_range(instructions(F))) {
        if (!I.isBinaryOp())
            continue;
        auto *BO = static_cast<BinaryOperator *>(&I);

        auto *NewV = reduceStrength(BO);
        if (!NewV)
            continue;

        LOG_INFO() << "  " << *BO << "  ->  " << *NewV << "\n";
        NewV->takeName(BO);
        BO->replaceAllUsesWith(NewV);
        BO->eraseFromParent();
        Changed = true;
    }

    if (!Changed)
        return PreservedAnalyses::all();

    auto PA = PreservedAnalyses();
    PA.preserveSet<CFGAnalyses>();
    return PA;
}
void registerStrengthReductionPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
