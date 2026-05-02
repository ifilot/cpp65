// SPDX-License-Identifier: LGPL-3.0-or-later
//
// cpp65 is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

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
    // The bus is where a larger emulator would connect RAM, ROM, and devices.
    cpp65::RamBus bus;
    cpp65::CPU cpu(bus);

    // Start execution at program_start after reset().
    bus.set_reset_vector(program_start);

    // Program:
    //   LDX #$03
    // loop:
    //   STX result_addr
    //   DEX
    //   BNE loop
    //   STP
    bus.load(program_start, {
        0xA2, 0x03,
        0x8E, static_cast<std::uint8_t>(result_addr & 0x00FF),
              static_cast<std::uint8_t>((result_addr >> 8) & 0x00FF),
        0xCA,
        0xD0, 0xFA,
        0xDB,
    });

    // Run the exact cycle budget for this tiny program. A real emulator front
    // end would usually call tick() from a machine clock loop instead.
    bus.reset();
    cpu.tick();
    for (std::size_t i = 0; i < 2 + ((4 + 2 + 3) * 2) + 4 + 2 + 2 + 3; ++i) {
        cpu.tick();
    }

    std::cout << "loop ended with memory[$0200] = " << static_cast<int>(bus.peek(result_addr)) << '\n';
    return 0;
}
