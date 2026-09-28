#include "Algebraic.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"

#include "util.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

bool isSameValue(Value *L, Value *R) {
    if (L == R)
        return true;

    if (!isInstruction(L, Instruction::Load) || !isInstruction(R, Instruction::Load))
        return false;
    auto *LI1 = static_cast<LoadInst *>(L);
    auto *LI2 = static_cast<LoadInst *>(R);
    if (!LI1->isSimple() || !LI2->isSimple())
        return false;
    if (LI1->getPointerOperand() != LI2->getPointerOperand() || LI1->getType() != LI2->getType() ||
        LI1->getParent() != LI2->getParent())
        return false;

    if (LI2->comesBefore(LI1))
        std::swap(LI1, LI2);
    for (auto It = std::next(LI1->getIterator()); &*It != LI2; ++It)
        if (It->mayWriteToMemory())
            return false;
    return true;
}

Value *simplifyAlgebraic(BinaryOperator *BO) {
    auto *L = BO->getOperand(0);
    auto *R = BO->getOperand(1);
    auto *Ty = BO->getType();

    switch (BO->getOpcode()) {
    case Instruction::Add:
        if (match(R, m_Zero())) // X + 0 -> X
            return L;
        break;

    case Instruction::Sub:
        if (isSameValue(L, R)) // X - X -> 0
            return Constant::getNullValue(Ty);
        break;

    case Instruction::Mul:
        if (match(R, m_One())) // X * 1 -> X
            return L;
        break;

    case Instruction::SDiv:
        if (isSameValue(L, R)) // X / X -> 1 (X == 0 is UB)
            return ConstantInt::get(Ty, 1);
        break;

    default:
        break;
    }
    return nullptr;
}

} // namespace

PreservedAnalyses AlgebraicPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("AlgebraicPass", F.getName());

    auto Changed = false;
    for (auto &I : make_early_inc_range(instructions(F))) {
        if (!I.isBinaryOp())
            continue;
        auto *BO = static_cast<BinaryOperator *>(&I);

        auto *NewV = simplifyAlgebraic(BO);
        if (!NewV)
            continue;

        LOG_INFO() << "  " << *BO << "  ->  " << *NewV << "\n";
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

void registerAlgebraicPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
