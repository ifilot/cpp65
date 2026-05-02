#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <iostream>

namespace {

constexpr std::uint16_t reset_vector = 0xFFFC;
constexpr std::uint16_t program_start = 0x8000;
constexpr std::uint16_t console_out = 0xF001;

} // namespace

int main() {
    // This example treats $F001 as a memory-mapped output register. Whenever
    // the emulated CPU writes a byte there, the host application prints it.
    cpp65::PutCharBus bus(console_out);
    cpp65::CPU cpu(bus);

    // Point the reset vector at the in-memory program below.
    bus.set_reset_vector(program_start);

    // Program:
    //   LDA #'H' ; STA console_out
    //   LDA #'e' ; STA console_out
    //   ...
    //   STP
    //
    // The CPU does not know this is console output. That behavior comes from
    // the write callback above, which intercepts writes to console_out.
    const std::array<std::uint8_t, 31> program = {
        0xA9, 'H', 0x8D, 0x01, 0xF0, // LDA #'H' ; STA console_out
        0xA9, 'e', 0x8D, 0x01, 0xF0,
        0xA9, 'l', 0x8D, 0x01, 0xF0,
        0xA9, 'l', 0x8D, 0x01, 0xF0,
        0xA9, 'o', 0x8D, 0x01, 0xF0,
        0xA9, '\n', 0x8D, 0x01, 0xF0,
        0xDB,                         // STP
    };

    bus.load(program_start, { program.begin(), program.end() });

    // Run enough cycles to emit all characters and execute STP.
    bus.reset();
    cpu.tick();
    for (std::size_t i = 0; i < (2 + 4) * 6 + 3; ++i) {
        cpu.tick();
    }

    return 0;
}
