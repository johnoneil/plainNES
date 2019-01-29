#pragma once

#include <stdint.h>
#include <array>

namespace APU {

void powerOn();
void reset();
void step();

uint8_t regGet(uint16_t addr, bool peek = false);
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