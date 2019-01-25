#pragma once

#include <stdint.h>
#include <array>

namespace IO
{

void init();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);

}
