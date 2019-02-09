#include "mapper4.h"
#include "ppu.h"
#include "cpu.h"
#include <iostream>
#include <algorithm>

//TODO: Add save ability for battery backed up PRG-RAM
Mapper4::Mapper4(GAMEPAK::ROMInfo romInfo, std::ifstream &file)
{
    PRGRAM.resize(0x2000);
    if(romInfo.fourScreenMode) {
        VRAM.resize(0x1000);
        fourScreenMode = true;
    }
    else VRAM.resize(0x800);
    PRGROM.resize(romInfo.PRGROMsize / 0x2000, std::vector<uint8_t>(0x2000, 0));
    CHRROM.resize(romInfo.CHRROMsize / 0x400, std::vector<uint8_t>(0x400, 0));

    std::fill(PRGRAM.begin(), PRGRAM.end(), 0);
	std::fill(VRAM.begin(), VRAM.end(), 0);

	loadData(file);
}

uint8_t Mapper4::memGet(uint16_t addr, bool peek)
{
    uint8_t returnedValue = CPU::busVal;
    if(addr >= 0x6000 && addr < 0x8000) {
        returnedValue = PRGRAM.at((addr - 0x6000) % 0x2000);
    }
    else if(addr >= 0x8000 && addr < 0xA000) {
        if(PRGbankmode == 0) returnedValue = PRGROM.at(R6).at((addr - 0x8000) % 0x2000);
        else returnedValue = PRGROM.end()[-2].at((addr - 0x8000) % 0x2000);
    }
    else if(addr >= 0xA000 && addr < 0xC000) {
        returnedValue = PRGROM.at(R7).at((addr - 0xA000) % 0x2000);
    }
    else if(addr >= 0xC000 && addr < 0xE000) {
        if(PRGbankmode == 0) returnedValue = PRGROM.end()[-2].at((addr - 0xC000) % 0x2000);
        else returnedValue = PRGROM.at(R6).at((addr - 0xC000) % 0x2000);
    }
    else if(addr >= 0xE000) {
        returnedValue = PRGROM.end()[-1].at((addr - 0xE000) % 0x2000);
    }
	if(peek) return returnedValue;
	CPU::busVal = returnedValue;
	return CPU::busVal;
}

void Mapper4::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x8000 && addr < 0xA000) {
        if(addr % 2 == 0) { //Even
            regWriteSel = val & 7;
            PRGbankmode = ((val & 0x40) > 0) ? 1 : 0;
            CHRbankmode = ((val & 0x80) > 0) ? 1 : 0;
        }
        else { //Odd
            switch(regWriteSel) {
                case 0: R0 = (val & 0xFE); break;
                case 1: R1 = (val & 0xFE); break;
                case 2: R2 = val; break;
                case 3: R3 = val; break;
                case 4: R4 = val; break;
                case 5: R5 = val; break;
                case 6: R6 = (val & 0x3F); break;
                case 7: R7 = (val & 0x3F); break;
            }
        }
    }
    else if(addr >= 0xA000 && addr < 0xC000) {
        if(addr % 2 == 0) { //Even
            horzMirroring = (val & 1) > 0;
        }
        //Ignore odd writes
        //PRG RAM protection doesn't need to be emulated
    }
    else if(addr >= 0xC000 && addr < 0xE000) {
        if(addr % 2 == 0) { //Even
            IRQlatch = val;
        }
        else { //Odd
            IRQreload = true;
        }
    }
    else if(addr >= 0xE000) {
        if(addr % 2 == 0) { //Even
            IRQenabled = false;
            IRQrequested = false;
            CPU::setIRQfromCart(IRQrequested);
        }
        else { //Odd
            IRQenabled = true;
        }
    }
}

uint8_t Mapper4::PPUmemGet(uint16_t addr, bool peek)
{
    PPUbusAddrChanged(addr % 0x3FFF);
	try {
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x0400) {
        if(CHRbankmode) return CHRROM.at(R2).at(addr % 0x400);
        else return CHRROM.at(R0).at(addr % 0x400);
    }
    else if(addr < 0x0800) {
        if(CHRbankmode) return CHRROM.at(R3).at(addr % 0x400);
        else return CHRROM.at(R0+1).at(addr % 0x400);
    }
    else if(addr < 0x0C00) {
        if(CHRbankmode) return CHRROM.at(R4).at(addr % 0x400);
        else return CHRROM.at(R1).at(addr % 0x400);
    }
    else if(addr < 0x1000) {
        if(CHRbankmode) return CHRROM.at(R5).at(addr % 0x400);
        else return CHRROM.at(R1+1).at(addr % 0x400);
    }
    else if(addr < 0x1400) {
        if(CHRbankmode) return CHRROM.at(R0).at(addr % 0x400);
        else return CHRROM.at(R2).at(addr % 0x400);
    }
    else if(addr < 0x1800) {
        if(CHRbankmode) return CHRROM.at(R0+1).at(addr % 0x400);
        else return CHRROM.at(R3).at(addr % 0x400);
    }
    else if(addr < 0x1C00) {
        if(CHRbankmode) return CHRROM.at(R1).at(addr % 0x400);
        else return CHRROM.at(R4).at(addr % 0x400);
    }
    else if(addr < 0x2000) {
        if(CHRbankmode) return CHRROM.at(R1+1).at(addr % 0x400);
        else return CHRROM.at(R5).at(addr % 0x400);
    }
	else if(addr < 0x3F00) {
		if(addr > 0x2FFF) addr -= 0x1000;
        if(fourScreenMode) return VRAM.at(addr - 0x2000);

		if(horzMirroring == false) {
			if(addr < 0x2800)
				return VRAM.at(addr - 0x2000);
			else
				return VRAM.at(addr - 0x2800);
		}
		else {
			if(addr < 0x2800)
				return VRAM.at(addr % 0x0400);
			else
				return VRAM.at(0x0400 + addr % 0x0400);
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		return PPU::getPalette((uint8_t)(addr % 0x20));
	}
	}
	catch(const std::out_of_range err)
	{
		std::cerr << "Out of range error when trying to get addr: " << addr << std::endl;
	}
    return 0;
}

void Mapper4::PPUmemSet(uint16_t addr, uint8_t val)
{
    PPUbusAddrChanged(addr % 0x3FFF);
	try {
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;

    //Can't write to CHRROM
    
	/*if(addr < 0x0400) {
        if(CHRbankmode) CHRROM.at(R2).at(addr % 0x400) = val;
        else CHRROM.at(R0).at(addr % 0x400) = val;
    }
    else if(addr < 0x0800) {
        if(CHRbankmode) CHRROM.at(R3).at(addr % 0x400) = val;
        else CHRROM.at(R0+1).at(addr % 0x400) = val;
    }
    else if(addr < 0x0C00) {
        if(CHRbankmode) CHRROM.at(R4).at(addr % 0x400) = val;
        else CHRROM.at(R1).at(addr % 0x400) = val;
    }
    else if(addr < 0x1000) {
        if(CHRbankmode) CHRROM.at(R5).at(addr % 0x400) = val;
        else CHRROM.at(R1+1).at(addr % 0x400) = val;
    }
    else if(addr < 0x1400) {
        if(CHRbankmode) CHRROM.at(R0).at(addr % 0x400) = val;
        else CHRROM.at(R2).at(addr % 0x400) = val;
    }
    else if(addr < 0x1800) {
        if(CHRbankmode) CHRROM.at(R0+1).at(addr % 0x400) = val;
        else CHRROM.at(R3).at(addr % 0x400) = val;
    }
    else if(addr < 0x1C00) {
        if(CHRbankmode) CHRROM.at(R1).at(addr % 0x400) = val;
        else CHRROM.at(R4).at(addr % 0x400) = val;
    }
    else if(addr < 0x2000) {
        if(CHRbankmode) CHRROM.at(R1+1).at(addr % 0x400) = val;
        else CHRROM.at(R5).at(addr % 0x400) = val;
    }*/
	if(addr >= 0x2000 && addr < 0x3F00) {
		if(addr > 0x2FFF) addr -= 0x1000;
        if(fourScreenMode) VRAM.at(addr - 0x2000) = val;

		if(horzMirroring == false) {
			if(addr < 0x2800)
				VRAM.at(addr - 0x2000) = val;
			else
				VRAM.at(addr - 0x2800) = val;
		}
		else {
			if(addr < 0x2800)
				VRAM.at(addr % 0x0400) = val;
			else
				VRAM.at(0x0400 + addr % 0x0400) = val;
		}
	}
	else if(addr >= 0x3F00 && addr < 0x4000) {
		//Internal to PPU. Never mapped.
		return PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
	}
	catch(const std::out_of_range err)
	{
		std::cerr << "Out of range error when trying to set addr: " << addr << std::endl;
	}
}

void Mapper4::loadData(std::ifstream &file)
{
	for(unsigned int bank=0; bank < PRGROM.size(); ++bank) {
		for(unsigned int idx=0; idx < PRGROM[bank].size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&PRGROM[bank][idx],1);
		}
	}

    for(unsigned int bank=0; bank < CHRROM.size(); ++bank) {
        for(unsigned int idx=0; idx < CHRROM[bank].size(); ++idx) {
            if(file.eof())
                throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
            file.read((char*)&CHRROM[bank][idx],1);
        }
    }
}

void Mapper4::powerOn()
{
	//Assumed power on states
	fourScreenMode = false;
    horzMirroring = false;
    IRQreload = false;
    IRQenabled = false;
    IRQrequested = false;
    R0 = R1 = R2 = R3 = R4 = R5 = R6 = R7 = 0;
    regWriteSel = 0;
    PRGbankmode = 0;
    CHRbankmode = 0;
    IRQlatch = 0;
}

void Mapper4::CPUstep()
{
    //M2 rises and falls every CPU cycle
    //if((lastVRAMaddr & 0x1000) == 0)
    //    ++M2cntr;
}

void Mapper4::PPUstep()
{
    CPU::setIRQfromCart(IRQrequested);
}

void Mapper4::PPUbusAddrChanged(uint16_t newAddr)
{
    if((lastVRAMaddr & 0x1000) == 0 && (newAddr & 0x1000) > 0) {
        std::cout << PPU::scanline << ":" << PPU::dot << " Clocking" << std::endl;
        if(IRQcntr == 0 || IRQreload) {
            IRQcntr = IRQlatch;
            IRQreload = false;
        }
        else {
            --IRQcntr;
        }
        if(IRQcntr == 0 && IRQenabled) {
            IRQrequested = true;
            CPU::setIRQfromCart(IRQrequested);
        }
    }
    lastVRAMaddr = newAddr;
}