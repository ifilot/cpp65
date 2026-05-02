// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <iostream>

namespace {

constexpr std::uint16_t reset_vector = 0xFFFC;
constexpr std::uint16_t program_start = 0x8000;
constexpr std::uint16_t result_addr = 0x0200;

} // namespace

int main() {
    // The CPU is intentionally memory-agnostic. A bus owns memory and devices.
    cpp65::RamBus bus;
    cpp65::CPU cpu(bus);

    // The reset vector tells the CPU where execution starts after reset().
    bus.set_reset_vector(program_start);

    // Program:
    //   LDA #$42
    //   STA result_addr
    //   STP
    bus.load(program_start, {
        0xA9, 0x42,
        0x8D, static_cast<std::uint8_t>(result_addr & 0x00FF),
              static_cast<std::uint8_t>((result_addr >> 8) & 0x00FF),
        0xDB,
    });

    // tick() advances the CPU by exactly one clock cycle. Run enough cycles
    // for LDA immediate, STA absolute, and STP.
    bus.reset();
    cpu.tick();
    for (std::size_t i = 0; i < 2 + 4 + 3; ++i) {
        cpu.tick();
    }

    std::cout << "memory[$0200] = $" << std::hex << static_cast<int>(bus.peek(result_addr)) << '\n';
    return 0;
}
