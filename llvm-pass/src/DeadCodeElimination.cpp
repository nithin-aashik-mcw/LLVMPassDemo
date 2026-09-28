#include "DeadCodeElimination.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Utils/Local.h"

#include <algorithm>
#include <vector>

#include "util.h"

using namespace llvm;

namespace {

bool isWriteOnly(AllocaInst *AI) {
    return std::all_of(AI->user_begin(), AI->user_end(), [AI](auto *U) {
        if (!isInstruction(U, Instruction::Store))
            return false;
        auto *SI = static_cast<StoreInst *>(U);
        return SI->isSimple() && SI->getPointerOperand() == AI;
    });
}

} // namespace

PreservedAnalyses DeadCodeEliminationPass::run(Function &F, FunctionAnalysisManager &) {
    LOG_PASS_FUNCTION("DeadCodeEliminationPass", F.getName());

    auto Changed = false;
    auto Dead = std::vector<Instruction *>();
    do {
        Dead.clear();
        for (auto &I : instructions(F)) {
            if (I.getOpcode() == Instruction::Alloca &&
                isWriteOnly(static_cast<AllocaInst *>(&I))) {
                for (auto *U : I.users())
                    Dead.push_back(static_cast<Instruction *>(U));
                Dead.push_back(&I);
            } else if (isInstructionTriviallyDead(&I)) {
                Dead.push_back(&I);
            }
        }

        for (auto *I : Dead) {
            LOG_INFO() << "  removed " << *I << "\n";
            I->eraseFromParent();
        }
        Changed |= !Dead.empty();
    } while (!Dead.empty());

    if (!Changed)
        return PreservedAnalyses::all();

    auto PA = PreservedAnalyses();
    PA.preserveSet<CFGAnalyses>();
    return PA;
}
void registerDeadCodeEliminationPass(PassBuilder &PB) {

    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
