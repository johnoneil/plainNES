#include "nes.h"
#include "apu.h"
#include "cpu.h"
#include "gamepak.h"
#include "ppu.h"
#include <iostream>
#include <fstream>
#include <array>


namespace NES {

//Audio
AudioBuffer rawAudio;
std::array<uint8_t, 2> controller_state;

//Options
bool logging = false;
bool PC_debug_start_flag = false;
uint16_t PC_debug_start = 0;

std::ofstream logFile;

bool running = false;

void setOptions(int options)
{
    logging = (options & LOGGING) > 0;
    PC_debug_start_flag = (options & SET_PC_START) > 0;

    if(logging) {
		logFile.open("log.txt",std::ios::trunc);
	}
}

int loadROM(std::string filename)
{
    std::ifstream file(filename, std::ios::binary);
    if(file.fail()) {
        std::cerr << "Unable to open file" << std::endl;
        return 1;
    }
    if(GAMEPAK::loadROM(file) > 0) {
		return 1;
	}
	file.close();
    return 0;
}

void powerOn()
{
    GAMEPAK::powerOn();
    CPU::powerOn();
	PPU::powerOn();
	APU::powerOn();

	if(PC_debug_start_flag) {
		CPU::setPC(PC_debug_start);
	}

    running = true;
}

void reset()
{
    GAMEPAK::reset();
    CPU::reset();
    PPU::reset();
    APU::reset();
}

void frameStep()
{
    while(PPU::isframeReady() == 0) {
		CPU::step();
	}
    PPU::setframeReady(false);
}

uint8_t getPalette(uint16_t addr) {
    return PPU::getPalette(addr);
}

uint8_t* getPixelMap() {
    return PPU::pixelMap.data();
}

std::array<std::array<uint8_t, 16*16*64>, 2> getPatternTableBuffers() {
    return PPU::getPatternTableBuffers();
}


} //NES