#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <span>

namespace cpp65 {

/**
 * @brief Abstract 16-bit address bus used by the CPU.
 */
class Bus {
public:
    virtual ~Bus() = default;

    /**
     * @brief Reads a byte from the bus.
     */
    virtual std::uint8_t read(std::uint16_t addr) = 0;

    /**
     * @brief Writes a byte to the bus.
     */
    virtual void write(std::uint16_t addr, std::uint8_t value) = 0;

    /**
     * @brief Asserts the reset line for the CPU to consume on its next tick.
     */
    void reset();

    /**
     * @brief Pulses the non-maskable interrupt line for the CPU to consume.
     */
    void nmi();

    /**
     * @brief Asserts the maskable interrupt line.
     */
    void irq();

    /**
     * @brief Clears the maskable interrupt line.
     */
    void clear_irq();

    /**
     * @brief Returns and clears the pending reset line state.
     */
    bool consume_reset();

    /**
     * @brief Returns and clears the pending NMI edge state.
     */
    bool consume_nmi();

    /**
     * @brief Returns whether the IRQ line is currently asserted.
     */
    bool irq_asserted() const;

private:
    bool reset_pending = false;
    bool nmi_pending = false;
    bool irq_line = false;
};

/**
 * @brief Simple 64 KiB RAM-backed bus.
 */
class RamBus : public Bus {
public:
    /**
     * @brief Reads a byte from RAM.
     */
    std::uint8_t read(std::uint16_t addr) override;

    /**
     * @brief Writes a byte to RAM.
     */
    void write(std::uint16_t addr, std::uint8_t value) override;

    /**
     * @brief Loads bytes into RAM starting at an address.
     */
    void load(std::uint16_t addr, std::initializer_list<std::uint8_t> bytes);

    /**
     * @brief Loads bytes into RAM starting at an address.
     */
    void load(std::uint16_t addr, std::span<const std::uint8_t> bytes);

    /**
     * @brief Writes the reset vector.
     */
    void set_reset_vector(std::uint16_t addr);

    /**
     * @brief Writes the NMI vector.
     */
    void set_nmi_vector(std::uint16_t addr);

    /**
     * @brief Writes the IRQ/BRK vector.
     */
    void set_irq_vector(std::uint16_t addr);

    /**
     * @brief Returns a byte from RAM without bus-side behavior.
     */
    std::uint8_t peek(std::uint16_t addr) const;

    /**
     * @brief Writes a byte into RAM without bus-side behavior.
     */
    void poke(std::uint16_t addr, std::uint8_t value);

private:
    std::array<std::uint8_t, 65536> memory{};

    /**
     * @brief Writes a little-endian interrupt vector.
     */
    void set_vector(std::uint16_t vector_addr, std::uint16_t target_addr);
};

/**
 * @brief RAM bus with a memory-mapped putchar output address.
 */
class PutCharBus : public RamBus {
public:
    using OutputFunction = std::function<void(std::uint8_t)>;

    /**
     * @brief Constructs a putchar bus with an optional output callback.
     */
    explicit PutCharBus(
        std::uint16_t output_addr = 0xF001,
        OutputFunction output = {}
    );

    /**
     * @brief Writes RAM or emits a character when writing to the output address.
     */
    void write(std::uint16_t addr, std::uint8_t value) override;

private:
    std::uint16_t output_addr;
    OutputFunction output;
};

} // namespace cpp65
