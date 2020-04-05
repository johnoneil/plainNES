#include "render.h"
#include <fstream>
#include <array>
#include <vector>

// Generated via: `xxd -i Resources/ntscpalette.pal > src/ntscpallete_pal.h`
#include "ntscpallete_pal.h"

namespace RENDER {

std::array<uint32_t, 0x40> NTSCkey;

int init()
{
    std::vector<unsigned char> palette(Resources_ntscpalette_pal, Resources_ntscpalette_pal + Resources_ntscpalette_pal_len);
    for(int i=0; i<0x40; ++i) {
        NTSCkey[i] = (palette[i*3+0]<<16)|(palette[i*3+1]<<8)|(palette[i*3+2]);
    }
    return 0;
}

uint8_t* convertNTSC2RGB(uint8_t* outputBuffer, uint8_t* inputPixelMap, int size)
{
    for(int i=0; i<(size/3); ++i) {
        outputBuffer[i*3 + 0] = (NTSCkey[inputPixelMap[i]] >> 16) & 0xFF;
        outputBuffer[i*3 + 1] = (NTSCkey[inputPixelMap[i]] >> 8) & 0xFF;
        outputBuffer[i*3 + 2] = NTSCkey[inputPixelMap[i]] & 0xFF;
    }
    return outputBuffer;
}


}