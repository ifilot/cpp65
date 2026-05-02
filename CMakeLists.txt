# SPDX-License-Identifier: LGPL-3.0-or-later
#
# cpp65 is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.

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

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/cpp65/config.h
    @ONLY
)

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
    ${CMAKE_CURRENT_BINARY_DIR}/generated
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

    add_executable(cpp65_example_sieve
        examples/sieve.cpp
    )
    target_link_libraries(cpp65_example_sieve PRIVATE cpp65_core)
    cpp65_apply_warnings(cpp65_example_sieve)
endif()

if(CPP65_BUILD_TESTS)
    enable_testing()
    find_program(CPP65_CA65 ca65)
    find_program(CPP65_LD65 ld65)

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

    if(CPP65_CA65 AND CPP65_LD65)
        set(CPP65_ASM_CONSOLE_SOURCE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/asm/console_output.s
        )
        set(CPP65_ASM_CONSOLE_CONFIG
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/asm/console_output.cfg
        )
        set(CPP65_ASM_CONSOLE_OBJECT
            ${CMAKE_CURRENT_BINARY_DIR}/tests/asm/console_output.o
        )
        set(CPP65_ASM_CONSOLE_BINARY
            ${CMAKE_CURRENT_BINARY_DIR}/tests/asm/console_output.bin
        )

        add_custom_command(
            OUTPUT ${CPP65_ASM_CONSOLE_OBJECT}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${CMAKE_CURRENT_BINARY_DIR}/tests/asm
            COMMAND ${CPP65_CA65}
                    --cpu 65C02
                    -o ${CPP65_ASM_CONSOLE_OBJECT}
                    ${CPP65_ASM_CONSOLE_SOURCE}
            DEPENDS ${CPP65_ASM_CONSOLE_SOURCE}
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${CPP65_ASM_CONSOLE_BINARY}
            COMMAND ${CPP65_LD65}
                    -C ${CPP65_ASM_CONSOLE_CONFIG}
                    -o ${CPP65_ASM_CONSOLE_BINARY}
                    ${CPP65_ASM_CONSOLE_OBJECT}
            DEPENDS
                ${CPP65_ASM_CONSOLE_OBJECT}
                ${CPP65_ASM_CONSOLE_CONFIG}
            VERBATIM
        )

        add_custom_target(cpp65_asm_console_program
            DEPENDS ${CPP65_ASM_CONSOLE_BINARY}
        )

        add_executable(cpp65_asm65_console_test
            tests/asm65_console_test.cpp
        )

        target_link_libraries(cpp65_asm65_console_test
            PRIVATE
            cpp65_core
        )

        cpp65_apply_warnings(cpp65_asm65_console_test)

        add_dependencies(cpp65_asm65_console_test cpp65_asm_console_program)

        add_test(
            NAME integration_asm65_console_program
            COMMAND cpp65_asm65_console_test ${CPP65_ASM_CONSOLE_BINARY}
        )
    else()
        message(STATUS "ca65/ld65 not found; skipping asm65 integration test")
    endif()

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

        add_test(
            NAME integration_sieve_console_output
            COMMAND cpp65_example_sieve
        )
        set_tests_properties(
            integration_sieve_console_output
            PROPERTIES
            PASS_REGULAR_EXPRESSION "^02 03 05 07 0B 0D 11 13 17 1D 1F \n$"
        )
    endif()
endif()
