# plainNES
A NES emulator written in C++

The emulator is currently largely a work in progress.

The current version has a fully functional CPU, including unofficial opcodes, and passes NESTest. The PPU works well enough to display some test ROMs, but is still unusable for games. The APU is not currently implemented. The emulator currently runs at top speed without throttling.

Controls are hardcoded at this time:

| NES | Keyboard |
| --- | --- |
| A | X |
| B | Z |
| Start | Enter |
| Select | Spacebar |
| Up/Down/Left/Right | Direction Keys |