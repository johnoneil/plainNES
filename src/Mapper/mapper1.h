#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper1 : public Mapper {
    protected:
    uint8_t mirroringMode, PRGbankmode, CHRbankmode;
    std::vector<uint8_t> VRAM;
    std::vector<std::vector<uint8_t>> PRGROM, PRGRAM, CHR;
    uint8_t CHRbank0, CHRbank1, PRGRAMbank, PRGROMbank;
    uint8_t MMCshiftReg;
    int writeCounter;
    bool usingCHRRAM = false;

    public:
    Mapper1(GAMEPAK::ROMInfo romInfo, std::ifstream &file);
    uint8_t memGet(uint16_t addr, bool peek = false) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr, bool peek = false) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(std::ifstream &file) override;

    void powerOn() override;
};