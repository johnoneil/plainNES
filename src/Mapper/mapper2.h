#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper2 : public Mapper {
    protected:
    uint8_t vertMirroring;
    std::vector<uint8_t> VRAM, CHR;
    std::vector<std::vector<uint8_t>> PRGROM;
    uint8_t PRGROMbank;
    bool usingCHRRAM = false;

    public:
    Mapper2(GAMEPAK::ROMInfo romInfo, std::ifstream &file);
    uint8_t memGet(uint16_t addr) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(std::ifstream &file) override;

    void powerOn() override;
};