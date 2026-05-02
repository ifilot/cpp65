// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <initializer_list>
#include <stdexcept>

namespace {

struct TestMachine {
    cpp65::RamBus bus;
    cpp65::CPU cpu;

    explicit TestMachine(cpp65::CPUModel model = cpp65::CPUModel::wdc65c02)
        : cpu(bus, model) {}

    void set_reset_vector(std::uint16_t addr) {
        bus.set_reset_vector(addr);
    }

    void set_nmi_vector(std::uint16_t addr) {
        bus.set_nmi_vector(addr);
    }

    void set_irq_vector(std::uint16_t addr) {
        bus.set_irq_vector(addr);
    }

    void load(std::uint16_t addr, std::initializer_list<std::uint8_t> bytes) {
        bus.load(addr, bytes);
    }

    void run_ticks(std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            cpu.tick();
        }
    }
};

void expect_eq(std::uint8_t actual, std::uint8_t expected) {
    if (actual != expected) {
        throw std::runtime_error("unexpected test value");
    }
}

void expect_flags(std::uint8_t actual, std::uint8_t expected) {
    constexpr std::uint8_t n_v_z_c = 0xC3;

    if ((actual & n_v_z_c) != expected) {
        throw std::runtime_error("unexpected processor flags");
    }
}

void test_load_store_and_increment() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA9, 0x42,       // LDA #$42
        0x8D, 0x00, 0x02, // STA $0200
        0xA2, 0x05,       // LDX #$05
        0xE8,             // INX
        0x8E, 0x01, 0x02, // STX $0201
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 4 + 2 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x0200), 0x42);
    expect_eq(machine.bus.peek(0x0201), 0x06);
}

void test_branch_taken_skips_instruction() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA9, 0x00,       // LDA #$00
        0xF0, 0x03,       // BEQ +3
        0xA9, 0x01,       // LDA #$01
        0xDB,             // STP
        0xA9, 0x02,       // LDA #$02
        0x8D, 0x02, 0x02, // STA $0202
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 3 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x0202), 0x02);
}

void test_branch_not_taken_falls_through() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA9, 0x01,       // LDA #$01
        0xF0, 0x05,       // BEQ +5
        0xA9, 0x7A,       // LDA #$7A
        0x8D, 0x03, 0x02, // STA $0203
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x0203), 0x7A);
}

void test_add_subtract_and_flags_visible_through_branching() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA9, 0x10,       // LDA #$10
        0x69, 0x05,       // ADC #$05
        0x8D, 0x04, 0x02, // STA $0204
        0x38,             // SEC
        0xE9, 0x15,       // SBC #$15
        0xF0, 0x03,       // BEQ +3
        0xA9, 0xFF,       // LDA #$FF
        0xDB,             // STP
        0xA9, 0x01,       // LDA #$01
        0x8D, 0x05, 0x02, // STA $0205
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 4 + 2 + 2 + 3 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x0204), 0x15);
    expect_eq(machine.bus.peek(0x0205), 0x01);
}

void test_subroutine_call_and_return() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0x20, 0x08, 0x80, // JSR $8008
        0x8D, 0x06, 0x02, // STA $0206
        0xDB,             // STP
        0xEA,             // NOP padding
        0xA9, 0x37,       // LDA #$37
        0x60,             // RTS
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(6 + 2 + 6 + 4 + 3);

    expect_eq(machine.bus.peek(0x0206), 0x37);
}

void test_zero_page_indexed_addressing_wraps() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA2, 0x01, // LDX #$01
        0xB5, 0xFF, // LDA $FF,X
        0x8D, 0x07, 0x02, // STA $0207
        0xDB, // STP
    });
    machine.bus.poke(0x0000, 0xAB);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 4 + 4 + 3);

    expect_eq(machine.bus.peek(0x0207), 0xAB);
}

void test_indexed_indirect_addressing_wraps_in_zero_page() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA2, 0x01, // LDX #$01
        0xA1, 0xFE, // LDA ($FE,X)
        0x8D, 0x08, 0x02, // STA $0208
        0xDB, // STP
    });
    machine.bus.poke(0x00FF, 0x00);
    machine.bus.poke(0x0000, 0x90);
    machine.bus.poke(0x9000, 0xCD);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 6 + 4 + 3);

    expect_eq(machine.bus.peek(0x0208), 0xCD);
}

void test_zero_page_indirect_addressing_wraps_pointer_high_byte() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xB2, 0xFF, // LDA ($FF)
        0x8D, 0x09, 0x02, // STA $0209
        0xDB, // STP
    });
    machine.bus.poke(0x00FF, 0x34);
    machine.bus.poke(0x0000, 0x12);
    machine.bus.poke(0x1234, 0xEF);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(5 + 4 + 3);

    expect_eq(machine.bus.peek(0x0209), 0xEF);
}

void test_indirect_indexed_addressing_wraps_pointer_high_byte() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xA0, 0x01, // LDY #$01
        0xB1, 0xFF, // LDA ($FF),Y
        0x8D, 0x0A, 0x02, // STA $020A
        0xDB, // STP
    });
    machine.bus.poke(0x00FF, 0x00);
    machine.bus.poke(0x0000, 0x90);
    machine.bus.poke(0x9001, 0x77);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 5 + 4 + 3);

    expect_eq(machine.bus.peek(0x020A), 0x77);
}

void test_nmi_enters_nmi_vector_handler() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.set_nmi_vector(0x9000);
    machine.load(0x8000, {
        0xEA, // NOP
        0xDB, // STP
    });
    machine.load(0x9000, {
        0xA9, 0x99, // LDA #$99
        0x8D, 0x0B, 0x02, // STA $020B
        0xDB, // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.bus.nmi();
    machine.cpu.tick();
    machine.run_ticks(8 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x020B), 0x99);
}

void test_irq_enters_irq_vector_when_interrupts_are_enabled() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.set_irq_vector(0x9000);
    machine.load(0x8000, {
        0x58, // CLI
        0xEA, // NOP
    });
    machine.load(0x9000, {
        0xA9, 0x55, // LDA #$55
        0x8D, 0x0C, 0x02, // STA $020C
        0xDB, // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2);
    machine.bus.irq();
    machine.cpu.tick();
    machine.run_ticks(7 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x020C), 0x55);
}

void test_irq_is_ignored_when_interrupts_are_disabled() {
    TestMachine machine;

    machine.set_reset_vector(0x8000);
    machine.set_irq_vector(0x9000);
    machine.load(0x8000, {
        0xA9, 0x11, // LDA #$11
        0x8D, 0x0D, 0x02, // STA $020D
        0xDB, // STP
    });
    machine.load(0x9000, {
        0xA9, 0xEE, // LDA #$EE
        0x8D, 0x0D, 0x02, // STA $020D
        0xDB, // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.bus.irq();
    machine.cpu.tick();
    machine.run_ticks(2 + 4 + 3);

    expect_eq(machine.bus.peek(0x020D), 0x11);
}

void test_nmos6502_indirect_jmp_wraps_high_byte_on_page_boundary() {
    TestMachine machine(cpp65::CPUModel::nmos6502);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0x6C, 0xFF, 0x12, // JMP ($12FF)
    });
    machine.load(0x9000, {
        0xA9, 0x42,       // LDA #$42
        0x8D, 0x0E, 0x02, // STA $020E
    });
    machine.bus.poke(0x12FF, 0x00);
    machine.bus.poke(0x1200, 0x90);
    machine.bus.poke(0x1300, 0x91);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(5 + 2 + 4);

    expect_eq(machine.bus.peek(0x020E), 0x42);
}

void test_wdc65c02_indirect_jmp_reads_high_byte_from_next_address() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0x6C, 0xFF, 0x12, // JMP ($12FF)
    });
    machine.load(0x9100, {
        0xA9, 0x43,       // LDA #$43
        0x8D, 0x0F, 0x02, // STA $020F
        0xDB,             // STP
    });
    machine.bus.poke(0x12FF, 0x00);
    machine.bus.poke(0x1200, 0x90);
    machine.bus.poke(0x1300, 0x91);

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(6 + 2 + 4 + 3);

    expect_eq(machine.bus.peek(0x020F), 0x43);
}

void test_wdc65c02_only_opcode_stz_is_not_available_on_nmos6502() {
    TestMachine nmos_machine(cpp65::CPUModel::nmos6502);
    TestMachine cmos_machine(cpp65::CPUModel::wdc65c02);

    const std::initializer_list<std::uint8_t> program = {
        0x64, 0x10, // STZ $10 on 65C02; unsupported opcode on NMOS 6502
        0xDB,       // STP
    };

    nmos_machine.set_reset_vector(0x8000);
    nmos_machine.bus.poke(0x0010, 0xAA);
    nmos_machine.load(0x8000, program);
    nmos_machine.bus.reset();
    nmos_machine.cpu.tick();
    nmos_machine.run_ticks(16);

    cmos_machine.set_reset_vector(0x8000);
    cmos_machine.bus.poke(0x0010, 0xAA);
    cmos_machine.load(0x8000, program);
    cmos_machine.bus.reset();
    cmos_machine.cpu.tick();
    cmos_machine.run_ticks(3 + 3);

    expect_eq(nmos_machine.bus.peek(0x0010), 0xAA);
    expect_eq(cmos_machine.bus.peek(0x0010), 0x00);
}

void test_decimal_adc_sets_nmos6502_binary_zero_flag_behavior() {
    TestMachine machine(cpp65::CPUModel::nmos6502);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x38,             // SEC
        0xA9, 0x89,       // LDA #$89
        0x69, 0x76,       // ADC #$76
        0x8D, 0x10, 0x02, // STA $0210
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x11, 0x02, // STA $0211
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 2 + 4 + 3 + 4 + 4);

    expect_eq(machine.bus.peek(0x0210), 0x66);
    expect_flags(machine.bus.peek(0x0211), 0x03);
}

void test_decimal_adc_sets_nmos6502_intermediate_negative_and_overflow_flags() {
    TestMachine machine(cpp65::CPUModel::nmos6502);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x18,             // CLC
        0xA9, 0x24,       // LDA #$24
        0x69, 0x56,       // ADC #$56
        0x8D, 0x18, 0x02, // STA $0218
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x19, 0x02, // STA $0219
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 2 + 4 + 3 + 4 + 4);

    expect_eq(machine.bus.peek(0x0218), 0x80);
    expect_flags(machine.bus.peek(0x0219), 0xC0);
}

void test_decimal_adc_sets_wdc65c02_adjusted_negative_and_overflow_flags() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x18,             // CLC
        0xA9, 0x24,       // LDA #$24
        0x69, 0x56,       // ADC #$56
        0x8D, 0x1A, 0x02, // STA $021A
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x1B, 0x02, // STA $021B
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 3 + 4 + 3 + 4 + 4 + 3);

    expect_eq(machine.bus.peek(0x021A), 0x80);
    expect_flags(machine.bus.peek(0x021B), 0xC0);
}

void test_decimal_adc_wdc65c02_takes_one_extra_cycle() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x18,             // CLC
        0xA9, 0x20,       // LDA #$20
        0x69, 0x22,       // ADC #$22
        0x8D, 0x1C, 0x02, // STA $021C
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 2);
    expect_eq(machine.bus.peek(0x021C), 0x00);

    machine.run_ticks(1);
    expect_eq(machine.bus.peek(0x021C), 0x00);

    machine.run_ticks(1);
    expect_eq(machine.bus.peek(0x021C), 0x42);
}

void test_decimal_adc_nmos6502_reaches_next_instruction_without_extra_cycle() {
    TestMachine machine(cpp65::CPUModel::nmos6502);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x18,             // CLC
        0xA9, 0x20,       // LDA #$20
        0x69, 0x22,       // ADC #$22
        0x8D, 0x1D, 0x02, // STA $021D
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 2 + 1);

    expect_eq(machine.bus.peek(0x021D), 0x42);
}

void test_decimal_adc_sets_wdc65c02_adjusted_zero_flag_behavior() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x38,             // SEC
        0xA9, 0x89,       // LDA #$89
        0x69, 0x76,       // ADC #$76
        0x8D, 0x12, 0x02, // STA $0212
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x13, 0x02, // STA $0213
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 3 + 4 + 3 + 4 + 4 + 3);

    expect_eq(machine.bus.peek(0x0212), 0x66);
    expect_flags(machine.bus.peek(0x0213), 0x01);
}

void test_decimal_adc_carry_wraps_bcd_result() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x18,             // CLC
        0xA9, 0x99,       // LDA #$99
        0x69, 0x01,       // ADC #$01
        0x8D, 0x14, 0x02, // STA $0214
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x15, 0x02, // STA $0215
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 3 + 4 + 3 + 4 + 4 + 3);

    expect_eq(machine.bus.peek(0x0214), 0x00);
    expect_flags(machine.bus.peek(0x0215), 0x03);
}

void test_decimal_sbc_borrows_to_bcd_99() {
    TestMachine machine(cpp65::CPUModel::wdc65c02);

    machine.set_reset_vector(0x8000);
    machine.load(0x8000, {
        0xF8,             // SED
        0x38,             // SEC
        0xA9, 0x00,       // LDA #$00
        0xE9, 0x01,       // SBC #$01
        0x8D, 0x16, 0x02, // STA $0216
        0x08,             // PHP
        0x68,             // PLA
        0x8D, 0x17, 0x02, // STA $0217
        0xDB,             // STP
    });

    machine.bus.reset();
    machine.cpu.tick();
    machine.run_ticks(2 + 2 + 2 + 3 + 4 + 3 + 4 + 4 + 3);

    expect_eq(machine.bus.peek(0x0216), 0x99);
    expect_flags(machine.bus.peek(0x0217), 0x80);
}

} // namespace

int main() {
    test_load_store_and_increment();
    test_branch_taken_skips_instruction();
    test_branch_not_taken_falls_through();
    test_add_subtract_and_flags_visible_through_branching();
    test_subroutine_call_and_return();
    test_zero_page_indexed_addressing_wraps();
    test_indexed_indirect_addressing_wraps_in_zero_page();
    test_zero_page_indirect_addressing_wraps_pointer_high_byte();
    test_indirect_indexed_addressing_wraps_pointer_high_byte();
    test_nmi_enters_nmi_vector_handler();
    test_irq_enters_irq_vector_when_interrupts_are_enabled();
    test_irq_is_ignored_when_interrupts_are_disabled();
    test_nmos6502_indirect_jmp_wraps_high_byte_on_page_boundary();
    test_wdc65c02_indirect_jmp_reads_high_byte_from_next_address();
    test_wdc65c02_only_opcode_stz_is_not_available_on_nmos6502();
    test_decimal_adc_sets_nmos6502_binary_zero_flag_behavior();
    test_decimal_adc_sets_nmos6502_intermediate_negative_and_overflow_flags();
    test_decimal_adc_sets_wdc65c02_adjusted_zero_flag_behavior();
    test_decimal_adc_sets_wdc65c02_adjusted_negative_and_overflow_flags();
    test_decimal_adc_wdc65c02_takes_one_extra_cycle();
    test_decimal_adc_nmos6502_reaches_next_instruction_without_extra_cycle();
    test_decimal_adc_carry_wraps_bcd_result();
    test_decimal_sbc_borrows_to_bcd_99();
    return 0;
}
