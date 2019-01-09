#pragma once

#include <stdint.h>

namespace APU {

void init();
void step();
void decLengthCounters();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);


}