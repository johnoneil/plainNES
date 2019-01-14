# plainNES
A NES emulator written in C++

Uses Boost and SDL2 libraries

The emulator is still largely a work in progress.

The current version has a fully functional CPU, including unofficial opcodes, and passes NESTest and a growing number of other test ROMs. The PPU works well enough to display many test ROMs and some games. The APU is not currently implemented outside of IRQ behavior and timers.

Controls are hardcoded at this time:

| NES | Keyboard |
| --- | --- |
| A | X |
| B | Z |
| Start | Enter |
| Select | Spacebar |
| Up/Down/Left/Right | Direction Keys |
| RESET | Ctrl-R |