# LLVMPassDemo

A small out-of-tree LLVM pass plugin with five toy optimization passes, plus a
demo app that shows what each one does.

- `llvm-pass/` builds the plugin, `CustomPlugin.so`.
- `src/` holds small C++ functions written to trigger each pass. At build time
  they are compiled to LLVM IR, run through the plugin, and linked into `MyApp`.

## Requirements

- LLVM and Clang with CMake config files (tested with LLVM 22.1.0 in `/usr/local`)
- CMake 3.20+
- Ninja or Make

## Build and run

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/src/MyApp
```

If CMake can't find LLVM, add `-DLLVM_DIR=/usr/local/lib/cmake/llvm`.

To see what the passes changed, compare the IR before and after:

```bash
diff build/llvm-ir/MyApp/StrengthReduction.ll build/llvm-ir/MyApp/StrengthReduction.opt.ll
```

The passes also print what they rewrite during the build.

## The passes

| Pass       | What it does                                              |
|------------|-----------------------------------------------------------|
| `a-pass`   | Algebraic identities: `x/x`, `x-x`, `x*1`, `x+0`          |
| `cf-pass`  | Folds instructions whose operands are all constants       |
| `cp-pass`  | Replaces loads of a local with the value last stored      |
| `dce-pass` | Removes dead instructions and variables nobody reads      |
| `sr-pass`  | Turns multiply by a power of two into a left shift        |

They run on `MyApp` in the order above. To turn one off, reconfigure with its
option set to `OFF`:

```bash
cmake -S . -B build -DENABLE_DCE_PASS=OFF
```

The options are `ENABLE_A_PASS`, `ENABLE_CF_PASS`, `ENABLE_CP_PASS`,
`ENABLE_DCE_PASS` and `ENABLE_SR_PASS`.

## Running a pass yourself

```bash
clang++ -S -emit-llvm -Xclang -disable-O0-optnone input.cpp -o input.ll
opt -load-pass-plugin=build/llvm-pass/CustomPlugin.so -passes=sr-pass input.ll -S -o out.ll
```

List several passes with commas, e.g. `-passes=cp-pass,dce-pass`.

The passes also add themselves to the start of the standard `-O` pipelines, so
loading the plugin with `-O2` runs all five as well.

## Adding a pass

1. Add a header in `llvm-pass/include/` and a source file in `llvm-pass/src/`.
   Copy an existing pass such as `StrengthReduction.cpp` and change its name.
2. Call its `register...Pass(PB)` function in `llvm-pass/src/CustomPlugin.cpp`.
3. Add the source file to `llvm-pass/CMakeLists.txt`.
4. To demo it in `MyApp`, add a test function in `src/`, call it from
   `src/main.cpp`, list the file in `src/CMakeLists.txt`, and add an
   `ENABLE_..._PASS` option in the top-level `CMakeLists.txt`.

Then rerun `cmake -S . -B build` and build.

## Code style

Formatting is enforced with clang-format through pre-commit:

```bash
pip install pre-commit
pre-commit install
```
