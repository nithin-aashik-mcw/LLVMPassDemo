#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"

#include "Algebraic.h"
#include "ConstantFolding.h"
#include "ConstantPropagation.h"
#include "DeadCodeElimination.h"
#include "StrengthReduction.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "CustomPlugin", LLVM_VERSION_STRING, [](PassBuilder &PB) {
                registerStrengthReductionPass(PB);
                registerConstantFoldingPass(PB);
                registerAlgebraicPass(PB);
                registerConstantPropagationPass(PB);
                registerDeadCodeEliminationPass(PB);
            }};
}
