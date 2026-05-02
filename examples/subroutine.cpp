#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <iostream>

namespace {

constexpr std::uint16_t reset_vector = 0xFFFC;
constexpr std::uint16_t program_start = 0x8000;
constexpr std::uint16_t subroutine_addr = 0x8008;
constexpr std::uint16_t result_addr = 0x0201;

} // namespace

int main() {
    // Minimal machine: a RAM bus plus a CPU connected to it.
    cpp65::RamBus bus;
    cpp65::CPU cpu(bus);

    // The CPU reads this vector during reset().
    bus.set_reset_vector(program_start);

    // Program:
    //   JSR subroutine_addr
    //   STA result_addr
    //   STP
    // subroutine:
    //   LDA #$37
    //   RTS
    bus.load(program_start, {
        0x20, static_cast<std::uint8_t>(subroutine_addr & 0x00FF),
              static_cast<std::uint8_t>((subroutine_addr >> 8) & 0x00FF),
        0x8D, static_cast<std::uint8_t>(result_addr & 0x00FF),
              static_cast<std::uint8_t>((result_addr >> 8) & 0x00FF),
        0xDB,
        0xEA,
        0xA9, 0x37,
        0x60,
    });

    // The stack is initialized by reset(), so JSR/RTS work without additional
    // setup in this small example.
    bus.reset();
    cpu.tick();
    for (std::size_t i = 0; i < 6 + 2 + 6 + 4 + 3; ++i) {
        cpu.tick();
    }

    std::cout << "subroutine returned $" << std::hex << static_cast<int>(bus.peek(result_addr)) << '\n';
    return 0;
}
