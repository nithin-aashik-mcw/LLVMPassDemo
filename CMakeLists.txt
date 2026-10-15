cmake_minimum_required(VERSION 3.20)

project(LLVMPassDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

# Per-pass toggles for the opt pipeline run on the demo executable's IR.
# e.g. cmake -S . -B build -DENABLE_DCE_PASS=OFF
option(ENABLE_A_PASS   "Run the algebraic simplification pass (a-pass)" ON)
option(ENABLE_CF_PASS  "Run the constant folding pass (cf-pass)"        ON)
option(ENABLE_CP_PASS  "Run the constant propagation pass (cp-pass)"    ON)
option(ENABLE_DCE_PASS "Run the dead code elimination pass (dce-pass)"  ON)
option(ENABLE_SR_PASS  "Run the strength reduction pass (sr-pass)"      ON)

set(LLVM_CUSTOM_PASSES)
if(ENABLE_A_PASS)
    list(APPEND LLVM_CUSTOM_PASSES a-pass)
endif()
if(ENABLE_CF_PASS)
    list(APPEND LLVM_CUSTOM_PASSES cf-pass)
endif()
if(ENABLE_CP_PASS)
    list(APPEND LLVM_CUSTOM_PASSES cp-pass)
endif()
if(ENABLE_DCE_PASS)
    list(APPEND LLVM_CUSTOM_PASSES dce-pass)
endif()
if(ENABLE_SR_PASS)
    list(APPEND LLVM_CUSTOM_PASSES sr-pass)
endif()
list(JOIN LLVM_CUSTOM_PASSES "," LLVM_CUSTOM_PASS_PIPELINE)
if(LLVM_CUSTOM_PASS_PIPELINE)
    message(STATUS "Custom pass pipeline: ${LLVM_CUSTOM_PASS_PIPELINE}")
else()
    message(STATUS "Custom pass pipeline: (none, all ENABLE_*_PASS options are OFF)")
endif()

add_subdirectory(mat-mul)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(LLVMCustomPass)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(llvm-pass)
add_subdirectory(src)
