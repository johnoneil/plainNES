#pragma once

#include <stdint.h>

namespace RENDER {

uint32_t* convertNTSC2ARGB(uint32_t* outputBuffer, uint8_t* inputPixelMap, int size);
int init();

}
