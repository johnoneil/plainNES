#include "render.h"
#include <fstream>
#include <array>

namespace RENDER {

std::array<uint32_t, 0x40> NTSCkey;

int init()
{
    std::ifstream PALfile("ntscpalette.pal",std::ios::binary);
    std::array<uint8_t, 3> buffer;
    for(int i=0; i<0x40; ++i) {
        if(PALfile.eof()) {
            PALfile.close();
            return 1;
        }
        PALfile.read((char*)buffer.data(), 3);
        NTSCkey[i] = (buffer[0]<<16)|(buffer[1]<<8)|(buffer[2]);
    }
    PALfile.close();
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