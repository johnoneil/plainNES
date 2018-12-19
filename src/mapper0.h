#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper0 : public Mapper {
    protected:
    bool horzMirroring;
    std::vector<uint8_t> VRAM, PRGROM, CHRROM;

    public:
    Mapper0(GAMEPAK::iNES_Header header);
    uint8_t memGet(uint16_t addr) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(GAMEPAK::iNES_Header header, std::ifstream &file) override;
};