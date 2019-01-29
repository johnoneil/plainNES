#pragma once

#include <stdint.h>
#include <array>

namespace IO
{

void init();
uint8_t regGet(uint16_t addr, bool peek = false);
void regSet(uint16_t addr, uint8_t val);

}
