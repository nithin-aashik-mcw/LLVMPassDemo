execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
    OUTPUT_VARIABLE LLVM_PASS_HOST_TRIPLE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

function(add_llvm_pass_executable TARGET)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-ir/${TARGET})
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    set(OBJECTS)

    foreach(SOURCE ${ARG_SOURCES})
        get_filename_component(NAME ${SOURCE} NAME_WE)

        set(IR_FILE ${GENERATED_DIR}/${NAME}.ll)
        set(OPT_IR_FILE ${GENERATED_DIR}/${NAME}.opt.ll)
        set(OBJ_FILE ${GENERATED_DIR}/${NAME}.o)

        # C++ → LLVM IR
        add_custom_command(
            OUTPUT ${IR_FILE}
            COMMAND
                ${LLVM_TOOLS_BINARY_DIR}/clang++
                -std=c++17
                -S
                -emit-llvm
                -target ${LLVM_PASS_HOST_TRIPLE}
                -Xclang -disable-O0-optnone
                -I${CMAKE_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
                -o ${IR_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
            VERBATIM
        )

        # LLVM IR → Custom pass → optimized IR
        add_custom_command(
            OUTPUT ${OPT_IR_FILE}
            COMMAND
                ${LLVM_TOOLS_BINARY_DIR}/opt
                -load-pass-plugin=$<TARGET_FILE:CustomPlugin>
                -passes=a-pass,cf-pass,cp-pass,dce-pass,sr-pass
                ${IR_FILE}
                -S
                -o ${OPT_IR_FILE}
            DEPENDS ${IR_FILE} CustomPlugin
            VERBATIM
        )

        # Optimized IR → object
        add_custom_command(
            OUTPUT ${OBJ_FILE}
            COMMAND
                ${LLVM_TOOLS_BINARY_DIR}/clang++
                -c
                -target ${LLVM_PASS_HOST_TRIPLE}
                ${OPT_IR_FILE}
                -o ${OBJ_FILE}
            DEPENDS ${OPT_IR_FILE}
            VERBATIM
        )

        list(APPEND OBJECTS ${OBJ_FILE})
    endforeach()

    add_executable(${TARGET} ${OBJECTS})

    # The sources are plain .o files, so CMake can't infer a language for
    # the link step from their extension — tell it explicitly.
    set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()
