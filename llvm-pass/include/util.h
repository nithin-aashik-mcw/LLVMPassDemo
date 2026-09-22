#pragma once

#include "llvm/Support/raw_ostream.h"

inline int PassLogCount = 0;

#define LOG_PASS_FUNCTION(Label, FuncName)                                  \
    do {                                                                    \
        llvm::errs().changeColor(llvm::raw_ostream::GREEN, /*Bold=*/true)   \
            << "[" << ++PassLogCount << "] " << (Label) << " : ";           \
        llvm::errs().changeColor(llvm::raw_ostream::GREEN) << (FuncName);   \
        llvm::errs().resetColor() << "\n";                                  \
    } while (0)

namespace detail {

struct ColorLogger {
    llvm::raw_ostream &OS;

    ColorLogger(llvm::raw_ostream &OS, llvm::raw_ostream::Colors Color) : OS(OS) {
        OS.changeColor(Color);
    }

    ~ColorLogger() { OS.resetColor(); }
};

template <typename T>
inline llvm::raw_ostream &operator<<(ColorLogger &&Logger, const T &Val) {
    return Logger.OS << Val;
}

} // namespace detail

#define LOG_INFO() ::detail::ColorLogger(llvm::errs(), llvm::raw_ostream::BLUE)