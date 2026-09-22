#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class PassBuilder;
} // namespace llvm

class StrengthReductionPass : public llvm::PassInfoMixin<StrengthReductionPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerStrengthReductionPass(llvm::PassBuilder &PB);
