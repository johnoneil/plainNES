#include "mapper3.h"
#include "ppu.h"
#include "cpu.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

Mapper3::Mapper3(GAMEPAK::ROMInfo romInfo, std::ifstream &file)
{
	vertMirroring = romInfo.mirroringMode;
	PRGROM.resize(romInfo.PRGROMsize);
	if(romInfo.PRGROMsize > 0x8000)
		std::cerr << "PRGROM larger than expected. Will ignore extra data" << std::endl;
	CHRROM.resize(romInfo.CHRROMsize / 0x2000, std::vector<uint8_t>(0x2000, 0));
    VRAM.resize(0x800); //Uses standard VRAM

	std::fill(VRAM.begin(), VRAM.end(), 0);

	loadData(file);
}

uint8_t Mapper3::memGet(uint16_t addr, bool peek)
{
	uint8_t returnedValue = CPU::busVal;
	if(addr >= 0x8000) {
		returnedValue = PRGROM.at((addr - 0x8000) % PRGROM.size());
	}
	if(peek) return returnedValue;
	CPU::busVal = returnedValue;
	return CPU::busVal;
}

void Mapper3::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x8000) {
        CHRbank = val % CHRROM.size();
    }
}

uint8_t Mapper3::PPUmemGet(uint16_t addr, bool peek)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
        //Bankable CHR
		return CHRROM.at(CHRbank).at(addr);
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

void Mapper3::PPUmemSet(uint16_t addr, uint8_t val)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
		//CHRROM not writable
		std::cerr << "Invalid write attempt to " << int_to_hex(addr) << std::endl;
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

void Mapper3::loadData(std::ifstream &file)
{
	for(unsigned int idx=0; idx < PRGROM.size(); ++idx) {
		if(file.eof())
			throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
		file.read((char*)&PRGROM[idx],1);
	}

	for(unsigned int bank=0; bank < CHRROM.size(); ++bank) {
		for(unsigned int idx=0; idx < CHRROM[bank].size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&CHRROM[bank][idx],1);
		}
	}
}

void Mapper3::powerOn()
{
	CHRbank = 0;
}