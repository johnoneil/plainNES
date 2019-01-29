#include "mapper2.h"
#include "ppu.h"
#include "cpu.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

Mapper2::Mapper2(GAMEPAK::ROMInfo romInfo, std::ifstream &file)
{
	vertMirroring = romInfo.mirroringMode;

    PRGROM.resize(romInfo.PRGROMsize / 0x4000, std::vector<uint8_t>(0x4000, 0));
    if(romInfo.CHRROMsize > 0) {
        usingCHRRAM = false;
        CHR.resize(romInfo.CHRROMsize);
    }
    else {
        usingCHRRAM = true;
        CHR.resize(0x2000);
    }
	
    VRAM.resize(0x800); //Uses standard VRAM

    std::fill(CHR.begin(), CHR.end(), 0);
	std::fill(VRAM.begin(), VRAM.end(), 0);

	loadData(file);

    PRGROMbank = 0;
}

uint8_t Mapper2::memGet(uint16_t addr)
{
	if(addr >= 0x8000 && addr < 0xC000) {
		CPU::busVal = PRGROM.at(PRGROMbank).at((addr - 0x8000) % 0x4000);
	}
    else if(addr >= 0xC000) {
        CPU::busVal = PRGROM.back().at((addr - 0xC000) % 0x4000);
    }
	return CPU::busVal;
}

void Mapper2::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x8000) {
        PRGROMbank = val % PRGROM.size();
    }
}

uint8_t Mapper2::PPUmemGet(uint16_t addr)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
        //Bankable CHR
		return CHR.at(addr);
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(vertMirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				return VRAM.at(addr % 0x0400);
			else
				return VRAM.at(0x0400 + addr % 0x0400);
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				return VRAM.at(addr - 0x2000);
			else
				return VRAM.at(addr - 0x2800);
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		return PPU::getPalette((uint8_t)(addr % 0x20));
	}
    return 0;
}

void Mapper2::PPUmemSet(uint16_t addr, uint8_t val)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
		CHR.at(addr) = val;
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(vertMirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				VRAM.at(addr % 0x0400) = val;
			else
				VRAM.at(0x0400 + addr % 0x0400) = val;
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				VRAM.at(addr - 0x2000) = val;
			else
				VRAM.at(addr - 0x2800) = val;
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
}

void Mapper2::loadData(std::ifstream &file)
{
	for(unsigned int bank=0; bank < PRGROM.size(); ++bank) {
		for(unsigned int idx=0; idx < PRGROM[bank].size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&PRGROM[bank][idx],1);
		}
	}

	if(usingCHRRAM == false) {
		for(unsigned int idx=0; idx < CHR.size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&CHR[idx],1);
		}
	}
}

void Mapper2::powerOn()
{
	PRGROMbank = 0;
}