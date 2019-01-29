#include "mapper0.h"
#include "ppu.h"
#include "cpu.h"
#include "utils.h"
#include <iostream>

Mapper0::Mapper0(GAMEPAK::ROMInfo romInfo, std::ifstream &file)
{
	vertMirroring = romInfo.mirroringMode;
	PRGRAM.resize(0x2000); //For compatability, always 8k
	PRGROM.resize(romInfo.PRGROMsize);
	if(romInfo.PRGROMsize > 0x8000)
		std::cerr << "PRGROM larger than expected. Will ignore extra data" << std::endl;
	CHR.resize(0x2000); //Always 8k
	if(romInfo.CHRROMsize == 0)
		usingCHRRAM = true;
    VRAM.resize(0x800); //Uses standard VRAM

	loadData(file);
}

uint8_t Mapper0::memGet(uint16_t addr)
{
    if(addr >= 0x6000 && addr < 0x8000) {
		CPU::busVal = PRGRAM.at(addr - 0x6000);
	}
	else if(addr >= 0x8000) {
		CPU::busVal = PRGROM.at((addr - 0x8000) % PRGROM.size());
	}
	return CPU::busVal;
}

void Mapper0::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x6000 && addr < 0x8000) {
		PRGRAM.at(addr - 0x6000) = val;
	}
	else {
		std::cerr << "Invalid write attempt to " << int_to_hex(addr) << std::endl;
	}
    return;
}

uint8_t Mapper0::PPUmemGet(uint16_t addr)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
		return CHR.at(addr % CHR.size());
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

void Mapper0::PPUmemSet(uint16_t addr, uint8_t val)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
		if(usingCHRRAM)
			CHR.at(addr % CHR.size()) = val;
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

void Mapper0::loadData(std::ifstream &file)
{
	for(unsigned int idx=0; idx < PRGROM.size(); ++idx) {
		if(file.eof())
			throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
		file.read((char*)&PRGROM[idx],1);
	}

	if(usingCHRRAM == false) {
		for(unsigned int idx=0; idx < CHR.size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&CHR[idx],1);
		}
	}
}