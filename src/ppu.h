#pragma once

#include <stdint.h>
#include <array>

namespace PPU {

extern unsigned int scanline;
extern unsigned int dot; //Dots are also called cycles and can be considered the pixel column

void init();
void step();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);
uint8_t getPalette(uint16_t addr);
void setPalette(uint16_t addr, uint8_t val);
void renderFrameStep();
void spriteEval();
void renderPixel();
void incrementHorz();
void incrementVert();
uint8_t* getPixelMap();
std::array<std::array<uint8_t, 16*16*64>, 2> getPatternTableBuffers();
bool isframeReady();
void setframeReady(bool set);

}
