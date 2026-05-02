#include "bus.h"
#include "cpu.h"

int main() {
    cpp65::RamBus bus;
    cpp65::CPU cpu(bus);
    (void)cpu;

    return 0;
}
