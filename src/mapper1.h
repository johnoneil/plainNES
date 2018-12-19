#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper1 : public Mapper {
    protected:
    uint8_t mirroringMode, PRGbankmode, CHRbankmode;
    std::vector<uint8_t> VRAM, PRGRAM;
    std::vector<std::vector<uint8_t>> PRGROM, CHR;
    uint8_t CHRbank0, CHRbank1, PRGbank;
    uint8_t MMCshiftReg;
    int writeCounter;

    public:
    Mapper1(GAMEPAK::iNES_Header header);
    uint8_t memGet(uint16_t addr) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(GAMEPAK::iNES_Header header, std::ifstream &file) override;
};