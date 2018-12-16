#include "gamepak.h"
#include "PPU.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>

namespace GAMEPAK {

std::vector<uint8_t> PRGROM, PRGRAM, CHRROM, CHRRAM;

const char* headerName = "NES\x1A";

struct iNES_Header {
	char NES[4];				//bytes 0-3
	uint8_t prgROM16cnt;		//bytes 4
	uint8_t chrROM8cnt;			//bytes 5
	uint8_t mirroring : 1;		//bytes 6.0
	uint8_t BattRAM : 1;		//bytes 6.1
	uint8_t trainer : 1;		//bytes 6.2
	uint8_t FourScreenMode : 1;	//bytes 6.3
	uint8_t mapperLSB : 4;		//bytes 6.4-7
	uint8_t vsUnisystem : 1;	//bytes 7.0
	uint8_t PlayChoice10 : 1;	//bytes 7.1
	uint8_t iNES2 : 2;			//bytes 7.2-3
	uint8_t mapperMSB : 4;		//bytes 7.4-7
	uint8_t prgRAM8cnt;			//bytes 8
}__attribute__((packed)) header;


void init() {
	
}

void tick() {
	
}

uint8_t CPUmemGet(uint16_t addr) {
	//Mapper 0 NROM only for now
	if(addr >= 0x6000 && addr < 0x8000) {
		return PRGRAM[(addr - 0x6000) % PRGRAM.size()];
	}
	else if(addr >= 0x8000) {
		return PRGROM[(addr - 0x8000) % PRGROM.size()];
	}
	else {
		std::cout << "Invalid memory location in GAMEPAK\n";
		return 0;
	}
}

void CPUmemSet(uint16_t addr, uint8_t val) {
	//Mapper 0 NROM only for now
	if(addr >= 0x6000 && addr < 0x8000) {
		PRGRAM[(addr - 0x6000) % PRGRAM.size()] = val;
	}
	else if(addr >= 0x8000) {
		std::cout << "Can't write to " << std::hex << addr << std::endl;
	}
	else {
		std::cout << "Invalid memory location in GAMEPAK\n";
	}
}

uint8_t PPUmemGet(uint16_t addr)
{
	//Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	//Mapper 0 for now
	if(addr < 0x2000) {
		return CHRROM[addr];
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(header.mirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				return PPU::getVRAM(addr % 0x0400);
			else
				return PPU::getVRAM(0x0400 + addr % 0x0400);
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				return PPU::getVRAM(addr - 0x2000);
			else
				return PPU::getVRAM(addr - 0x2800);
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		return PPU::getPalette((uint8_t)(addr % 0x20));
	}
	return 0;
}

void PPUmemSet(uint16_t addr, uint8_t val)
{
	//Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	//Mapper 0 for now
	if(addr < 0x2000) {
		return; //ROM. Do nothing
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(header.mirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				PPU::setVRAM(addr % 0x0400, val);
			else
				PPU::setVRAM(0x0400 + addr % 0x0400, val);
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				PPU::setVRAM(addr - 0x2000, val);
			else
				PPU::setVRAM(addr - 0x2800, val);
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
}

int loadROM(std::ifstream &file) {
	char headerdata[16];
	file.read(headerdata,16);
	
	if(memcmp(headerName,headerdata,4) != 0) {
		std::cout << "Invalid file format\nIS: ";
		for(int i=0; i<4; i++)
			std::cout << std::hex << int(headerdata[i]) << " ";
		std::cout << "\nSB: ";
		for(int i=0; i<4; i++)
			std::cout << std::hex << int(headerdata[i]) << " ";
		std::cout << std::endl;
		return -1;
	}
	if(((headerdata[7] >> 2) & 0x03) == 2) {
		std::cout << "iNES2 header format. Not yet implemented\n";
		return -1;
	}
	
	
	memcpy(&header, headerdata, sizeof(header));
	
	printf("PRG16Cnt: %i\nCHR8Cnt: %i\nMirroring: %i\nBattRAM: %i\nTrainer: %i\nFour Screen Mode: %i\nMapper: %i\nvsUnisystem: %i\nPlayChoice10: %i\niNES2: %i\nPRGRAM8Cnt: %i\n",
		   header.prgROM16cnt,header.chrROM8cnt,header.mirroring,header.BattRAM,header.trainer,header.FourScreenMode,
		   ((uint16_t)header.mapperMSB << 8) | header.mapperLSB,header.vsUnisystem,header.PlayChoice10,header.iNES2,header.prgRAM8cnt);
	
	if((((uint16_t)header.mapperMSB << 8) | header.mapperLSB) != 0) {
		std::cout << "Unsupported mapper: " << (int)(((uint16_t)header.mapperMSB << 8) | header.mapperLSB) << std::endl;
		return -1;
	}

	if(header.prgROM16cnt) PRGROM.resize(header.prgROM16cnt * 0x4000);
	//if(header.chrROM8cnt) CHRROM.resize(header.chrROM8cnt * 0x2000);
	CHRROM.resize(0x2000);
	if(header.prgRAM8cnt) PRGRAM.resize(header.prgRAM8cnt * 0x2000);
	//TODO add support for CHR RAM
	
	if(header.trainer) file.seekg(16+512); //Skip trainer if it exists
	for(unsigned int idx=0; idx < PRGROM.size(); ++idx) {
		if(file.eof())
			break;
		file.read((char*)&PRGROM[idx],1);
	}
	if(header.chrROM8cnt) {
		for(unsigned int idx=0; idx < CHRROM.size(); ++idx) {
			if(file.eof())
				break;
			file.read((char*)&CHRROM[idx],1);
		}
	}
	
	return 1;
}

}