#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class PassBuilder;
} // namespace llvm

class ConstantPropagationPass : public llvm::PassInfoMixin<ConstantPropagationPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerConstantPropagationPass(llvm::PassBuilder &PB);
