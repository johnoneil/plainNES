# plainNES
A NES emulator written in C++

The emulator is still a work in progress, but can already run a variety of games. Graphics are rendered using OpenGL, and incorporates the ImGui GUI library. SDL2 is used for window management, input processing, and audio. A few Boost modules are also incorporated.

The current version has a fully functional CPU, including unofficial opcodes. Main PPU functionality is fully implemented, with some timing and obscure behavior still in work. The APU works, with some audio glitches.

Unit testing is performed with the Catch2 library, and tests against many of the test ROMs listed on the [NESDev Wiki](https://wiki.nesdev.com/w/index.php/Emulator_tests)

## Screenshots
![castlevania_rs](https://user-images.githubusercontent.com/3280380/52517364-6f89f100-2bef-11e9-91b7-e19334bca2cd.PNG)
![contra_rs](https://user-images.githubusercontent.com/3280380/52517365-73b60e80-2bef-11e9-8cb9-0187c999bc26.PNG)
![loz_rs](https://user-images.githubusercontent.com/3280380/52517368-77499580-2bef-11e9-8036-3f329520df16.PNG)
![megaman2_rs](https://user-images.githubusercontent.com/3280380/52517369-7a448600-2bef-11e9-9e59-498aaf5decc2.PNG)
![smb1_rs](https://user-images.githubusercontent.com/3280380/52517370-7d3f7680-2bef-11e9-8a9f-9377ed240b8b.PNG)
![excitebike_rs](https://user-images.githubusercontent.com/3280380/52517462-88df6d00-2bf0-11e9-861c-ca208aa9426f.PNG)


## Controls
Controls are hardcoded at this time:

| NES | Keyboard |
| --- | --- |
| A | X |
| B | Z |
| Start | Enter |
| Select | Spacebar |
| Up/Down/Left/Right | Direction Keys |
| RESET | Ctrl-R |





