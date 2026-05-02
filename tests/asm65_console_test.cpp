// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint16_t program_start = 0x8000;
constexpr std::uint16_t console_out = 0xF001;
constexpr std::size_t max_ticks = 20000;

std::vector<std::uint8_t> read_binary(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("could not open assembled binary");
    }

    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };
}

void expect_string(const std::string& actual, const std::string& expected) {
    if (actual != expected) {
        throw std::runtime_error("unexpected assembled program output");
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("usage: asm65_console_test <program.bin>");
    }

    const std::vector<std::uint8_t> program = read_binary(argv[1]);
    if (program.empty()) {
        throw std::runtime_error("assembled binary is empty");
    }

    std::string output;
    cpp65::PutCharBus bus(
        console_out,
        [&](std::uint8_t value) {
            output.push_back(static_cast<char>(value));
        }
    );
    cpp65::CPU cpu(bus);

    bus.load(program_start, program);
    bus.reset();

    for (std::size_t i = 0; i < max_ticks; ++i) {
        cpu.tick();
    }

    expect_string(output, "ASM65 says hello from cpp65!\n");
    return 0;
}
