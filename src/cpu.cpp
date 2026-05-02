#include "cpu.h"

namespace cpp65 {

/**
 * @brief Constructs a CPU connected to a bus.
 */
CPU::CPU(Bus& bus_)
    : bus(bus_) {}

/**
 * @brief Advances the CPU by one clock cycle.
 */
void CPU::tick() {
    if (bus.consume_reset()) {
        reset();
        return;
    }

    if (cycles > 0) {
        --cycles;
        return;
    }

    if (bus.consume_nmi()) {
        nmi();
        return;
    }

    if (bus.irq_asserted() && !get_flag(I)) {
        irq();
        return;
    }

    if (stopped || waiting) {
        return;
    }

    opcode = read(pc++);
    
    const Instruction& inst = table[opcode];

    cycles = inst.cycles;

    const std::uint8_t extra1 = (this->*inst.addrmode)();
    const std::uint8_t extra2 = (this->*inst.operate)();

    cycles += (extra1 & extra2);

    if (cycles > 0) {
        --cycles;
    }
}

/**
 * @brief Resets CPU registers and loads the reset vector into the program counter.
 */
void CPU::reset() {
    // fetch PC from $FFFC-$FFFD
    pc = static_cast<std::uint16_t>(read(0xFFFC)) | (static_cast<std::uint16_t>(read(0xFFFD)) << 8);
    sp = 0x01FD;
    reg_a = 0;
    reg_x = 0;
    reg_y = 0;
    reg_f = U | I;
    addr_abs = 0;
    addr_rel = 0;
    fetched = 0;
    cycles = 0;
    stopped = false;
    waiting = false;
}

/**
 * @brief Handles a non-maskable interrupt request.
 */
void CPU::nmi() {
    interrupt(0xFFFA, 8, false);
}

/**
 * @brief Handles a maskable interrupt request when interrupts are enabled.
 */
void CPU::irq() {
    if (get_flag(I)) {
        return;
    }

    interrupt(0xFFFE, 7, false);
}

constinit const std::array<CPU::Instruction, 256> CPU::table = []() constexpr {
    std::array<Instruction, 256> table{};
    table.fill({ "NOP", &CPU::nop, &CPU::imp, 1 });

    // 0x00
    table[0x00] = { "BRK", &CPU::brk, &CPU::imm, 7 };
    table[0x01] = { "ORA", &CPU::ora, &CPU::izx, 6 };
    table[0x02] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0x03] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x04] = { "TSB", &CPU::tsb, &CPU::zp0, 5 };
    table[0x05] = { "ORA", &CPU::ora, &CPU::zp0, 3 };
    table[0x06] = { "ASL", &CPU::asl, &CPU::zp0, 5 };
    table[0x07] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x08] = { "PHP", &CPU::php, &CPU::imp, 3 };
    table[0x09] = { "ORA", &CPU::ora, &CPU::imm, 2 };
    table[0x0A] = { "ASL", &CPU::asl, &CPU::acc, 2 };
    table[0x0B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x0C] = { "TSB", &CPU::tsb, &CPU::abs, 6 };
    table[0x0D] = { "ORA", &CPU::ora, &CPU::abs, 4 };
    table[0x0E] = { "ASL", &CPU::asl, &CPU::abs, 6 };
    table[0x0F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x10
    table[0x10] = { "BPL", &CPU::bpl, &CPU::rel, 2 };
    table[0x11] = { "ORA", &CPU::ora, &CPU::izy, 5 };
    table[0x12] = { "ORA", &CPU::ora, &CPU::zpi, 5 };
    table[0x13] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x14] = { "TRB", &CPU::trb, &CPU::zp0, 5 };
    table[0x15] = { "ORA", &CPU::ora, &CPU::zpx, 4 };
    table[0x16] = { "ASL", &CPU::asl, &CPU::zpx, 6 };
    table[0x17] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x18] = { "CLC", &CPU::clc, &CPU::imp, 2 };
    table[0x19] = { "ORA", &CPU::ora, &CPU::aby, 4 };
    table[0x1A] = { "INC", &CPU::inc, &CPU::imp, 2 };
    table[0x1B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x1C] = { "TRB", &CPU::trb, &CPU::abs, 6 };
    table[0x1D] = { "ORA", &CPU::ora, &CPU::abx, 4 };
    table[0x1E] = { "ASL", &CPU::asl, &CPU::abx, 6 };
    table[0x1F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x20
    table[0x20] = { "JSR", &CPU::jsr, &CPU::abs, 6 };
    table[0x21] = { "AND", &CPU::and_, &CPU::izx, 6 };
    table[0x22] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0x23] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x24] = { "BIT", &CPU::bit, &CPU::zp0, 3 };
    table[0x25] = { "AND", &CPU::and_, &CPU::zp0, 3 };
    table[0x26] = { "ROL", &CPU::rol, &CPU::zp0, 5 };
    table[0x27] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x28] = { "PLP", &CPU::plp, &CPU::imp, 4 };
    table[0x29] = { "AND", &CPU::and_, &CPU::imm, 2 };
    table[0x2A] = { "ROL", &CPU::rol, &CPU::acc, 2 };
    table[0x2B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x2C] = { "BIT", &CPU::bit, &CPU::abs, 4 };
    table[0x2D] = { "AND", &CPU::and_, &CPU::abs, 4 };
    table[0x2E] = { "ROL", &CPU::rol, &CPU::abs, 6 };
    table[0x2F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x30
    table[0x30] = { "BMI", &CPU::bmi, &CPU::rel, 2 };
    table[0x31] = { "AND", &CPU::and_, &CPU::izy, 5 };
    table[0x32] = { "AND", &CPU::and_, &CPU::zpi, 5 };
    table[0x33] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x34] = { "BIT", &CPU::bit, &CPU::zpx, 4 };
    table[0x35] = { "AND", &CPU::and_, &CPU::zpx, 4 };
    table[0x36] = { "ROL", &CPU::rol, &CPU::zpx, 6 };
    table[0x37] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x38] = { "SEC", &CPU::sec, &CPU::imp, 2 };
    table[0x39] = { "AND", &CPU::and_, &CPU::aby, 4 };
    table[0x3A] = { "DEC", &CPU::dec, &CPU::imp, 2 };
    table[0x3B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x3C] = { "BIT", &CPU::bit, &CPU::abx, 4 };
    table[0x3D] = { "AND", &CPU::and_, &CPU::abx, 4 };
    table[0x3E] = { "ROL", &CPU::rol, &CPU::abx, 6 };
    table[0x3F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x40
    table[0x40] = { "RTI", &CPU::rti, &CPU::imp, 6 };
    table[0x41] = { "EOR", &CPU::eor, &CPU::izx, 6 };
    table[0x42] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0x43] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x44] = { "NOP", &CPU::nop, &CPU::zp0, 3 };
    table[0x45] = { "EOR", &CPU::eor, &CPU::zp0, 3 };
    table[0x46] = { "LSR", &CPU::lsr, &CPU::zp0, 5 };
    table[0x47] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x48] = { "PHA", &CPU::pha, &CPU::imp, 3 };
    table[0x49] = { "EOR", &CPU::eor, &CPU::imm, 2 };
    table[0x4A] = { "LSR", &CPU::lsr, &CPU::acc, 2 };
    table[0x4B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x4C] = { "JMP", &CPU::jmp, &CPU::abs, 3 };
    table[0x4D] = { "EOR", &CPU::eor, &CPU::abs, 4 };
    table[0x4E] = { "LSR", &CPU::lsr, &CPU::abs, 6 };
    table[0x4F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x50
    table[0x50] = { "BVC", &CPU::bvc, &CPU::rel, 2 };
    table[0x51] = { "EOR", &CPU::eor, &CPU::izy, 5 };
    table[0x52] = { "EOR", &CPU::eor, &CPU::zpi, 5 };
    table[0x53] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x54] = { "NOP", &CPU::nop, &CPU::zpx, 4 };
    table[0x55] = { "EOR", &CPU::eor, &CPU::zpx, 4 };
    table[0x56] = { "LSR", &CPU::lsr, &CPU::zpx, 6 };
    table[0x57] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x58] = { "CLI", &CPU::cli, &CPU::imp, 2 };
    table[0x59] = { "EOR", &CPU::eor, &CPU::aby, 4 };
    table[0x5A] = { "PHY", &CPU::phy, &CPU::imp, 3 };
    table[0x5B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x5C] = { "NOP", &CPU::nop, &CPU::abs, 8 };
    table[0x5D] = { "EOR", &CPU::eor, &CPU::abx, 4 };
    table[0x5E] = { "LSR", &CPU::lsr, &CPU::abx, 6 };
    table[0x5F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x60
    table[0x60] = { "RTS", &CPU::rts, &CPU::imp, 6 };
    table[0x61] = { "ADC", &CPU::adc, &CPU::izx, 6 };
    table[0x62] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0x63] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x64] = { "STZ", &CPU::stz, &CPU::zp0, 3 };
    table[0x65] = { "ADC", &CPU::adc, &CPU::zp0, 3 };
    table[0x66] = { "ROR", &CPU::ror, &CPU::zp0, 5 };
    table[0x67] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x68] = { "PLA", &CPU::pla, &CPU::imp, 4 };
    table[0x69] = { "ADC", &CPU::adc, &CPU::imm, 2 };
    table[0x6A] = { "ROR", &CPU::ror, &CPU::acc, 2 };
    table[0x6B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x6C] = { "JMP", &CPU::jmp, &CPU::ind, 6 };
    table[0x6D] = { "ADC", &CPU::adc, &CPU::abs, 4 };
    table[0x6E] = { "ROR", &CPU::ror, &CPU::abs, 6 };
    table[0x6F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x70
    table[0x70] = { "BVS", &CPU::bvs, &CPU::rel, 2 };
    table[0x71] = { "ADC", &CPU::adc, &CPU::izy, 5 };
    table[0x72] = { "ADC", &CPU::adc, &CPU::zpi, 5 };
    table[0x73] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x74] = { "STZ", &CPU::stz, &CPU::zpx, 4 };
    table[0x75] = { "ADC", &CPU::adc, &CPU::zpx, 4 };
    table[0x76] = { "ROR", &CPU::ror, &CPU::zpx, 6 };
    table[0x77] = { "RMB", &CPU::rmb, &CPU::zp0, 5 };
    table[0x78] = { "SEI", &CPU::sei, &CPU::imp, 2 };
    table[0x79] = { "ADC", &CPU::adc, &CPU::aby, 4 };
    table[0x7A] = { "PLY", &CPU::ply, &CPU::imp, 4 };
    table[0x7B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x7C] = { "JMP", &CPU::jmp, &CPU::iax, 6 };
    table[0x7D] = { "ADC", &CPU::adc, &CPU::abx, 4 };
    table[0x7E] = { "ROR", &CPU::ror, &CPU::abx, 6 };
    table[0x7F] = { "BBR", &CPU::bbr, &CPU::zpr, 5 };

    // 0x80
    table[0x80] = { "BRA", &CPU::bra, &CPU::rel, 3 };
    table[0x81] = { "STA", &CPU::sta, &CPU::izx, 6 };
    table[0x82] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0x83] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x84] = { "STY", &CPU::sty, &CPU::zp0, 3 };
    table[0x85] = { "STA", &CPU::sta, &CPU::zp0, 3 };
    table[0x86] = { "STX", &CPU::stx, &CPU::zp0, 3 };
    table[0x87] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0x88] = { "DEY", &CPU::dey, &CPU::imp, 2 };
    table[0x89] = { "BIT", &CPU::bit, &CPU::imm, 2 };
    table[0x8A] = { "TXA", &CPU::txa, &CPU::imp, 2 };
    table[0x8B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x8C] = { "STY", &CPU::sty, &CPU::abs, 4 };
    table[0x8D] = { "STA", &CPU::sta, &CPU::abs, 4 };
    table[0x8E] = { "STX", &CPU::stx, &CPU::abs, 4 };
    table[0x8F] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0x90
    table[0x90] = { "BCC", &CPU::bcc, &CPU::rel, 2 };
    table[0x91] = { "STA", &CPU::sta, &CPU::izy, 6 };
    table[0x92] = { "STA", &CPU::sta, &CPU::zpi, 5 };
    table[0x93] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x94] = { "STY", &CPU::sty, &CPU::zpx, 4 };
    table[0x95] = { "STA", &CPU::sta, &CPU::zpx, 4 };
    table[0x96] = { "STX", &CPU::stx, &CPU::zpy, 4 };
    table[0x97] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0x98] = { "TYA", &CPU::tya, &CPU::imp, 2 };
    table[0x99] = { "STA", &CPU::sta, &CPU::aby, 5 };
    table[0x9A] = { "TXS", &CPU::txs, &CPU::imp, 2 };
    table[0x9B] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0x9C] = { "STZ", &CPU::stz, &CPU::abs, 4 };
    table[0x9D] = { "STA", &CPU::sta, &CPU::abx, 5 };
    table[0x9E] = { "STZ", &CPU::stz, &CPU::abx, 5 };
    table[0x9F] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xA0
    table[0xA0] = { "LDY", &CPU::ldy, &CPU::imm, 2 };
    table[0xA1] = { "LDA", &CPU::lda, &CPU::izx, 6 };
    table[0xA2] = { "LDX", &CPU::ldx, &CPU::imm, 2 };
    table[0xA3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xA4] = { "LDY", &CPU::ldy, &CPU::zp0, 3 };
    table[0xA5] = { "LDA", &CPU::lda, &CPU::zp0, 3 };
    table[0xA6] = { "LDX", &CPU::ldx, &CPU::zp0, 3 };
    table[0xA7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xA8] = { "TAY", &CPU::tay, &CPU::imp, 2 };
    table[0xA9] = { "LDA", &CPU::lda, &CPU::imm, 2 };
    table[0xAA] = { "TAX", &CPU::tax, &CPU::imp, 2 };
    table[0xAB] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xAC] = { "LDY", &CPU::ldy, &CPU::abs, 4 };
    table[0xAD] = { "LDA", &CPU::lda, &CPU::abs, 4 };
    table[0xAE] = { "LDX", &CPU::ldx, &CPU::abs, 4 };
    table[0xAF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xB0
    table[0xB0] = { "BCS", &CPU::bcs, &CPU::rel, 2 };
    table[0xB1] = { "LDA", &CPU::lda, &CPU::izy, 5 };
    table[0xB2] = { "LDA", &CPU::lda, &CPU::zpi, 5 };
    table[0xB3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xB4] = { "LDY", &CPU::ldy, &CPU::zpx, 4 };
    table[0xB5] = { "LDA", &CPU::lda, &CPU::zpx, 4 };
    table[0xB6] = { "LDX", &CPU::ldx, &CPU::zpy, 4 };
    table[0xB7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xB8] = { "CLV", &CPU::clv, &CPU::imp, 2 };
    table[0xB9] = { "LDA", &CPU::lda, &CPU::aby, 4 };
    table[0xBA] = { "TSX", &CPU::tsx, &CPU::imp, 2 };
    table[0xBB] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xBC] = { "LDY", &CPU::ldy, &CPU::abx, 4 };
    table[0xBD] = { "LDA", &CPU::lda, &CPU::abx, 4 };
    table[0xBE] = { "LDX", &CPU::ldx, &CPU::aby, 4 };
    table[0xBF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xC0
    table[0xC0] = { "CPY", &CPU::cpy, &CPU::imm, 2 };
    table[0xC1] = { "CMP", &CPU::cmp, &CPU::izx, 6 };
    table[0xC2] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0xC3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xC4] = { "CPY", &CPU::cpy, &CPU::zp0, 3 };
    table[0xC5] = { "CMP", &CPU::cmp, &CPU::zp0, 3 };
    table[0xC6] = { "DEC", &CPU::dec, &CPU::zp0, 5 };
    table[0xC7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xC8] = { "INY", &CPU::iny, &CPU::imp, 2 };
    table[0xC9] = { "CMP", &CPU::cmp, &CPU::imm, 2 };
    table[0xCA] = { "DEX", &CPU::dex, &CPU::imp, 2 };
    table[0xCB] = { "WAI", &CPU::wai, &CPU::imp, 3 };
    table[0xCC] = { "CPY", &CPU::cpy, &CPU::abs, 4 };
    table[0xCD] = { "CMP", &CPU::cmp, &CPU::abs, 4 };
    table[0xCE] = { "DEC", &CPU::dec, &CPU::abs, 6 };
    table[0xCF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xD0
    table[0xD0] = { "BNE", &CPU::bne, &CPU::rel, 2 };
    table[0xD1] = { "CMP", &CPU::cmp, &CPU::izy, 5 };
    table[0xD2] = { "CMP", &CPU::cmp, &CPU::zpi, 5 };
    table[0xD3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xD4] = { "NOP", &CPU::nop, &CPU::zpx, 4 };
    table[0xD5] = { "CMP", &CPU::cmp, &CPU::zpx, 4 };
    table[0xD6] = { "DEC", &CPU::dec, &CPU::zpx, 6 };
    table[0xD7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xD8] = { "CLD", &CPU::cld, &CPU::imp, 2 };
    table[0xD9] = { "CMP", &CPU::cmp, &CPU::aby, 4 };
    table[0xDA] = { "PHX", &CPU::phx, &CPU::imp, 3 };
    table[0xDB] = { "STP", &CPU::stp, &CPU::imp, 3 };
    table[0xDC] = { "NOP", &CPU::nop, &CPU::abs, 4 };
    table[0xDD] = { "CMP", &CPU::cmp, &CPU::abx, 4 };
    table[0xDE] = { "DEC", &CPU::dec, &CPU::abx, 7 };
    table[0xDF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xE0
    table[0xE0] = { "CPX", &CPU::cpx, &CPU::imm, 2 };
    table[0xE1] = { "SBC", &CPU::sbc, &CPU::izx, 6 };
    table[0xE2] = { "NOP", &CPU::nop, &CPU::imm, 2 };
    table[0xE3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xE4] = { "CPX", &CPU::cpx, &CPU::zp0, 3 };
    table[0xE5] = { "SBC", &CPU::sbc, &CPU::zp0, 3 };
    table[0xE6] = { "INC", &CPU::inc, &CPU::zp0, 5 };
    table[0xE7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xE8] = { "INX", &CPU::inx, &CPU::imp, 2 };
    table[0xE9] = { "SBC", &CPU::sbc, &CPU::imm, 2 };
    table[0xEA] = { "NOP", &CPU::nop, &CPU::imp, 2 };
    table[0xEB] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xEC] = { "CPX", &CPU::cpx, &CPU::abs, 4 };
    table[0xED] = { "SBC", &CPU::sbc, &CPU::abs, 4 };
    table[0xEE] = { "INC", &CPU::inc, &CPU::abs, 6 };
    table[0xEF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    // 0xF0
    table[0xF0] = { "BEQ", &CPU::beq, &CPU::rel, 2 };
    table[0xF1] = { "SBC", &CPU::sbc, &CPU::izy, 5 };
    table[0xF2] = { "SBC", &CPU::sbc, &CPU::zpi, 5 };
    table[0xF3] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xF4] = { "NOP", &CPU::nop, &CPU::zpx, 4 };
    table[0xF5] = { "SBC", &CPU::sbc, &CPU::zpx, 4 };
    table[0xF6] = { "INC", &CPU::inc, &CPU::zpx, 6 };
    table[0xF7] = { "SMB", &CPU::smb, &CPU::zp0, 5 };
    table[0xF8] = { "SED", &CPU::sed, &CPU::imp, 2 };
    table[0xF9] = { "SBC", &CPU::sbc, &CPU::aby, 4 };
    table[0xFA] = { "PLX", &CPU::plx, &CPU::imp, 4 };
    table[0xFB] = { "NOP", &CPU::nop, &CPU::imp, 1 };
    table[0xFC] = { "NOP", &CPU::nop, &CPU::abs, 4 };
    table[0xFD] = { "SBC", &CPU::sbc, &CPU::abx, 4 };
    table[0xFE] = { "INC", &CPU::inc, &CPU::abx, 7 };
    table[0xFF] = { "BBS", &CPU::bbs, &CPU::zpr, 5 };

    return table;
}();

/**
 * @brief Extracts the bit index encoded in RMB/SMB/BBR/BBS opcodes.
 */
std::uint8_t CPU::bit_index_from_opcode() const {
    return (opcode >> 4) & 0x07;
}

/**
 * @brief Returns whether a processor status flag is set.
 */
bool CPU::get_flag(Flag flag) const {
    return (reg_f & flag) != 0;
}

/**
 * @brief Sets or clears a processor status flag.
 */
void CPU::set_flag(Flag flag, bool value) {
    if (value) {
        reg_f |= flag;
    } else {
        reg_f &= static_cast<std::uint8_t>(~flag);
    }
}

/**
 * @brief Updates the zero and negative flags from a value.
 */
void CPU::set_zn(std::uint8_t value) {
    set_flag(Z, value == 0);
    set_flag(N, (value & 0x80) != 0);
}

/**
 * @brief Reads a byte from the connected bus.
 */
std::uint8_t CPU::read(std::uint16_t addr) {
    return bus.read(addr);
}

/**
 * @brief Writes a byte to the connected bus.
 */
void CPU::write(std::uint16_t addr, std::uint8_t value) {
    bus.write(addr, value);
}

/**
 * @brief Fetches the operand for the current instruction.
 */
std::uint8_t CPU::fetch() {
    if (table[opcode].addrmode == &CPU::acc || table[opcode].addrmode == &CPU::imp) {
        fetched = reg_a;
    } else {
        fetched = read(addr_abs);
    }

    return fetched;
}

/**
 * @brief Reads a 16-bit little-endian value from memory.
 */
std::uint16_t CPU::read16(std::uint16_t addr) {
    const std::uint16_t lo = read(addr);
    const std::uint16_t hi = read(static_cast<std::uint16_t>(addr + 1));
    return static_cast<std::uint16_t>(lo | (hi << 8));
}

/**
 * @brief Pushes a byte onto the hardware stack.
 */
void CPU::push(std::uint8_t value) {
    write(sp--, value);
    sp = static_cast<std::uint16_t>(0x0100 | (sp & 0x00FF));
}

/**
 * @brief Pulls a byte from the hardware stack.
 */
std::uint8_t CPU::pull() {
    sp = static_cast<std::uint16_t>(0x0100 | ((sp + 1) & 0x00FF));
    return read(sp);
}

/**
 * @brief Applies a relative branch when the condition is true.
 */
std::uint8_t CPU::branch_if(bool condition) {
    if (!condition) {
        return 0;
    }

    ++cycles;
    const std::uint16_t old_pc = pc;
    pc = static_cast<std::uint16_t>(pc + addr_rel);

    if ((old_pc & 0xFF00) != (pc & 0xFF00)) {
        ++cycles;
    }

    return 0;
}

/**
 * @brief Compares a register value with the fetched operand.
 */
std::uint8_t CPU::compare(std::uint8_t lhs) {
    const std::uint8_t rhs = fetch();
    const std::uint16_t temp = static_cast<std::uint16_t>(lhs) - rhs;
    set_flag(C, lhs >= rhs);
    set_zn(static_cast<std::uint8_t>(temp));
    return 1;
}

/**
 * @brief Enters an interrupt handler through the given vector.
 */
void CPU::interrupt(std::uint16_t vector, std::uint8_t cycle_count, bool set_break_flag) {
    push(static_cast<std::uint8_t>((pc >> 8) & 0xFF));
    push(static_cast<std::uint8_t>(pc & 0xFF));
    const std::uint8_t pushed_status = static_cast<std::uint8_t>((reg_f & ~B) | U | (set_break_flag ? B : 0));
    push(pushed_status);
    set_flag(I, true);
    pc = read16(vector);
    cycles = cycle_count > 0 ? cycle_count - 1 : 0;
    waiting = false;
}

/**
 * @brief Resolves implied addressing.
 */
std::uint8_t CPU::imp() {
    fetched = reg_a;
    return 0;
}

/**
 * @brief Resolves accumulator addressing.
 */
std::uint8_t CPU::acc() {
    fetched = reg_a;
    return 0;
}

/**
 * @brief Resolves immediate addressing.
 */
std::uint8_t CPU::imm() {
    addr_abs = pc++;
    return 0;
}

/**
 * @brief Resolves zero-page addressing.
 */
std::uint8_t CPU::zp0() {
    addr_abs = read(pc++);
    return 0;
}

/**
 * @brief Resolves zero-page indexed-by-X addressing.
 */
std::uint8_t CPU::zpx() {
    addr_abs = static_cast<std::uint8_t>(read(pc++) + reg_x);
    return 0;
}

/**
 * @brief Resolves zero-page indexed-by-Y addressing.
 */
std::uint8_t CPU::zpy() {
    addr_abs = static_cast<std::uint8_t>(read(pc++) + reg_y);
    return 0;
}

/**
 * @brief Resolves absolute addressing.
 */
std::uint8_t CPU::abs() {
    addr_abs = read16(pc);
    pc += 2;
    return 0;
}

/**
 * @brief Resolves absolute indexed-by-X addressing.
 */
std::uint8_t CPU::abx() {
    const std::uint16_t base = read16(pc);
    pc += 2;
    addr_abs = static_cast<std::uint16_t>(base + reg_x);
    return (addr_abs & 0xFF00) != (base & 0xFF00);
}

/**
 * @brief Resolves absolute indexed-by-Y addressing.
 */
std::uint8_t CPU::aby() {
    const std::uint16_t base = read16(pc);
    pc += 2;
    addr_abs = static_cast<std::uint16_t>(base + reg_y);
    return (addr_abs & 0xFF00) != (base & 0xFF00);
}

/**
 * @brief Resolves absolute indirect addressing.
 */
std::uint8_t CPU::ind() {
    const std::uint16_t ptr = read16(pc);
    pc += 2;
    addr_abs = read16(ptr);
    return 0;
}

/**
 * @brief Resolves absolute indexed indirect addressing.
 */
std::uint8_t CPU::iax() {
    const std::uint16_t ptr = static_cast<std::uint16_t>(read16(pc) + reg_x);
    pc += 2;
    addr_abs = read16(ptr);
    return 0;
}

/**
 * @brief Resolves indexed indirect addressing.
 */
std::uint8_t CPU::izx() {
    const std::uint8_t zp = static_cast<std::uint8_t>(read(pc++) + reg_x);
    const std::uint16_t lo = read(zp);
    const std::uint16_t hi = read(static_cast<std::uint8_t>(zp + 1));
    addr_abs = static_cast<std::uint16_t>(lo | (hi << 8));
    return 0;
}

/**
 * @brief Resolves indirect indexed addressing.
 */
std::uint8_t CPU::izy() {
    const std::uint8_t zp = read(pc++);
    const std::uint16_t lo = read(zp);
    const std::uint16_t hi = read(static_cast<std::uint8_t>(zp + 1));
    const std::uint16_t base = static_cast<std::uint16_t>(lo | (hi << 8));
    addr_abs = static_cast<std::uint16_t>(base + reg_y);
    return (addr_abs & 0xFF00) != (base & 0xFF00);
}

/**
 * @brief Resolves zero-page indirect addressing.
 */
std::uint8_t CPU::zpi() {
    const std::uint8_t zp = read(pc++);
    const std::uint16_t lo = read(zp);
    const std::uint16_t hi = read(static_cast<std::uint8_t>(zp + 1));
    addr_abs = static_cast<std::uint16_t>(lo | (hi << 8));
    return 0;
}

/**
 * @brief Resolves relative branch addressing.
 */
std::uint8_t CPU::rel() {
    addr_rel = read(pc++);
    if (addr_rel & 0x80) {
        addr_rel |= 0xFF00;
    }
    return 0;
}

/**
 * @brief Resolves zero-page plus relative branch addressing.
 */
std::uint8_t CPU::zpr() {
    addr_abs = read(pc++);
    addr_rel = read(pc++);
    if (addr_rel & 0x80) {
        addr_rel |= 0xFF00;
    }
    return 0;
}

/**
 * @brief Executes Add with Carry.
 */
std::uint8_t CPU::adc() {
    const std::uint8_t value = fetch();
    const std::uint16_t sum = static_cast<std::uint16_t>(reg_a) + value + (get_flag(C) ? 1 : 0);
    const std::uint8_t result = static_cast<std::uint8_t>(sum);
    set_flag(C, sum > 0xFF);
    set_flag(V, (~(reg_a ^ value) & (reg_a ^ result) & 0x80) != 0);
    reg_a = result;
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Executes logical AND with the accumulator.
 */
std::uint8_t CPU::and_() {
    reg_a &= fetch();
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Executes arithmetic shift left.
 */
std::uint8_t CPU::asl() {
    const std::uint8_t value = fetch();
    const std::uint16_t result = static_cast<std::uint16_t>(value) << 1;
    set_flag(C, (result & 0x100) != 0);
    const std::uint8_t out = static_cast<std::uint8_t>(result);
    set_zn(out);
    if (table[opcode].addrmode == &CPU::acc) {
        reg_a = out;
    } else {
        write(addr_abs, out);
    }
    return 0;
}

/**
 * @brief Executes branch if bit reset.
 */
std::uint8_t CPU::bbr() {
    return branch_if((read(addr_abs) & (1 << bit_index_from_opcode())) == 0);
}

/**
 * @brief Executes branch if bit set.
 */
std::uint8_t CPU::bbs() {
    return branch_if((read(addr_abs) & (1 << bit_index_from_opcode())) != 0);
}

/**
 * @brief Executes branch if carry clear.
 */
std::uint8_t CPU::bcc() {
    return branch_if(!get_flag(C));
}

/**
 * @brief Executes branch if carry set.
 */
std::uint8_t CPU::bcs() {
    return branch_if(get_flag(C));
}

/**
 * @brief Executes branch if equal.
 */
std::uint8_t CPU::beq() {
    return branch_if(get_flag(Z));
}

/**
 * @brief Executes branch if minus.
 */
std::uint8_t CPU::bmi() {
    return branch_if(get_flag(N));
}

/**
 * @brief Executes branch if not equal.
 */
std::uint8_t CPU::bne() {
    return branch_if(!get_flag(Z));
}

/**
 * @brief Executes branch if plus.
 */
std::uint8_t CPU::bpl() {
    return branch_if(!get_flag(N));
}

/**
 * @brief Executes unconditional relative branch.
 */
std::uint8_t CPU::bra() {
    return branch_if(true);
}

/**
 * @brief Executes branch if overflow clear.
 */
std::uint8_t CPU::bvc() {
    return branch_if(!get_flag(V));
}

/**
 * @brief Executes branch if overflow set.
 */
std::uint8_t CPU::bvs() {
    return branch_if(get_flag(V));
}

/**
 * @brief Executes bit test.
 */
std::uint8_t CPU::bit() {
    const std::uint8_t value = fetch();
    set_flag(Z, (reg_a & value) == 0);
    if (table[opcode].addrmode != &CPU::imm) {
        set_flag(N, (value & 0x80) != 0);
        set_flag(V, (value & 0x40) != 0);
    }
    return 0;
}

/**
 * @brief Executes software interrupt.
 */
std::uint8_t CPU::brk() {
    ++pc;
    interrupt(0xFFFE, 0, true);
    return 0;
}

/**
 * @brief Clears the carry flag.
 */
std::uint8_t CPU::clc() {
    set_flag(C, false);
    return 0;
}

/**
 * @brief Clears the decimal mode flag.
 */
std::uint8_t CPU::cld() {
    set_flag(D, false);
    return 0;
}

/**
 * @brief Clears the interrupt disable flag.
 */
std::uint8_t CPU::cli() {
    set_flag(I, false);
    return 0;
}

/**
 * @brief Clears the overflow flag.
 */
std::uint8_t CPU::clv() {
    set_flag(V, false);
    return 0;
}

/**
 * @brief Compares the accumulator with memory.
 */
std::uint8_t CPU::cmp() {
    return compare(reg_a);
}

/**
 * @brief Compares the X register with memory.
 */
std::uint8_t CPU::cpx() {
    return compare(reg_x);
}

/**
 * @brief Compares the Y register with memory.
 */
std::uint8_t CPU::cpy() {
    return compare(reg_y);
}

/**
 * @brief Decrements memory or the accumulator.
 */
std::uint8_t CPU::dec() {
    if (table[opcode].addrmode == &CPU::imp) {
        --reg_a;
        set_zn(reg_a);
    } else {
        const std::uint8_t value = static_cast<std::uint8_t>(read(addr_abs) - 1);
        write(addr_abs, value);
        set_zn(value);
    }
    return 0;
}

/**
 * @brief Decrements the X register.
 */
std::uint8_t CPU::dex() {
    --reg_x;
    set_zn(reg_x);
    return 0;
}

/**
 * @brief Decrements the Y register.
 */
std::uint8_t CPU::dey() {
    --reg_y;
    set_zn(reg_y);
    return 0;
}

/**
 * @brief Executes exclusive OR with the accumulator.
 */
std::uint8_t CPU::eor() {
    reg_a ^= fetch();
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Increments memory or the accumulator.
 */
std::uint8_t CPU::inc() {
    if (table[opcode].addrmode == &CPU::imp) {
        ++reg_a;
        set_zn(reg_a);
    } else {
        const std::uint8_t value = static_cast<std::uint8_t>(read(addr_abs) + 1);
        write(addr_abs, value);
        set_zn(value);
    }
    return 0;
}

/**
 * @brief Increments the X register.
 */
std::uint8_t CPU::inx() {
    ++reg_x;
    set_zn(reg_x);
    return 0;
}

/**
 * @brief Increments the Y register.
 */
std::uint8_t CPU::iny() {
    ++reg_y;
    set_zn(reg_y);
    return 0;
}

/**
 * @brief Jumps to the effective address.
 */
std::uint8_t CPU::jmp() {
    pc = addr_abs;
    return 0;
}

/**
 * @brief Jumps to subroutine.
 */
std::uint8_t CPU::jsr() {
    const std::uint16_t ret = static_cast<std::uint16_t>(pc - 1);
    push(static_cast<std::uint8_t>((ret >> 8) & 0xFF));
    push(static_cast<std::uint8_t>(ret & 0xFF));
    pc = addr_abs;
    return 0;
}

/**
 * @brief Loads the accumulator.
 */
std::uint8_t CPU::lda() {
    reg_a = fetch();
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Loads the X register.
 */
std::uint8_t CPU::ldx() {
    reg_x = fetch();
    set_zn(reg_x);
    return 1;
}

/**
 * @brief Loads the Y register.
 */
std::uint8_t CPU::ldy() {
    reg_y = fetch();
    set_zn(reg_y);
    return 1;
}

/**
 * @brief Executes logical shift right.
 */
std::uint8_t CPU::lsr() {
    const std::uint8_t value = fetch();
    set_flag(C, (value & 0x01) != 0);
    const std::uint8_t out = static_cast<std::uint8_t>(value >> 1);
    set_zn(out);
    if (table[opcode].addrmode == &CPU::acc) {
        reg_a = out;
    } else {
        write(addr_abs, out);
    }
    return 0;
}

/**
 * @brief Executes no operation.
 */
std::uint8_t CPU::nop() {
    return 0;
}

/**
 * @brief Executes logical OR with the accumulator.
 */
std::uint8_t CPU::ora() {
    reg_a |= fetch();
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Pushes the accumulator.
 */
std::uint8_t CPU::pha() {
    push(reg_a);
    return 0;
}

/**
 * @brief Pushes the processor status.
 */
std::uint8_t CPU::php() {
    push(reg_f | B | U);
    return 0;
}

/**
 * @brief Pushes the X register.
 */
std::uint8_t CPU::phx() {
    push(reg_x);
    return 0;
}

/**
 * @brief Pushes the Y register.
 */
std::uint8_t CPU::phy() {
    push(reg_y);
    return 0;
}

/**
 * @brief Pulls the accumulator.
 */
std::uint8_t CPU::pla() {
    reg_a = pull();
    set_zn(reg_a);
    return 0;
}

/**
 * @brief Pulls the processor status.
 */
std::uint8_t CPU::plp() {
    reg_f = static_cast<std::uint8_t>((pull() & ~B) | U);
    return 0;
}

/**
 * @brief Pulls the X register.
 */
std::uint8_t CPU::plx() {
    reg_x = pull();
    set_zn(reg_x);
    return 0;
}

/**
 * @brief Pulls the Y register.
 */
std::uint8_t CPU::ply() {
    reg_y = pull();
    set_zn(reg_y);
    return 0;
}

/**
 * @brief Resets a zero-page memory bit.
 */
std::uint8_t CPU::rmb() {
    const std::uint8_t mask = static_cast<std::uint8_t>(~(1 << bit_index_from_opcode()));
    write(addr_abs, static_cast<std::uint8_t>(read(addr_abs) & mask));
    return 0;
}

/**
 * @brief Executes rotate left.
 */
std::uint8_t CPU::rol() {
    const std::uint8_t value = fetch();
    const std::uint16_t result = static_cast<std::uint16_t>(value << 1) | (get_flag(C) ? 1 : 0);
    set_flag(C, (result & 0x100) != 0);
    const std::uint8_t out = static_cast<std::uint8_t>(result);
    set_zn(out);
    if (table[opcode].addrmode == &CPU::acc) {
        reg_a = out;
    } else {
        write(addr_abs, out);
    }
    return 0;
}

/**
 * @brief Executes rotate right.
 */
std::uint8_t CPU::ror() {
    const std::uint8_t value = fetch();
    const std::uint8_t carry = get_flag(C) ? 0x80 : 0x00;
    set_flag(C, (value & 0x01) != 0);
    const std::uint8_t out = static_cast<std::uint8_t>((value >> 1) | carry);
    set_zn(out);
    if (table[opcode].addrmode == &CPU::acc) {
        reg_a = out;
    } else {
        write(addr_abs, out);
    }
    return 0;
}

/**
 * @brief Returns from interrupt.
 */
std::uint8_t CPU::rti() {
    reg_f = static_cast<std::uint8_t>((pull() & ~B) | U);
    const std::uint16_t lo = pull();
    const std::uint16_t hi = pull();
    pc = static_cast<std::uint16_t>(lo | (hi << 8));
    return 0;
}

/**
 * @brief Returns from subroutine.
 */
std::uint8_t CPU::rts() {
    const std::uint16_t lo = pull();
    const std::uint16_t hi = pull();
    pc = static_cast<std::uint16_t>((lo | (hi << 8)) + 1);
    return 0;
}

/**
 * @brief Executes subtract with carry.
 */
std::uint8_t CPU::sbc() {
    const std::uint8_t value = static_cast<std::uint8_t>(fetch() ^ 0xFF);
    const std::uint16_t sum = static_cast<std::uint16_t>(reg_a) + value + (get_flag(C) ? 1 : 0);
    const std::uint8_t result = static_cast<std::uint8_t>(sum);
    set_flag(C, sum > 0xFF);
    set_flag(V, ((reg_a ^ result) & (value ^ result) & 0x80) != 0);
    reg_a = result;
    set_zn(reg_a);
    return 1;
}

/**
 * @brief Sets the carry flag.
 */
std::uint8_t CPU::sec() {
    set_flag(C, true);
    return 0;
}

/**
 * @brief Sets the decimal mode flag.
 */
std::uint8_t CPU::sed() {
    set_flag(D, true);
    return 0;
}

/**
 * @brief Sets the interrupt disable flag.
 */
std::uint8_t CPU::sei() {
    set_flag(I, true);
    return 0;
}

/**
 * @brief Sets a zero-page memory bit.
 */
std::uint8_t CPU::smb() {
    write(addr_abs, static_cast<std::uint8_t>(read(addr_abs) | (1 << bit_index_from_opcode())));
    return 0;
}

/**
 * @brief Stores the accumulator.
 */
std::uint8_t CPU::sta() {
    write(addr_abs, reg_a);
    return 0;
}

/**
 * @brief Stops the CPU.
 */
std::uint8_t CPU::stp() {
    stopped = true;
    return 0;
}

/**
 * @brief Stores the X register.
 */
std::uint8_t CPU::stx() {
    write(addr_abs, reg_x);
    return 0;
}

/**
 * @brief Stores the Y register.
 */
std::uint8_t CPU::sty() {
    write(addr_abs, reg_y);
    return 0;
}

/**
 * @brief Stores zero.
 */
std::uint8_t CPU::stz() {
    write(addr_abs, 0);
    return 0;
}

/**
 * @brief Transfers the accumulator to X.
 */
std::uint8_t CPU::tax() {
    reg_x = reg_a;
    set_zn(reg_x);
    return 0;
}

/**
 * @brief Transfers the accumulator to Y.
 */
std::uint8_t CPU::tay() {
    reg_y = reg_a;
    set_zn(reg_y);
    return 0;
}

/**
 * @brief Tests and resets memory bits.
 */
std::uint8_t CPU::trb() {
    const std::uint8_t value = read(addr_abs);
    set_flag(Z, (reg_a & value) == 0);
    write(addr_abs, static_cast<std::uint8_t>(value & ~reg_a));
    return 0;
}

/**
 * @brief Tests and sets memory bits.
 */
std::uint8_t CPU::tsb() {
    const std::uint8_t value = read(addr_abs);
    set_flag(Z, (reg_a & value) == 0);
    write(addr_abs, static_cast<std::uint8_t>(value | reg_a));
    return 0;
}

/**
 * @brief Transfers the stack pointer to X.
 */
std::uint8_t CPU::tsx() {
    reg_x = static_cast<std::uint8_t>(sp & 0x00FF);
    set_zn(reg_x);
    return 0;
}

/**
 * @brief Transfers X to the accumulator.
 */
std::uint8_t CPU::txa() {
    reg_a = reg_x;
    set_zn(reg_a);
    return 0;
}

/**
 * @brief Transfers X to the stack pointer.
 */
std::uint8_t CPU::txs() {
    sp = static_cast<std::uint16_t>(0x0100 | reg_x);
    return 0;
}

/**
 * @brief Transfers Y to the accumulator.
 */
std::uint8_t CPU::tya() {
    reg_a = reg_y;
    set_zn(reg_a);
    return 0;
}

/**
 * @brief Waits for an interrupt.
 */
std::uint8_t CPU::wai() {
    waiting = true;
    return 0;
}

} // namespace cpp65
