#include "render.h"
#include <fstream>

namespace RENDER {

uint32_t NTSCkey[0x40];

int init()
{
    std::ifstream PALfile("ntscpalette.pal",std::ios::binary);
    uint8_t buffer[3];
    for(int i=0; i<0x40; ++i) {
        if(PALfile.eof()) {
            PALfile.close();
            return 1;
        }
        PALfile.read((char*)buffer, 3);
        NTSCkey[i] = (0xFF << 24)|(buffer[0]<<16)|(buffer[1]<<8)|(buffer[2]);
    }
    PALfile.close();
    return 0;
}

uint32_t* convertNTSC2ARGB(uint32_t* outputBuffer, uint8_t* inputPixelMap, int size)
{
    for(int i=0; i<size; ++i) {
        outputBuffer[i] = NTSCkey[inputPixelMap[i]];
    }
    return outputBuffer;
}


}