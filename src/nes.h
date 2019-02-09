#pragma once

#include <string>
#include <array>

namespace NES {

const int CPU_CLOCK_RATE = 1789773;
const int APU_CLOCK_RATE = CPU_CLOCK_RATE;
const int APU_AUDIO_BUFFER_SIZE = APU_CLOCK_RATE / 30; //Roughly two frames of audio

enum Options
{
    LOGGING         = 1 << 0,
    SET_PC_START    = 1 << 1,
};

struct AudioBuffer {
    std::array<float, APU_AUDIO_BUFFER_SIZE> buffer;
    int writeIdx;
    int readIdx;
};

extern AudioBuffer rawAudio;
extern std::array<uint8_t, 2> controller_state;

extern bool running, romLoaded;

extern bool logging;
extern uint16_t PC_debug_start;
extern std::ofstream logFile;

void enableLogging();
int loadROM(std::string filename);
void powerOn();
void reset();
void pause(bool enable);
void frameStep(bool force = false);

void setDebugPC(bool enable, uint16_t debugPC = 0);

unsigned long getFrameNum();
uint8_t getPalette(uint16_t addr);
uint8_t* getPixelMap();
std::array<std::array<uint8_t, 16*16*64>, 2> getPatternTableBuffers();

} //NES