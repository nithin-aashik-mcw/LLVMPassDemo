# LLVMPassDemo

An out-of-tree LLVM pass plugin demo. It builds a loadable pass plugin
(`CustomPlugin.so`) containing five toy IR-optimization passes, and a small
demo pipeline (`MyApp`) that compiles hand-written C++ "before" snippets to
LLVM IR, runs them through the plugin, and links the optimized result into an
executable — so you can see each pass's effect end to end.

## Layout

```
.
├── CMakeLists.txt              top-level build: finds LLVM, wires up both subdirs
├── cmake/
│   └── LLVMCustomPass.cmake    add_llvm_pass_executable(): C++ → .ll → opt (CustomPlugin) → .o → exe
├── llvm-pass/                  the pass PLUGIN itself → builds CustomPlugin.so
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── util.h              LOG_PASS_FUNCTION / LOG_INFO logging helpers
│   │   ├── Algebraic.h
│   │   ├── ConstantFolding.h
│   │   ├── ConstantPropagation.h
│   │   ├── DeadCodeElimination.h
│   │   └── StrengthReduction.h
│   └── src/
│       ├── CustomPlugin.cpp    plugin entry point: registers all five passes
│       ├── Algebraic.cpp
│       ├── ConstantFolding.cpp
│       ├── ConstantPropagation.cpp
│       ├── DeadCodeElimination.cpp
│       └── StrengthReduction.cpp
├── src/                        demo WORKLOAD compiled to IR and optimized → builds MyApp
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── Algebraic.cpp           `a/a`, `b-b`, `result/result`, ... → algebraic simplification
│   ├── ConstantFolding.cpp     compile-time-constant arithmetic → constant folding
│   ├── ConstantPropagation.cpp copies of known values → constant/copy propagation
│   ├── DeadCodeElimination.cpp redundant/reassigned values → dead code elimination
│   └── StrengthReduction.cpp   multiply by power-of-2 → shift
└── build/                      out-of-source build directory (git-ignored)
```

`src/*.cpp` and `llvm-pass/src/*.cpp` share names by design but are two
different things:

- **`llvm-pass/src/`** — the actual LLVM `PassInfoMixin` implementations that
  get compiled into `CustomPlugin.so`.
- **`src/`** — plain C++ "before" snippets, each written to trigger the
  anti-pattern its same-named pass cleans up. These are compiled to `.ll`,
  fed through `CustomPlugin.so`, and the optimized `.ll` is what actually
  gets linked into `MyApp`.

## Prerequisites

- LLVM + Clang, built/installed with CMake config files (`find_package(LLVM
  CONFIG)` must succeed) — this repo was set up against **LLVM 22.1.0**
  installed at `/usr/local` (`LLVM_DIR=/usr/local/lib/cmake/llvm`). Any LLVM
  version with the new-PM `PassPlugin` API (LLVM ≥ 13) should work.
- CMake ≥ 3.20
- A generator/build tool: Ninja (recommended — a `build.ninja` is already
  used in this checkout) or Make
- A C++17 compiler (the project builds `clang++` itself against LLVM headers,
  and also uses `clang++`/`opt` from the LLVM install to compile/optimize the
  `src/` workload at build time)

## Building

All commands assume you are at the repo root
(`/home/aashik/Project/LLVMPassDemo` under WSL).

### 1. Configure

Pick one generator and one build type — any combination works.

```bash
# Default generator (Ninja, if available), no explicit build type
cmake -S . -B build

# Explicit Ninja generator
cmake -S . -B build -G Ninja

# Explicit Unix Makefiles generator
cmake -S . -B build -G "Unix Makefiles"

# With an explicit build type
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel

# If LLVM isn't auto-discoverable, point CMake at it explicitly
cmake -S . -B build -DLLVM_DIR=/usr/local/lib/cmake/llvm
```

Use a different build directory name if you want multiple side-by-side
configurations, e.g. `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug`.

### 2. Build

Any of these build **everything** (`CustomPlugin.so` and `MyApp`):

```bash
cmake --build build                 # generator-agnostic, works for Ninja or Make
cmake --build build -j$(nproc)      # parallel, explicit job count
ninja -C build                      # if configured with -G Ninja
(cd build && make -j$(nproc))       # if configured with -G "Unix Makefiles"
```

Build just one target:

```bash
cmake --build build --target CustomPlugin   # only the pass plugin (.so)
cmake --build build --target MyApp          # only the demo executable (pulls in CustomPlugin as a dependency)
```

Clean / rebuild:

```bash
cmake --build build --target clean
rm -rf build && cmake -S . -B build && cmake --build build   # full reconfigure
```

### 3. Run the demo

```bash
./build/src/MyApp
```

This runs the plugin-optimized versions of `algrbraic`, `constantFolding`,
`constantPropagation`, `deadCodeElimination`, and `strengthReduction`. You
can also inspect the intermediate IR CMake generated along the way:

```
build/src/llvm-ir/MyApp/<Name>.ll        # C++ → LLVM IR (unoptimized, -O0 with optnone disabled)
build/src/llvm-ir/MyApp/<Name>.opt.ll    # after CustomPlugin.so's passes
build/src/llvm-ir/MyApp/<Name>.o         # compiled object
```

## Running the passes manually with `opt`

Useful for experimenting on your own `.ll`/`.c`/`.cpp` files without going
through the `MyApp` pipeline.

Build the plugin first (`cmake --build build --target CustomPlugin`), then:

```bash
# 1. Get LLVM IR from a C++ source (matches what LLVMCustomPass.cmake does)
clang++ -std=c++17 -S -emit-llvm -Xclang -disable-O0-optnone input.cpp -o input.ll

# 2. Run a single named pass
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=sr-pass input.ll -S -o out.ll

# 3. Run several named passes, comma-separated, in the order given
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so \
    -passes=a-pass,cf-pass,cp-pass,dce-pass,sr-pass \
    input.ll -S -o out.ll

# Any subset / any order is valid, e.g.:
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=cf-pass input.ll -S -o out.ll
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=sr-pass,a-pass input.ll -S -o out.ll
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=dce-pass,cp-pass,cf-pass input.ll -S -o out.ll
```

### Registered pass names

| Name in `-passes=` | Source                                   | What it does                                            |
|---------------------|-------------------------------------------|-----------------------------------------------------------|
| `a-pass`             | `llvm-pass/src/Algebraic.cpp`             | Algebraic identity simplification (`x/x`, `x-x`, ...)     |
| `cf-pass`            | `llvm-pass/src/ConstantFolding.cpp`       | Constant folding                                           |
| `cp-pass`            | `llvm-pass/src/ConstantPropagation.cpp`   | Constant / copy propagation                                 |
| `dce-pass`           | `llvm-pass/src/DeadCodeElimination.cpp`   | Dead / redundant code elimination                            |
| `sr-pass`            | `llvm-pass/src/StrengthReduction.cpp`     | Strength reduction (`x * 2^n` → `x << n`)                     |

> **Note:** every pass registers *both* a `registerPipelineParsingCallback`
> (so it can be named explicitly in `-passes=`) **and** a
> `registerPipelineStartEPCallback` (so it also auto-inserts itself at the
> start of *any* standard pipeline). This means loading `CustomPlugin.so` and
> running a normal optimization level also runs all five custom passes,
> regardless of what you put in `-passes=`:
>
> ```bash
> opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes='default<O2>' input.ll -S -o out.ll
> opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -O1 input.ll -S -o out.ll
> ```

## Adding a new pass

Say you want to add a pass named `my-pass` implemented by `MyPass`.

1. **Header** — `llvm-pass/include/MyPass.h`:

   ```cpp
   #pragma once
   #include "llvm/IR/PassManager.h"

   namespace llvm { class PassBuilder; }

   class MyPass : public llvm::PassInfoMixin<MyPass> {
   public:
       llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
   };

   void registerMyPass(llvm::PassBuilder &PB);
   ```

2. **Implementation** — `llvm-pass/src/MyPass.cpp`, following the existing
   passes' pattern (see `StrengthReduction.cpp`):

   ```cpp
   #include "MyPass.h"
   #include "llvm/Passes/PassBuilder.h"
   #include "util.h"

   using namespace llvm;

   PreservedAnalyses MyPass::run(Function &F, FunctionAnalysisManager &) {
       LOG_PASS_FUNCTION("MyPass", F.getName());
       bool Changed = false;
       // ... transform F ...
       return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
   }

   void registerMyPass(PassBuilder &PB) {
       // Makes `-passes=my-pass` work
       PB.registerPipelineParsingCallback(
           [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
               if (Name == "my-pass") {
                   FPM.addPass(MyPass());
                   return true;
               }
               return false;
           });

       // Optional: also auto-run at the start of every standard -O pipeline.
       // Omit this callback if you only want the pass to run when named explicitly.
       PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel) {
           MPM.addPass(createModuleToFunctionPassAdaptor(MyPass()));
       });
   }
   ```

3. **Register it in the plugin entry point** — `llvm-pass/src/CustomPlugin.cpp`:

   ```cpp
   #include "MyPass.h"
   // ...
   registerMyPass(PB);
   ```

4. **Add the source file to the plugin's CMake target** —
   `llvm-pass/CMakeLists.txt`:

   ```cmake
   add_library(CustomPlugin MODULE
       src/CustomPlugin.cpp
       src/Algebraic.cpp
       src/ConstantFolding.cpp
       src/ConstantPropagation.cpp
       src/DeadCodeElimination.cpp
       src/StrengthReduction.cpp
       src/MyPass.cpp
   )
   ```

5. *(Optional)* **Add a matching workload snippet** so `MyApp` demos it:
   - Add `src/MyPass.cpp` with a small function written to trigger what
     `MyPass` fixes.
   - Declare and call it from `src/main.cpp` (`extern int myPass(); ...`).
   - Add the file to `src/CMakeLists.txt`'s `SOURCES` list.
   - Add `my-pass` to the `-passes=` list in `cmake/LLVMCustomPass.cmake`
     (order in that comma-separated list is the order the passes run in):

     ```cmake
     -passes=a-pass,cf-pass,cp-pass,dce-pass,sr-pass,my-pass
     ```

6. **Reconfigure and rebuild** (new source files require a re-run of cmake):

   ```bash
   cmake -S . -B build
   cmake --build build
   ```

7. **Verify** it standalone before relying on the full pipeline:

   ```bash
   opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=my-pass src/MyPass.ll -S -o -
   ```

Steps 1–4 are required for the pass to exist at all. Step 4's ordering in
`CustomPlugin.cpp` doesn't affect `-passes=` behavior (each name is looked up
independently), but it does affect the order passes are appended by
`registerPipelineStartEPCallback` when a whole `-O*` pipeline is run. Steps
5–6 are only needed if you also want it demoed by `MyApp`.

## Code style

`.clang-format` defines the style; `.pre-commit-config.yaml` runs
`clang-format -i` on staged `.c/.cc/.cpp/.cxx/.h/.hpp/.hxx/.inc` files, plus
whitespace/EOF/merge-conflict checks. Enable it with:

```bash
pip install pre-commit
pre-commit install
```
