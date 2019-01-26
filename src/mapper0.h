#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper0 : public Mapper {
    protected:
    bool vertMirroring;
    bool usingCHRRAM = false;
    std::vector<uint8_t> VRAM, PRGROM, CHR, PRGRAM;

    public:
    Mapper0(GAMEPAK::ROMInfo romInfo, std::ifstream &file);
    uint8_t memGet(uint16_t addr) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(std::ifstream &file) override;
};