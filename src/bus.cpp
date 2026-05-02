// SPDX-License-Identifier: LGPL-3.0-or-later
//
// cpp65 is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

#include "bus.h"

#include <utility>

namespace cpp65 {

/**
 * @brief Asserts the reset line for the CPU to consume on its next tick.
 */
void Bus::reset() {
    reset_pending = true;
}

/**
 * @brief Pulses the non-maskable interrupt line for the CPU to consume.
 */
void Bus::nmi() {
    nmi_pending = true;
}

/**
 * @brief Asserts the maskable interrupt line.
 */
void Bus::irq() {
    irq_line = true;
}

/**
 * @brief Clears the maskable interrupt line.
 */
void Bus::clear_irq() {
    irq_line = false;
}

/**
 * @brief Returns and clears the pending reset line state.
 */
bool Bus::consume_reset() {
    const bool requested = reset_pending;
    reset_pending = false;
    return requested;
}

/**
 * @brief Returns and clears the pending NMI edge state.
 */
bool Bus::consume_nmi() {
    const bool requested = nmi_pending;
    nmi_pending = false;
    return requested;
}

/**
 * @brief Returns whether the IRQ line is currently asserted.
 */
bool Bus::irq_asserted() const {
    return irq_line;
}

/**
 * @brief Reads a byte from RAM.
 */
std::uint8_t RamBus::read(std::uint16_t addr) {
    return memory[addr];
}

/**
 * @brief Writes a byte to RAM.
 */
void RamBus::write(std::uint16_t addr, std::uint8_t value) {
    memory[addr] = value;
}

/**
 * @brief Loads bytes into RAM starting at an address.
 */
void RamBus::load(std::uint16_t addr, std::initializer_list<std::uint8_t> bytes) {
    load(addr, std::span<const std::uint8_t>(bytes.begin(), bytes.size()));
}

/**
 * @brief Loads bytes into RAM starting at an address.
 */
void RamBus::load(std::uint16_t addr, std::span<const std::uint8_t> bytes) {
    std::uint16_t cursor = addr;
    for (std::uint8_t byte : bytes) {
        memory[cursor++] = byte;
    }
}

/**
 * @brief Writes the reset vector.
 */
void RamBus::set_reset_vector(std::uint16_t addr) {
    set_vector(0xFFFC, addr);
}

/**
 * @brief Writes the NMI vector.
 */
void RamBus::set_nmi_vector(std::uint16_t addr) {
    set_vector(0xFFFA, addr);
}

/**
 * @brief Writes the IRQ/BRK vector.
 */
void RamBus::set_irq_vector(std::uint16_t addr) {
    set_vector(0xFFFE, addr);
}

/**
 * @brief Returns a byte from RAM without bus-side behavior.
 */
std::uint8_t RamBus::peek(std::uint16_t addr) const {
    return memory[addr];
}

/**
 * @brief Writes a byte into RAM without bus-side behavior.
 */
void RamBus::poke(std::uint16_t addr, std::uint8_t value) {
    memory[addr] = value;
}

/**
 * @brief Writes a little-endian interrupt vector.
 */
void RamBus::set_vector(std::uint16_t vector_addr, std::uint16_t target_addr) {
    memory[vector_addr] = static_cast<std::uint8_t>(target_addr & 0x00FF);
    memory[static_cast<std::uint16_t>(vector_addr + 1)] =
        static_cast<std::uint8_t>((target_addr >> 8) & 0x00FF);
}

/**
 * @brief Constructs a putchar bus with an optional output callback.
 */
PutCharBus::PutCharBus(std::uint16_t output_addr_, OutputFunction output_)
    : output_addr(output_addr_),
      output(std::move(output_)) {}

/**
 * @brief Writes RAM or emits a character when writing to the output address.
 */
void PutCharBus::write(std::uint16_t addr, std::uint8_t value) {
    if (addr == output_addr) {
        if (output) {
            output(value);
        } else {
            std::putchar(value);
        }
        return;
    }

    RamBus::write(addr, value);
}

} // namespace cpp65
