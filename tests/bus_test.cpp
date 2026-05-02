// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#include "bus.h"
#include "version.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void expect_eq(std::uint8_t actual, std::uint8_t expected) {
    if (actual != expected) {
        throw std::runtime_error("unexpected bus byte");
    }
}

void expect_string(const std::string& actual, const std::string& expected) {
    if (actual != expected) {
        throw std::runtime_error("unexpected bus output");
    }
}

void expect_version(std::string_view actual, std::string_view expected) {
    if (actual != expected) {
        throw std::runtime_error("unexpected version string");
    }
}

void test_builtin_version_is_available() {
    expect_eq(cpp65::version_major, 1);
    expect_eq(cpp65::version_minor, 0);
    expect_eq(cpp65::version_patch, 0);
    expect_version(cpp65::version, "1.0.0");
}

void test_ram_bus_read_write_and_load() {
    cpp65::RamBus bus;

    bus.write(0x1234, 0xAB);
    bus.load(0x2000, { 0x01, 0x02, 0x03 });

    expect_eq(bus.read(0x1234), 0xAB);
    expect_eq(bus.read(0x2000), 0x01);
    expect_eq(bus.read(0x2001), 0x02);
    expect_eq(bus.read(0x2002), 0x03);
}

void test_ram_bus_vectors_are_little_endian() {
    cpp65::RamBus bus;

    bus.set_nmi_vector(0x1234);
    bus.set_reset_vector(0x5678);
    bus.set_irq_vector(0x9ABC);

    expect_eq(bus.peek(0xFFFA), 0x34);
    expect_eq(bus.peek(0xFFFB), 0x12);
    expect_eq(bus.peek(0xFFFC), 0x78);
    expect_eq(bus.peek(0xFFFD), 0x56);
    expect_eq(bus.peek(0xFFFE), 0xBC);
    expect_eq(bus.peek(0xFFFF), 0x9A);
}

void test_putchar_bus_routes_output_address() {
    std::string output;
    cpp65::PutCharBus bus(
        0xF001,
        [&](std::uint8_t value) {
            output.push_back(static_cast<char>(value));
        }
    );

    bus.write(0xF001, 'A');
    bus.write(0x0200, 0x55);

    expect_string(output, "A");
    expect_eq(bus.peek(0x0200), 0x55);
    expect_eq(bus.peek(0xF001), 0x00);
}

} // namespace

int main() {
    test_builtin_version_is_available();
    test_ram_bus_read_write_and_load();
    test_ram_bus_vectors_are_little_endian();
    test_putchar_bus_routes_output_address();
    return 0;
}
