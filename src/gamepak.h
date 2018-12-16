#ifndef GAMEPAK_H
#define GAMEPAK_H

#include <stdint.h>
#include <fstream>

namespace GAMEPAK {

void init();
void tick();
uint8_t CPUmemGet(uint16_t addr);
void CPUmemSet(uint16_t addr, uint8_t val);
uint8_t PPUmemGet(uint16_t addr);
void PPUmemSet(uint16_t addr, uint8_t val);
int loadROM(std::ifstream &file);


}

#endif