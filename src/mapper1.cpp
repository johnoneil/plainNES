#include "mapper1.h"
#include "ppu.h"

Mapper1::Mapper1(GAMEPAK::iNES_Header header)
{
    VRAM.resize(0x800);
    PRGROM.resize(header.prgROM16cnt, std::vector<uint8_t>(0x4000, 0));
	PRGRAM.resize(0x2000);
    CHR.resize(32, std::vector<uint8_t>(0x1000, 0));

	//Assumed power on states
	mirroringMode = CHRbankmode = 0;
	PRGbankmode = 3;
	CHRbank0 = CHRbank1 = 0;
	PRGbank = 0;
	MMCshiftReg = 0;
	writeCounter = 0;
}

uint8_t Mapper1::memGet(uint16_t addr)
{
    if(addr >= 0x6000 && addr < 0x8000) {
		return PRGRAM[addr % 0x6000];
	}
	else if(addr >= 0x8000 && addr < 0xC000) {
		if(PRGbankmode <= 1) {
			return PRGROM[PRGbank & 0xE][addr % 0x8000];
		}
		else if(PRGbankmode == 2) {
			return PRGROM[0][addr % 0x8000];
		}
		else {
			return PRGROM[PRGbank & 0xF][addr % 0x8000];
		}
	}
	else if(addr >= 0xC000) {
		if(PRGbankmode <= 1) {
			return PRGROM[(PRGbank & 0xE) + 1][addr % 0xC000];
		}
		else if(PRGbankmode == 2) {
			return PRGROM[PRGbank & 0xF][addr % 0xC000];
		}
		else {
			return PRGROM.back()[addr % 0xC000];
		}
	}
	else {
		return 0;
	}
}

void Mapper1::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x6000 && addr < 0x8000) {
		PRGRAM[addr % 0x6000] = val;
	}
    else if(addr >= 0x8000) {	//MMC control
		
		if((val >> 7) == 1) {	//Clear shift register
			MMCshiftReg = 0;
			writeCounter = 0;
		}
		else {					//Write into register
			MMCshiftReg |= (val & 1) << writeCounter;
			++writeCounter;
			if(writeCounter >= 5) {
				switch((addr & 0x6000) >> 13) { //Only need bits 13 and 14
					case 0:
						mirroringMode = MMCshiftReg & 0x3;
						PRGbankmode = (MMCshiftReg & 0xC) >> 2;
						CHRbankmode = (MMCshiftReg & 0x10) >> 4;
						break;
					case 1:
						CHRbank0 = MMCshiftReg;
						break;
					case 2:
						CHRbank1 = MMCshiftReg;
						break;
					case 3:
						PRGbank = MMCshiftReg;
						break;
				}
				MMCshiftReg = 0;
				writeCounter = 0;
			}
		}
	}
}

uint8_t Mapper1::PPUmemGet(uint16_t addr)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x1000) {
		if(CHRbankmode == 0) {
			return CHR[CHRbank0 & 0x1E][addr];
		}
		else {
			return CHR[CHRbank0][addr];
		}
	}
	else if(addr < 0x2000) {
		if(CHRbankmode == 0) {
			return CHR[(CHRbank0 & 0x1E) + 1][addr % 0x1000];
		}
		else {
			return CHR[CHRbank1][addr % 0x1000];
		}
	}
	else if(addr < 0x3F00) {
		if(mirroringMode == 0) {
			return VRAM[addr % 0x0400];
		}
		else if(mirroringMode == 1) {
			return VRAM[(addr % 0x0400) + 0x400];
		}
		else if(mirroringMode == 2) {
			if(addr < 0x2800)
				return VRAM[addr - 0x2000];
			else
				return VRAM[addr - 0x2800];
		}
		else {
			if(addr < 0x2800)
				return VRAM[addr % 0x0400];
			else
				return VRAM[0x0400 + addr % 0x0400];
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		return PPU::getPalette((uint8_t)(addr % 0x20));
	}
    return 0;
}

void Mapper1::PPUmemSet(uint16_t addr, uint8_t val)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x1000) {
		if(CHRbankmode == 0) {
			CHR[CHRbank0 & 0x1E][addr] = val;
		}
		else {
			CHR[CHRbank0][addr] = val;
		}
	}
	else if(addr < 0x2000) {
		if(CHRbankmode == 0) {
			CHR[(CHRbank0 & 0x1E) + 1][addr % 0x1000] = val;
		}
		else {
			CHR[CHRbank1][addr % 0x1000] = val;
		}
	}
	else if(addr < 0x3F00) {
		if(mirroringMode == 0) {
			VRAM[addr % 0x0400] = val;
		}
		else if(mirroringMode == 1) {
			VRAM[(addr % 0x0400) + 0x400] = val;
		}
		else if(mirroringMode == 2) {
			if(addr < 0x2800)
				VRAM[addr - 0x2000] = val;
			else
				VRAM[addr - 0x2800] = val;
		}
		else {
			if(addr < 0x2800)
				VRAM[addr % 0x0400] = val;
			else
				VRAM[0x0400 + addr % 0x0400] = val;
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
}

void Mapper1::loadData(GAMEPAK::iNES_Header header, std::ifstream &file)
{
	for(unsigned int bank=0; bank < PRGROM.size(); ++bank) {
		for(unsigned int idx=0; idx < PRGROM[bank].size(); ++idx) {
			if(file.eof())
				break;
			file.read((char*)&PRGROM[bank][idx],1);
		}
	}

	for(unsigned int bank=0; bank < header.chrROM8cnt; ++bank) {
		for(unsigned int idx=0; idx < CHR[bank].size(); ++idx) {
			if(file.eof())
				break;
			file.read((char*)&CHR[bank][idx],1);
		}
	}

}