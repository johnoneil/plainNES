#pragma once

#include <stdint.h>
#include "gamepak.h"

class Mapper {
    public:
    //Memory access functions for CPU mappable memory banks 0x4020 - 0xFFFF (PRGROM/PRGRAM)
    //0x4020-0x5FFF often empty (open bus behavior)
    //0x6000-0x7FFF normally PRGRAM
    //0x8000-0xFFFF normally PRGROM
    virtual uint8_t memGet(uint16_t addr, bool peek = false);
    virtual void memSet(uint16_t addr, uint8_t val);

    //Memory access functions for PPU mappable memory banks
    //0x0000-0x1FFF: Pattern tables (CHRROM/CHRRAM)
    //0x2000-0x3EFF: Nametables (VRAM) - often just the internal 2kb RAM (0x800 bytes) mirrored
    //0x3F00-0x4000: PPU palette. Never mappable, so use PPU get/set functions
    virtual uint8_t PPUmemGet(uint16_t addr, bool peek = false);
    virtual void PPUmemSet(uint16_t addr, uint8_t val);

    //Load rom file into memory
    virtual void loadData(std::ifstream &file);

    //Handles mapper behavior at powerOn or reset
    virtual void powerOn();
    virtual void reset();

    //For mappers which react to clock signals or other systems
    virtual void CPUstep(); //Occurs every CPU step
    virtual void PPUstep(); //Occurs every PPU step
};

