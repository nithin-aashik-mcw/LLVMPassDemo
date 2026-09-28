#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class PassBuilder;
} // namespace llvm

class DeadCodeEliminationPass : public llvm::PassInfoMixin<DeadCodeEliminationPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerDeadCodeEliminationPass(llvm::PassBuilder &PB);
