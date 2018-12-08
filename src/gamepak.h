#ifndef GAMEPAK_H
#define GAMEPAK_H

#include <stdint.h>
#include <fstream>

namespace GAMEPAK {

void init();
void tick();
uint8_t memGet(uint16_t addr);
void memSet(uint16_t addr, uint8_t val);
int loadROM(std::ifstream &file);


}

#endif