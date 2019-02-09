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
bool romLoaded = false;

void enableLogging()
{
    logging = true;
	logFile.open("log.txt",std::ios::trunc);
}

int loadROM(std::string filename)
{
    romLoaded = false;
    std::ifstream file(filename, std::ios::binary);
    if(file.fail()) {
        std::cerr << "Unable to open file" << std::endl;
        return 1;
    }
    if(GAMEPAK::loadROM(file) > 0) {
		return 1;
	}
	file.close();
    romLoaded = true;
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

void pause(bool enable)
{
    running = !enable;
}

void frameStep(bool force)
{
    if(running || force) {
        while(PPU::isframeReady() == 0) {
            CPU::step();
        }
        PPU::setframeReady(false);
    }
}

void setDebugPC(bool enable, uint16_t debugPC)
{
    if(enable) {
        PC_debug_start_flag = true;
        PC_debug_start = debugPC;
    }
    else
        PC_debug_start_flag = false;
    
}

unsigned long getFrameNum()
{
    return PPU::frame;
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