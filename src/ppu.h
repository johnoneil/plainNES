#pragma once

#include <stdint.h>

namespace PPU {

void init();
void step();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);
uint8_t getVRAM(uint16_t addr);
void setVRAM(uint16_t addr, uint8_t val);
uint8_t getPalette(uint16_t addr);
void setPalette(uint16_t addr, uint8_t val);
void renderFrameStep();
void spriteEval();
void renderPixel();
void incrementHorz();
void incrementVert();
uint8_t* getPixelMap();
uint8_t* getPatternTableBuffers();
bool isframeReady();
void setframeReady(bool set);

}
