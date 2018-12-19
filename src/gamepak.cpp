#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include "gamepak.h"
#include "PPU.h"
#include "mapper.h"
#include "mapper0.h"
#include "mapper1.h"

namespace GAMEPAK {

Mapper *mapper;
const char* headerName = "NES\x1A";
iNES_Header header;

int init(std::ifstream &file) {
	char headerdata[16];
	uint8_t mapperNum;

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

	mapperNum = ((uint16_t)header.mapperMSB << 8) | header.mapperLSB;
	
	//printf("PRG16Cnt: %i\nCHR8Cnt: %i\nMirroring: %i\nBattRAM: %i\nTrainer: %i\nFour Screen Mode: %i\nMapper: %i\nvsUnisystem: %i\nPlayChoice10: %i\niNES2: %i\nPRGRAM8Cnt: %i\n",
	//	   header.prgROM16cnt,header.chrROM8cnt,header.mirroring,header.BattRAM,header.trainer,header.FourScreenMode,
	//	   mapperNum,header.vsUnisystem,header.PlayChoice10,header.iNES2,header.prgRAM8cnt);
	
	switch(mapperNum) {
		case 0: mapper = new Mapper0(header); break;
		case 1: mapper = new Mapper1(header); break;
		default: mapper = NULL;
	}

	if(mapper == NULL) {
		std::cout << "Unsupported mapper: " << mapperNum << std::endl;
		return -1;
	}

	mapper->loadData(header, file);
	
	return 1;
}

uint8_t CPUmemGet(uint16_t addr) {
	return mapper->memGet(addr);
}

void CPUmemSet(uint16_t addr, uint8_t val) {
	mapper->memSet(addr, val);
}

uint8_t PPUmemGet(uint16_t addr)
{
	return mapper->PPUmemGet(addr);
}

void PPUmemSet(uint16_t addr, uint8_t val)
{
	mapper->PPUmemSet(addr, val);
}

}