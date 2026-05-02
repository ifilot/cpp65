#include "bus.h"
#include "cpu.h"

#include <cstdint>
#include <initializer_list>
#include <stdexcept>

namespace {

struct TestMachine {
    cpp65::RamBus bus;
    cpp65::CPU cpu;

    TestMachine()
        : cpu(bus) {}

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
    return 0;
}
