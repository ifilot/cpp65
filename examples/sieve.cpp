// SPDX-License-Identifier: LGPL-3.0-or-later
//
// cpp65 is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

#include "bus.h"
#include "cpu.h"

#include <array>
#include <cstdint>

namespace {

constexpr std::uint16_t program_start = 0x8000;
constexpr std::uint16_t console_out = 0xF001;

} // namespace

int main() {
    // PutCharBus is a regular 64 KiB RAM bus with one tiny device attached:
    // every write to $F001 is forwarded to std::putchar().
    cpp65::PutCharBus bus(console_out);
    cpp65::CPU cpu(bus);

    // The reset vector is the hardware entry point the CPU reads after reset.
    bus.set_reset_vector(program_start);

    // This 65C02 program computes prime numbers below 32 with a sieve.
    //
    // Memory used by the program:
    //   $0000      current sieve base p
    //   $0001      current candidate/multiple n
    //   $0200-$021F primality flags, indexed by the candidate number
    //   $F001      memory-mapped console output
    //
    // It prints the resulting primes as two-digit hexadecimal numbers:
    //   02 03 05 07 0B 0D 11 13 17 1D 1F
    const std::array<std::uint8_t, 138> program = {
        0xA9, 0x00,                         // LDA #$00
        0xA2, 0x00,                         // LDX #$00
        0x9D, 0x00, 0x02,                   // STA $0200,X
        0xE8,                               // INX
        0xE0, 0x20,                         // CPX #$20
        0xD0, 0xF8,                         // BNE clear_loop
        0xA9, 0x01,                         // LDA #$01
        0xA2, 0x02,                         // LDX #$02
        0x9D, 0x00, 0x02,                   // STA $0200,X
        0xE8,                               // INX
        0xE0, 0x20,                         // CPX #$20
        0xD0, 0xF8,                         // BNE init_loop
        0xA9, 0x02,                         // LDA #$02
        0x85, 0x00,                         // STA $00
        0xA6, 0x00,                         // LDX $00
        0xBD, 0x00, 0x02,                   // LDA $0200,X
        0xF0, 0x19,                         // BEQ next_p
        0xA5, 0x00,                         // LDA $00
        0x18,                               // CLC
        0x65, 0x00,                         // ADC $00
        0x85, 0x01,                         // STA $01
        0xA6, 0x01,                         // LDX $01
        0xA9, 0x00,                         // LDA #$00
        0x9D, 0x00, 0x02,                   // STA $0200,X
        0xA5, 0x01,                         // LDA $01
        0x18,                               // CLC
        0x65, 0x00,                         // ADC $00
        0x85, 0x01,                         // STA $01
        0xC9, 0x20,                         // CMP #$20
        0x90, 0xEE,                         // BCC mark_loop
        0xE6, 0x00,                         // INC $00
        0xA5, 0x00,                         // LDA $00
        0xC9, 0x06,                         // CMP #$06
        0x90, 0xD8,                         // BCC outer_loop
        0xA2, 0x02,                         // LDX #$02
        0x86, 0x01,                         // STX $01
        0xBD, 0x00, 0x02,                   // LDA $0200,X
        0xF0, 0x09,                         // BEQ print_next
        0x8A,                               // TXA
        0x20, 0x63, 0x80,                   // JSR print_hex_byte
        0xA9, 0x20,                         // LDA #' '
        0x8D, 0x01, 0xF0,                   // STA $F001
        0xA6, 0x01,                         // LDX $01
        0xE8,                               // INX
        0xE0, 0x20,                         // CPX #$20
        0xD0, 0xE9,                         // BNE print_loop
        0xA9, 0x0A,                         // LDA #'\n'
        0x8D, 0x01, 0xF0,                   // STA $F001
        0xDB,                               // STP
        0x48,                               // PHA
        0x4A,                               // LSR
        0x4A,                               // LSR
        0x4A,                               // LSR
        0x4A,                               // LSR
        0xAA,                               // TAX
        0xBD, 0x7A, 0x80,                   // LDA hex_digits,X
        0x8D, 0x01, 0xF0,                   // STA $F001
        0x68,                               // PLA
        0x29, 0x0F,                         // AND #$0F
        0xAA,                               // TAX
        0xBD, 0x7A, 0x80,                   // LDA hex_digits,X
        0x8D, 0x01, 0xF0,                   // STA $F001
        0x60,                               // RTS
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };

    bus.load(program_start, { program.begin(), program.end() });

    // Give the emulated CPU enough ticks to reset, run the sieve, print the
    // result, and stop. Front ends with a machine clock can call tick() from
    // their normal clock loop instead of using a fixed example budget.
    bus.reset();
    for (std::size_t i = 0; i < 40000; ++i) {
        cpu.tick();
    }

    return 0;
}
