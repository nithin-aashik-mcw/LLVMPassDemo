add_library(CustomPlugin MODULE
    src/CustomPlugin.cpp
    src/Algebraic.cpp
    src/ConstantFolding.cpp
    src/ConstantPropagation.cpp
    src/DeadCodeElimination.cpp
    src/StrengthReduction.cpp
)

target_include_directories(CustomPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_features(CustomPlugin PRIVATE cxx_std_17)

if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(CustomPlugin PRIVATE -fno-rtti)
endif()

set_target_properties(CustomPlugin PROPERTIES
    PREFIX ""
)
