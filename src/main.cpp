// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2026 Ivo Filot
// Repository: https://github.com/ifilot/cpp65

#include "bus.h"
#include "cpu.h"
#include "version.h"

#include <iostream>

int main() {
    cpp65::RamBus bus;
    cpp65::CPU cpu(bus);
    (void)cpu;

    std::cout << "cpp65 " << cpp65::version << '\n';

    return 0;
}
