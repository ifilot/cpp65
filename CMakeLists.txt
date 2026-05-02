cmake_minimum_required(VERSION 3.20)

project(cpp65
    VERSION 0.1.0
    DESCRIPTION "6502/65C02 Emulator written in C++20"
    LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)

option(CPP65_BUILD_TESTS "Build tests" ON)
option(CPP65_BUILD_EXAMPLES "Build examples" ON)
option(CPP65_ENABLE_WARNINGS "Enable compiler warnings" ON)

function(cpp65_apply_warnings target)
    if(CPP65_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
endfunction()

add_library(cpp65_core
    src/bus.cpp
    src/cpu.cpp
)

target_compile_features(cpp65_core PUBLIC cxx_std_20)

target_include_directories(cpp65_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

cpp65_apply_warnings(cpp65_core)

if(UNIX)
    target_compile_definitions(cpp65_core PUBLIC PLATFORM_LINUX)
elseif(WIN32)
    target_compile_definitions(cpp65_core PUBLIC PLATFORM_WINDOWS)
endif()

add_executable(cpp65
    src/main.cpp
)

target_link_libraries(cpp65
    PRIVATE
    cpp65_core
)

cpp65_apply_warnings(cpp65)

set_target_properties(cpp65 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(CPP65_BUILD_EXAMPLES)
    add_executable(cpp65_example_load_store
        examples/load_store.cpp
    )
    target_link_libraries(cpp65_example_load_store PRIVATE cpp65_core)
    cpp65_apply_warnings(cpp65_example_load_store)

    add_executable(cpp65_example_branch_loop
        examples/branch_loop.cpp
    )
    target_link_libraries(cpp65_example_branch_loop PRIVATE cpp65_core)
    cpp65_apply_warnings(cpp65_example_branch_loop)

    add_executable(cpp65_example_subroutine
        examples/subroutine.cpp
    )
    target_link_libraries(cpp65_example_subroutine PRIVATE cpp65_core)
    cpp65_apply_warnings(cpp65_example_subroutine)

    add_executable(cpp65_example_memory_mapped_console
        examples/memory_mapped_console.cpp
    )
    target_link_libraries(cpp65_example_memory_mapped_console PRIVATE cpp65_core)
    cpp65_apply_warnings(cpp65_example_memory_mapped_console)
endif()

if(CPP65_BUILD_TESTS)
    enable_testing()

    add_executable(cpp65_cpu_instruction_test
        tests/cpu_instruction_test.cpp
    )

    target_link_libraries(cpp65_cpu_instruction_test
        PRIVATE
        cpp65_core
    )

    cpp65_apply_warnings(cpp65_cpu_instruction_test)

    add_test(
        NAME cpu_instruction
        COMMAND cpp65_cpu_instruction_test
    )

    add_executable(cpp65_bus_test
        tests/bus_test.cpp
    )

    target_link_libraries(cpp65_bus_test
        PRIVATE
        cpp65_core
    )

    cpp65_apply_warnings(cpp65_bus_test)

    add_test(
        NAME bus
        COMMAND cpp65_bus_test
    )

    if(CPP65_BUILD_EXAMPLES)
        add_test(
            NAME integration_memory_mapped_console_output
            COMMAND cpp65_example_memory_mapped_console
        )
        set_tests_properties(
            integration_memory_mapped_console_output
            PROPERTIES
            PASS_REGULAR_EXPRESSION "^Hello\n$"
        )
    endif()
endif()
