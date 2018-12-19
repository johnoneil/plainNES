#pragma once

#include <stdint.h>
#include "gamepak.h"

class Mapper {
    public:
    virtual uint8_t memGet(uint16_t addr);
    virtual void memSet(uint16_t addr, uint8_t val);
    virtual uint8_t PPUmemGet(uint16_t addr);
    virtual void PPUmemSet(uint16_t addr, uint8_t val);
    virtual void loadData(GAMEPAK::iNES_Header header, std::ifstream &file);
};

