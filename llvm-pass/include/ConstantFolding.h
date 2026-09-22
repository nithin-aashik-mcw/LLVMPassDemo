#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class PassBuilder;
} // namespace llvm

class ConstantFoldingPass : public llvm::PassInfoMixin<ConstantFoldingPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerConstantFoldingPass(llvm::PassBuilder &PB);
