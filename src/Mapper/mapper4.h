#pragma once

#include <stdint.h>
#include <vector>
#include "gamepak.h"
#include "mapper.h"


class Mapper4 : public Mapper {
    protected:
    bool fourScreenMode = false;
    bool horzMirroring = false;
    bool IRQreload = false;
    bool IRQenabled = false;
    bool IRQrequested = false;
    bool A12low = true;
    std::vector<uint8_t> PRGRAM, VRAM;
    std::vector<std::vector<uint8_t>> PRGROM, CHRROM;
    uint8_t R0, R1, R2, R3, R4, R5, R6, R7;
    uint8_t regWriteSel = 0;
    uint8_t PRGbankmode = 0;
    uint8_t CHRbankmode = 0;
    uint8_t IRQlatch = 0;
    uint8_t IRQcntr = 0;
    uint8_t M2cntr = 0;
    uint16_t currPPUAddr = 0;

    public:
    Mapper4(GAMEPAK::ROMInfo romInfo, std::ifstream &file);
    uint8_t memGet(uint16_t addr, bool peek = false) override;
    void memSet(uint16_t addr, uint8_t val) override;
    uint8_t PPUmemGet(uint16_t addr, bool peek = false) override;
    void PPUmemSet(uint16_t addr, uint8_t val) override;
    void loadData(std::ifstream &file) override;

    void powerOn() override;

    void step() override;
};