#pragma once

#include <stdint.h>

namespace IO
{

extern uint8_t controller_state[2];

void init();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);

}
