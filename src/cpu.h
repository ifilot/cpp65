// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#pragma once

#include "bus.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cpp65 {

/**
 * @brief Selects the CPU variant and its opcode timing/behavior table.
 */
enum class CPUModel {
    /**
     * @brief Original NMOS 6502 official instruction set.
     */
    nmos6502,

    /**
     * @brief Western Design Center 65C02 instruction set.
     */
    wdc65c02,
};

class CPU {
private:
    using Operation = std::uint8_t (CPU::*)();
    using AddressMode = std::uint8_t (CPU::*)();

    struct Instruction {
        const char* name;
        Operation operate;
        AddressMode addrmode;
        std::uint8_t cycles;
    };

    std::uint8_t reg_x = 0;
    std::uint8_t reg_y = 0;
    std::uint8_t reg_a = 0;
    std::uint8_t reg_f = 0;          // flag register
    std::uint8_t opcode = 0;
    std::uint8_t fetched = 0;
    std::uint16_t pc = 0;            // program counter
    std::uint16_t sp = 0;            // stack pointer
    std::uint16_t addr_abs = 0;
    std::uint16_t addr_rel = 0;
    std::size_t cycles = 0;
    bool stopped = false;
    bool waiting = false;

    CPUModel model;
    const std::array<Instruction, 256>* table;

    static constinit const std::array<Instruction, 256> nmos6502_table;
    static constinit const std::array<Instruction, 256> wdc65c02_table;

    Bus& bus;

public:
    /**
     * @brief Constructs a CPU connected to a bus with the selected CPU model.
     */
    explicit CPU(Bus& bus_, CPUModel model_ = CPUModel::wdc65c02);

    /**
     * @brief Advances the CPU by one clock cycle.
     */
    void tick();

private:
    enum Flag : std::uint8_t {
        C = 1 << 0,
        Z = 1 << 1,
        I = 1 << 2,
        D = 1 << 3,
        B = 1 << 4,
        U = 1 << 5,
        V = 1 << 6,
        N = 1 << 7,
    };

    /**
     * @brief Returns whether a processor status flag is set.
     */
    bool get_flag(Flag flag) const;

    /**
     * @brief Sets or clears a processor status flag.
     */
    void set_flag(Flag flag, bool value);

    /**
     * @brief Updates the zero and negative flags from a value.
     */
    void set_zn(std::uint8_t value);

    /**
     * @brief Resets CPU registers and loads the reset vector into the program counter.
     */
    void reset();

    /**
     * @brief Handles a non-maskable interrupt request.
     */
    void nmi();

    /**
     * @brief Handles a maskable interrupt request when interrupts are enabled.
     */
    void irq();

    /**
     * @brief Reads a byte from the connected bus.
     */
    std::uint8_t read(std::uint16_t addr);

    /**
     * @brief Writes a byte to the connected bus.
     */
    void write(std::uint16_t addr, std::uint8_t value);

    /**
     * @brief Fetches the operand for the current instruction.
     */
    std::uint8_t fetch();

    /**
     * @brief Reads a 16-bit little-endian value from memory.
     */
    std::uint16_t read16(std::uint16_t addr);

    /**
     * @brief Pushes a byte onto the hardware stack.
     */
    void push(std::uint8_t value);

    /**
     * @brief Pulls a byte from the hardware stack.
     */
    std::uint8_t pull();

    /**
     * @brief Applies a relative branch when the condition is true.
     */
    std::uint8_t branch_if(bool condition);

    /**
     * @brief Compares a register value with the fetched operand.
     */
    std::uint8_t compare(std::uint8_t lhs);

    /**
     * @brief Enters an interrupt handler through the given vector.
     */
    void interrupt(std::uint16_t vector, std::uint8_t cycle_count, bool set_break_flag);

    /**
     * @brief Extracts the bit index encoded in RMB/SMB/BBR/BBS opcodes.
     */
    std::uint8_t bit_index_from_opcode() const;

    /**
     * @brief Returns the dispatch entry for the current opcode.
     */
    const Instruction& current_instruction() const;


    /**
     * @brief Resolves implied addressing.
     */
    std::uint8_t imp();

    /**
     * @brief Resolves accumulator addressing.
     */
    std::uint8_t acc();

    /**
     * @brief Resolves immediate addressing.
     */
    std::uint8_t imm();

    /**
     * @brief Resolves zero-page addressing.
     */
    std::uint8_t zp0();

    /**
     * @brief Resolves zero-page indexed-by-X addressing.
     */
    std::uint8_t zpx();

    /**
     * @brief Resolves zero-page indexed-by-Y addressing.
     */
    std::uint8_t zpy();

    /**
     * @brief Resolves absolute addressing.
     */
    std::uint8_t abs();

    /**
     * @brief Resolves absolute indexed-by-X addressing.
     */
    std::uint8_t abx();

    /**
     * @brief Resolves absolute indexed-by-Y addressing.
     */
    std::uint8_t aby();

    /**
     * @brief Resolves absolute indirect addressing.
     */
    std::uint8_t ind();

    /**
     * @brief Resolves absolute indexed indirect addressing.
     */
    std::uint8_t iax();

    /**
     * @brief Resolves indexed indirect addressing.
     */
    std::uint8_t izx();

    /**
     * @brief Resolves indirect indexed addressing.
     */
    std::uint8_t izy();

    /**
     * @brief Resolves zero-page indirect addressing.
     */
    std::uint8_t zpi();

    /**
     * @brief Resolves relative branch addressing.
     */
    std::uint8_t rel();

    /**
     * @brief Resolves zero-page plus relative branch addressing.
     */
    std::uint8_t zpr();


    /**
     * @brief Executes Add with Carry.
     */
    std::uint8_t adc();

    /**
     * @brief Executes logical AND with the accumulator.
     */
    std::uint8_t and_();

    /**
     * @brief Executes arithmetic shift left.
     */
    std::uint8_t asl();

    /**
     * @brief Executes branch if bit reset.
     */
    std::uint8_t bbr();

    /**
     * @brief Executes branch if bit set.
     */
    std::uint8_t bbs();

    /**
     * @brief Executes branch if carry clear.
     */
    std::uint8_t bcc();

    /**
     * @brief Executes branch if carry set.
     */
    std::uint8_t bcs();

    /**
     * @brief Executes branch if equal.
     */
    std::uint8_t beq();

    /**
     * @brief Executes bit test.
     */
    std::uint8_t bit();

    /**
     * @brief Executes branch if minus.
     */
    std::uint8_t bmi();

    /**
     * @brief Executes branch if not equal.
     */
    std::uint8_t bne();

    /**
     * @brief Executes branch if plus.
     */
    std::uint8_t bpl();

    /**
     * @brief Executes unconditional relative branch.
     */
    std::uint8_t bra();

    /**
     * @brief Executes software interrupt.
     */
    std::uint8_t brk();

    /**
     * @brief Executes branch if overflow clear.
     */
    std::uint8_t bvc();

    /**
     * @brief Executes branch if overflow set.
     */
    std::uint8_t bvs();

    /**
     * @brief Clears the carry flag.
     */
    std::uint8_t clc();

    /**
     * @brief Clears the decimal mode flag.
     */
    std::uint8_t cld();

    /**
     * @brief Clears the interrupt disable flag.
     */
    std::uint8_t cli();

    /**
     * @brief Clears the overflow flag.
     */
    std::uint8_t clv();

    /**
     * @brief Compares the accumulator with memory.
     */
    std::uint8_t cmp();

    /**
     * @brief Compares the X register with memory.
     */
    std::uint8_t cpx();

    /**
     * @brief Compares the Y register with memory.
     */
    std::uint8_t cpy();

    /**
     * @brief Decrements memory or the accumulator.
     */
    std::uint8_t dec();

    /**
     * @brief Decrements the X register.
     */
    std::uint8_t dex();

    /**
     * @brief Decrements the Y register.
     */
    std::uint8_t dey();

    /**
     * @brief Executes exclusive OR with the accumulator.
     */
    std::uint8_t eor();

    /**
     * @brief Increments memory or the accumulator.
     */
    std::uint8_t inc();

    /**
     * @brief Increments the X register.
     */
    std::uint8_t inx();

    /**
     * @brief Increments the Y register.
     */
    std::uint8_t iny();

    /**
     * @brief Jumps to the effective address.
     */
    std::uint8_t jmp();

    /**
     * @brief Jumps to subroutine.
     */
    std::uint8_t jsr();

    /**
     * @brief Loads the accumulator.
     */
    std::uint8_t lda();

    /**
     * @brief Loads the X register.
     */
    std::uint8_t ldx();

    /**
     * @brief Loads the Y register.
     */
    std::uint8_t ldy();

    /**
     * @brief Executes logical shift right.
     */
    std::uint8_t lsr();

    /**
     * @brief Executes no operation.
     */
    std::uint8_t nop();

    /**
     * @brief Executes an unsupported or jammed opcode.
     */
    std::uint8_t kil();

    /**
     * @brief Executes logical OR with the accumulator.
     */
    std::uint8_t ora();

    /**
     * @brief Pushes the accumulator.
     */
    std::uint8_t pha();

    /**
     * @brief Pushes the processor status.
     */
    std::uint8_t php();

    /**
     * @brief Pushes the X register.
     */
    std::uint8_t phx();

    /**
     * @brief Pushes the Y register.
     */
    std::uint8_t phy();

    /**
     * @brief Pulls the accumulator.
     */
    std::uint8_t pla();

    /**
     * @brief Pulls the processor status.
     */
    std::uint8_t plp();

    /**
     * @brief Pulls the X register.
     */
    std::uint8_t plx();

    /**
     * @brief Pulls the Y register.
     */
    std::uint8_t ply();

    /**
     * @brief Resets a zero-page memory bit.
     */
    std::uint8_t rmb();

    /**
     * @brief Executes rotate left.
     */
    std::uint8_t rol();

    /**
     * @brief Executes rotate right.
     */
    std::uint8_t ror();

    /**
     * @brief Returns from interrupt.
     */
    std::uint8_t rti();

    /**
     * @brief Returns from subroutine.
     */
    std::uint8_t rts();

    /**
     * @brief Executes subtract with carry.
     */
    std::uint8_t sbc();

    /**
     * @brief Sets the carry flag.
     */
    std::uint8_t sec();

    /**
     * @brief Sets the decimal mode flag.
     */
    std::uint8_t sed();

    /**
     * @brief Sets the interrupt disable flag.
     */
    std::uint8_t sei();

    /**
     * @brief Sets a zero-page memory bit.
     */
    std::uint8_t smb();

    /**
     * @brief Stores the accumulator.
     */
    std::uint8_t sta();

    /**
     * @brief Stops the CPU.
     */
    std::uint8_t stp();

    /**
     * @brief Stores the X register.
     */
    std::uint8_t stx();

    /**
     * @brief Stores the Y register.
     */
    std::uint8_t sty();

    /**
     * @brief Stores zero.
     */
    std::uint8_t stz();

    /**
     * @brief Transfers the accumulator to X.
     */
    std::uint8_t tax();

    /**
     * @brief Transfers the accumulator to Y.
     */
    std::uint8_t tay();

    /**
     * @brief Tests and resets memory bits.
     */
    std::uint8_t trb();

    /**
     * @brief Tests and sets memory bits.
     */
    std::uint8_t tsb();

    /**
     * @brief Transfers the stack pointer to X.
     */
    std::uint8_t tsx();

    /**
     * @brief Transfers X to the accumulator.
     */
    std::uint8_t txa();

    /**
     * @brief Transfers X to the stack pointer.
     */
    std::uint8_t txs();

    /**
     * @brief Transfers Y to the accumulator.
     */
    std::uint8_t tya();

    /**
     * @brief Waits for an interrupt.
     */
    std::uint8_t wai();
};

} // namespace cpp65
