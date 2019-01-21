#pragma once

#include <stdint.h>
#include <array>

namespace APU {

const int APU_AUDIO_RATE = 1789773;

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
void loadDMC();
void mixOutput();
void generateMixerTables();
float *getRawAudioBuffer();
int getRawAudioBufferSize();
void resetRawAudioBuffer();

}