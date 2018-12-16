#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

namespace RENDER {

uint32_t* convertNTSC2ARGB(uint32_t* outputBuffer, uint8_t* inputPixelMap);
int init();

}
#endif