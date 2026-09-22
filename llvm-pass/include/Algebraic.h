#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class PassBuilder;
} // namespace llvm

class AlgebraicPass : public llvm::PassInfoMixin<AlgebraicPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerAlgebraicPass(llvm::PassBuilder &PB);
