#include "Chip8.hpp"

int main(int argc, char const *argv[]) {
    init_emu();

    Chip8 emulator("EmuTest");

    emulator.load_program("resources/chipPrograms/Particle Demo [zeroZshadow, 2008].ch8");

    emulator.run();

    return 0;
}
