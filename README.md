# cpp65

[![CI](https://github.com/ifilot/cpp65/actions/workflows/ci.yml/badge.svg)](https://github.com/ifilot/cpp65/actions/workflows/ci.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

`cpp65` is a small C++20 6502/65C02 emulator project. It currently provides a
cycle-driven CPU core with selectable NMOS 6502 and WDC 65C02 models, a bus
abstraction, RAM-backed buses, examples, and a small test suite.

The project is intentionally simple and educational: the CPU talks only to a
`Bus`, and the bus is where RAM, devices, and CPU signal lines are modeled.

## Features

- C++20 CPU core with model-specific 256-entry opcode dispatch tables.
- Built-in version constants exposed through `version.h` and generated from
  the CMake project version.
- Selectable `CPUModel::nmos6502` and `CPUModel::wdc65c02` behavior.
- One-cycle-at-a-time execution through `CPU::tick()`.
- Abstract `Bus` interface for the 16-bit address bus, 8-bit data bus, and CPU
  signal lines.
- Reset, NMI, and IRQ signaling through the bus.
- `RamBus` with 64 KiB of memory.
- `PutCharBus` for simple memory-mapped console output.
- Example programs covering load/store, branches, subroutines, console output,
  and a small sieve.
- Unit and integration tests, including an optional `ca65`/`ld65` assembler
  integration test.

## Requirements

- CMake 3.20 or newer.
- A C++20 compiler.
- Optional: `ca65` and `ld65` from cc65 for the assembler integration test.

## Building

```bash
cmake -S . -B build
cmake --build build
```

Useful CMake options:

```bash
cmake -S . -B build -DCPP65_BUILD_TESTS=ON
cmake -S . -B build -DCPP65_BUILD_EXAMPLES=ON
cmake -S . -B build -DCPP65_ENABLE_WARNINGS=ON
```

All three options are enabled by default.

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

The test suite is split into:

- CPU instruction tests.
- Bus tests.
- Integration tests that run complete programs and check their output.
- Optional assembler integration test using `ca65` and `ld65`, when those tools
  are found by CMake.

## Running Examples

After building, example executables are placed in the build directory:

```bash
./build/cpp65_example_load_store
./build/cpp65_example_branch_loop
./build/cpp65_example_subroutine
./build/cpp65_example_memory_mapped_console
./build/cpp65_example_sieve
```

The console example prints:

```text
Hello
```

The sieve example computes primes below 32 in emulated memory and prints:

```text
02 03 05 07 0B 0D 11 13 17 1D 1F 
```

## Basic Usage

Create a bus, connect a CPU to it, load a program, set the reset vector, assert
reset, and then call `tick()` from your emulated machine clock.

```cpp
#include "bus.h"
#include "cpu.h"
#include "version.h"

#include <cstddef>
#include <cstdint>

int main() {
    static_assert(cpp65::version == "0.1.0");

    constexpr std::uint16_t program_start = 0x8000;
    constexpr std::uint16_t result_addr = 0x0200;

    cpp65::RamBus bus;
    cpp65::CPU cpu(bus, cpp65::CPUModel::wdc65c02);

    bus.set_reset_vector(program_start);
    bus.load(program_start, {
        0xA9, 0x2A,                         // LDA #$2A
        0x8D, 0x00, 0x02,                   // STA $0200
        0xDB,                               // STP
    });

    bus.reset();

    for (std::size_t i = 0; i < 16; ++i) {
        cpu.tick();
    }

    return bus.peek(result_addr) == 0x2A ? 0 : 1;
}
```

## Bus Model

`CPU` does not own memory directly. It reads and writes through a `Bus`:

```cpp
class Bus {
public:
    virtual std::uint8_t read(std::uint16_t addr) = 0;
    virtual void write(std::uint16_t addr, std::uint8_t value) = 0;

    void reset();
    void nmi();
    void irq();
    void clear_irq();
};
```

This means a larger emulator can provide its own bus implementation that maps
RAM, ROM, I/O registers, video memory, cartridge hardware, or test devices into
the CPU address space.

`RamBus` is the simplest implementation: it provides 64 KiB of RAM and helpers
for loading programs and setting interrupt vectors.

`PutCharBus` extends `RamBus` with a memory-mapped output register. By default,
writing a byte to `$F001` calls `std::putchar()`.

## Memory-Mapped Output

The memory-mapped console examples use this convention:

- `$F001`: write-only console output byte.

For example, this 65C02 sequence prints `A`:

```asm
lda #'A'
sta $F001
stp
```

The CPU does not know that `$F001` is special. That behavior belongs to the bus.

## CPU Models

The CPU constructor accepts a model:

```cpp
cpp65::CPU nmos6502(bus, cpp65::CPUModel::nmos6502);
cpp65::CPU wdc65c02(bus, cpp65::CPUModel::wdc65c02);
```

The default is `CPUModel::wdc65c02`, because the examples use 65C02 additions
such as `STP`.

The models use separate opcode tables and preserve visible behavioral
differences such as:

- NMOS 6502 official opcode set versus 65C02 extended opcodes.
- 65C02-only instructions such as `BRA`, `STZ`, `TSB`, `TRB`, `RMB`, `SMB`,
  `BBR`, `BBS`, `PHX`, `PLX`, `PHY`, `PLY`, `WAI`, and `STP`.
- NMOS 6502 `JMP ($xxFF)` indirect-addressing page-wrap behavior.
- 65C02 fixed `JMP (abs)` behavior.
- Model-specific decimal-mode `ADC`/`SBC` result, flag, and cycle behavior.
- Per-model opcode cycle counts for the implemented official instructions.

## Assembler Integration

If `ca65` and `ld65` are available, CMake builds an integration test program
from:

- `tests/asm/console_output.s`
- `tests/asm/console_output.cfg`

The resulting binary is loaded at `$8000`, executed by the emulator, and checked
for expected console output.

## Project Layout

```text
src/
  bus.h, bus.cpp        Bus abstractions and RAM/putchar bus implementations
  cpu.h, cpu.cpp        CPU core and opcode dispatch table
  main.cpp              Small top-level executable

examples/
  load_store.cpp
  branch_loop.cpp
  subroutine.cpp
  memory_mapped_console.cpp
  sieve.cpp

tests/
  cpu_instruction_test.cpp
  bus_test.cpp
  asm65_console_test.cpp
  asm/
    console_output.s
    console_output.cfg
```

## Current Limitations

- NMOS undocumented opcodes are treated as unsupported jammed opcodes rather
  than emulating their unofficial behavior.
- The emulator is still early-stage and focused on correctness-oriented tests
  and small examples.

## License

`cpp65` is licensed under the GNU Lesser General Public License v3.0 or later.
See [LICENSE](LICENSE) for the full license text.

## Development Notes

The core library target is `cpp65_core`. Examples and tests link against that
library, so changes to the CPU and bus are exercised across both small unit
tests and runnable programs.

The public API is intentionally narrow:

- Construct `cpp65::CPU` with a `cpp65::Bus&`.
- Drive the CPU with `CPU::tick()`.
- Signal reset, NMI, and IRQ through the bus.
- Expose memory and devices through a concrete bus implementation.
