#pragma once

#include <stdint.h>
#include <array>

namespace IO
{

extern std::array<uint8_t, 2> controller_state;

void init();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);

}
