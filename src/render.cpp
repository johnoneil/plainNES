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
    // 64 (0x40) colors (defined by r,g,b) so input palette is length (64*3) = 192 (0xC0)
    for(int i=0; i<64; ++i) {
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

uint32_t* convertNTSC2ARGB(uint32_t* outputBuffer, uint8_t* inputPixelMap, int size)
{
    for(int i=0; i<size; ++i) {
        uint8_t a = 0xff;
        uint8_t r = (NTSCkey[inputPixelMap[i]] >> 16) & 0xFF;
        uint8_t g = (NTSCkey[inputPixelMap[i]] >> 8) & 0xFF;
        uint8_t b = (NTSCkey[inputPixelMap[i]]) & 0xFF;
        outputBuffer[i] =  (a<<24) | (r<<16) | (g<<8) | (b);
    }
    return outputBuffer;
}


}
