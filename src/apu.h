#pragma once

#include <stdint.h>

namespace APU {

const int OUTPUT_AUDIO_FREQ = 48000;
const int OUTPUT_AUDIO_BUFFER_SIZE = 1024;

extern bool audioBufferReady;
extern uint16_t* outputBufferPtr;

void init();
void step();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);
void clockLengthCounters();
void clockEnvelopes();
void clockLinearCounter();
void clockSweep();
void stepPulse1();
void stepPulse2();
void stepTriangle();
void stepNoise();
void stepDMC();
void mixOutput();
void generateMixerTables();

}