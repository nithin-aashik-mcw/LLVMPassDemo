#include "ConstantPropagation.h"

#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

#include <unordered_map>
#include <unordered_set>

#include "util.h"

using namespace llvm;

namespace {

bool isTrackable(AllocaInst *AI) {
    auto *Ty = AI->getAllocatedType();
    for (auto *U : AI->users()) {
        if (isInstruction(U, Instruction::Load)) {
            auto *LI = static_cast<LoadInst *>(U);
            if (!LI->isSimple() || LI->getType() != Ty)
                return false;
        } else if (isInstruction(U, Instruction::Store)) {
            auto *SI = static_cast<StoreInst *>(U);
            if (!SI->isSimple() || SI->getPointerOperand() != AI ||
                SI->getValueOperand()->getType() != Ty)
                return false;
        } else {
            return false;
        }
    }
    return true;
}

AllocaInst *trackedVariable(Value *Ptr, const std::unordered_set<AllocaInst *> &Trackable) {
    if (!isInstruction(Ptr, Instruction::Alloca))
        return nullptr;
    auto *AI = static_cast<AllocaInst *>(Ptr);
    return Trackable.count(AI) ? AI : nullptr;
}

} // namespace

PreservedAnalyses ConstantPropagationPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("ConstantPropagationPass", F.getName());

    const auto &DL = F.getDataLayout();

    auto Trackable = std::unordered_set<AllocaInst *>();
    for (auto &I : instructions(F)) {
        if (I.getOpcode() != Instruction::Alloca)
            continue;
        auto *AI = static_cast<AllocaInst *>(&I);
        if (isTrackable(AI))
            Trackable.insert(AI);
    }

    auto Changed = false;
    for (auto &BB : F) {
        auto Known = std::unordered_map<AllocaInst *, Value *>();

        for (auto &I : make_early_inc_range(BB)) {
            if (I.getOpcode() == Instruction::Store) {
                auto *SI = static_cast<StoreInst *>(&I);
                if (auto *AI = trackedVariable(SI->getPointerOperand(), Trackable))
                    Known[AI] = SI->getValueOperand();
                continue;
            }

            Value *NewV = nullptr;
            if (I.getOpcode() == Instruction::Load) {
                auto *LI = static_cast<LoadInst *>(&I);
                if (auto *AI = trackedVariable(LI->getPointerOperand(), Trackable)) {
                    auto It = Known.find(AI);
                    if (It != Known.end())
                        NewV = It->second;
                }
            } else {
                NewV = ConstantFoldInstruction(&I, DL);
            }
            if (!NewV)
                continue;

            LOG_INFO() << "  " << I << "  ->  " << *NewV << "\n";
            I.replaceAllUsesWith(NewV);
            I.eraseFromParent();
            Changed = true;
        }
    }

    if (!Changed)
        return PreservedAnalyses::all();

    auto PA = PreservedAnalyses();
    PA.preserveSet<CFGAnalyses>();
    return PA;
}
void registerConstantPropagationPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
