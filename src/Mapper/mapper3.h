#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper3 : public Mapper {
    protected:
    bool vertMirroring;
    std::vector<uint8_t> VRAM, PRGROM;
    std::vector<std::vector<uint8_t>> CHRROM;
    uint8_t CHRbank;

    public:
    Mapper3(GAMEPAK::ROMInfo romInfo, std::ifstream &file);
    uint8_t memGet(uint16_t addr, bool peek = false) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr, bool peek = false) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(std::ifstream &file) override;
    void powerOn() override;
};