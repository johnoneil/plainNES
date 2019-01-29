#include "mapper1.h"
#include "ppu.h"
#include "cpu.h"
#include <iostream>
#include <algorithm>

//TODO: Add save ability for battery backed up PRG-RAM
Mapper1::Mapper1(GAMEPAK::ROMInfo romInfo, std::ifstream &file)
{
	if(romInfo.iNESversion == 1) {
		//Have to assume 8k PRG-RAM. Won't work with a few uncommon games that use bankable PRG-RAM
		PRGRAM.resize(1, std::vector<uint8_t>(0x2000, 0));
		PRGROM.resize(romInfo.PRGROMsize / 0x4000, std::vector<uint8_t>(0x4000, 0));
		if(romInfo.CHRROMsize == 0) {
			usingCHRRAM = true;
			CHR.resize(2, std::vector<uint8_t>(0x1000, 0));
		}
		else {
			usingCHRRAM = false;
			CHR.resize(romInfo.CHRROMsize / 0x1000, std::vector<uint8_t>(0x1000, 0));
		}
		VRAM.resize(0x800);
	}
	else {
		if(romInfo.batteryPresent && romInfo.PRGNVRAMsize > 0)
			PRGRAM.resize(romInfo.PRGNVRAMsize / 0x2000, std::vector<uint8_t>(0x2000, 0));
		else if(romInfo.PRGRAMsize > 0)
			PRGRAM.resize(romInfo.PRGRAMsize / 0x2000, std::vector<uint8_t>(0x2000, 0));
		
		PRGROM.resize(romInfo.PRGROMsize / 0x4000, std::vector<uint8_t>(0x4000, 0));
		if(romInfo.CHRROMsize == 0) {
			usingCHRRAM = true;
			CHR.resize(romInfo.CHRRAMsize / 0x1000, std::vector<uint8_t>(0x1000, 0));
		}
		else {
			usingCHRRAM = false;
			CHR.resize(romInfo.CHRROMsize / 0x1000, std::vector<uint8_t>(0x1000, 0));
		}
		VRAM.resize(0x800);
	}

	std::fill(VRAM.begin(), VRAM.end(), 0);

	loadData(file);
}

uint8_t Mapper1::memGet(uint16_t addr, bool peek)
{
	uint8_t returnedValue = CPU::busVal;
    if(addr >= 0x6000 && addr < 0x8000) {
		addr -= 0x6000;
		if(PRGRAM.size() > 0)
			returnedValue = PRGRAM.at(PRGRAMbank).at(addr);
	}
	else if(addr >= 0x8000 && addr < 0xC000) {
		addr -= 0x8000;
		if(PRGbankmode <= 1) {
			returnedValue = PRGROM.at(PRGROMbank & 0xE).at(addr);
		}
		else if(PRGbankmode == 2) {
			returnedValue = PRGROM.at(0).at(addr % 0x8000);
		}
		else {
			returnedValue = PRGROM.at(PRGROMbank & 0xF).at(addr);
		}
	}
	else if(addr >= 0xC000) {
		addr -= 0xC000;
		if(PRGbankmode <= 1) {
			returnedValue = PRGROM.at((PRGROMbank & 0xE) + 1).at(addr);
		}
		else if(PRGbankmode == 2) {
			returnedValue = PRGROM.at(PRGROMbank & 0xF).at(addr);
		}
		else {
			returnedValue = PRGROM.back().at(addr);
		}
	}
	if(peek) return returnedValue;
	CPU::busVal = returnedValue;
	return CPU::busVal;
}

void Mapper1::memSet(uint16_t addr, uint8_t val)
{
    if(addr >= 0x6000 && addr < 0x8000) {
		addr -= 0x6000;
		if(PRGRAM.size() > 0)
			PRGRAM.at(PRGRAMbank).at(addr) = val;
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
						PRGROMbank = MMCshiftReg;
						break;
				}
				MMCshiftReg = 0;
				writeCounter = 0;
			}
		}
	}
}

uint8_t Mapper1::PPUmemGet(uint16_t addr, bool peek)
{
	try {
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x1000) {
		if(CHRbankmode == 0) {
			return CHR.at(CHRbank0 & 0x1E).at(addr);
		}
		else {
			return CHR.at(CHRbank0).at(addr);
		}
	}
	else if(addr < 0x2000) {
		if(CHRbankmode == 0) {
			return CHR.at((CHRbank0 & 0x1E) + 1).at(addr % 0x1000);
		}
		else {
			return CHR.at(CHRbank1).at(addr % 0x1000);
		}
	}
	else if(addr < 0x3F00) {
		if(addr > 0x2FFF) addr -= 0x1000;
		if(mirroringMode == 0) {
			return VRAM.at(addr % 0x0400);
		}
		else if(mirroringMode == 1) {
			return VRAM.at((addr % 0x0400) + 0x400);
		}
		else if(mirroringMode == 2) {
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

void Mapper1::PPUmemSet(uint16_t addr, uint8_t val)
{
	try {
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x1000) {
		if(CHRbankmode == 0) {
			CHR.at(CHRbank0 & 0x1E).at(addr) = val;
		}
		else {
			CHR.at(CHRbank0).at(addr) = val;
		}
	}
	else if(addr < 0x2000) {
		if(CHRbankmode == 0) {
			CHR.at((CHRbank0 & 0x1E) + 1).at(addr % 0x1000) = val;
		}
		else {
			CHR.at(CHRbank1).at(addr % 0x1000) = val;
		}
	}
	else if(addr < 0x3F00) {
		if(addr > 0x2FFF) addr -= 0x1000;
		if(mirroringMode == 0) {
			VRAM.at(addr % 0x0400) = val;
		}
		else if(mirroringMode == 1) {
			VRAM.at((addr % 0x0400) + 0x400) = val;
		}
		else if(mirroringMode == 2) {
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
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
	}
	catch(const std::out_of_range err)
	{
		std::cerr << "Out of range error when trying to set addr: " << addr << std::endl;
	}
}

void Mapper1::loadData(std::ifstream &file)
{
	for(unsigned int bank=0; bank < PRGROM.size(); ++bank) {
		for(unsigned int idx=0; idx < PRGROM[bank].size(); ++idx) {
			if(file.eof())
				throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
			file.read((char*)&PRGROM[bank][idx],1);
		}
	}

	if(usingCHRRAM == false) {
		for(unsigned int bank=0; bank < CHR.size(); ++bank) {
			for(unsigned int idx=0; idx < CHR[bank].size(); ++idx) {
				if(file.eof())
					throw std::out_of_range("Reached EOF unexpectantly while loading ROM");
				file.read((char*)&CHR[bank][idx],1);
			}
		}
	}
}

void Mapper1::powerOn()
{
	//Assumed power on states
	mirroringMode = CHRbankmode = 0;
	PRGbankmode = 3;
	CHRbank0 = CHRbank1 = 0;
	PRGRAMbank = PRGROMbank = 0;
	MMCshiftReg = 0;
	writeCounter = 0;
}