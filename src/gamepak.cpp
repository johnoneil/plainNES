#include "gamepak.h"
#include "ppu.h"
#include "Mapper/mapper.h"
#include "Mapper/mapper0.h"
#include "Mapper/mapper1.h"
#include "Mapper/mapper2.h"
#include "Mapper/mapper3.h"
#include "Mapper/mapper4.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <array>
#include <math.h>
#include <cmath>

namespace GAMEPAK {

Mapper *mapper;
const char* headerName = "NES\x1A";
ROMInfo romInfo = {0};
long mapperNum = 0;
int submapperNum = 0;

int loadROM(std::ifstream &file) {
	std::array<char, 16> headerdata;
	uint8_t mapperNum;

	file.seekg(0, file.end);
	long fileSize = file.tellg();
	file.seekg(0, file.beg);

	if(fileSize < 16) {
		std::cerr << "Invalid file" << std::endl;
		return 1;
	}

	file.read(headerdata.data(),16);
	
	if(memcmp(headerName,headerdata.data(),4) != 0) {
		std::cout << "Invalid file format\nIS: ";
		for(int i=0; i<4; i++)
			std::cout << std::hex << int(headerdata[i]) << " ";
		std::cout << "\nSB: ";
		for(int i=0; i<4; i++)
			std::cout << std::hex << int(headerdata[i]) << " ";
		std::cout << std::endl;
		return 1;
	}
	if(((headerdata[7] >> 2) & 0x03) == 2) {
		//iNES2 header
		iNES2_Header header;
		memcpy(&header, headerdata.data(), sizeof(header));

		//Check if game console type is currently supported
		if(header.consoleType != 0) {
			switch(header.consoleType) {
				case 1:
					std::cerr << "Nintendo Vs. System not supported" << std::endl;
					break;
				case 2:
					std::cerr << "Nintendo Playchoice 10 not supported" << std::endl;
					break;
				default:
					std::cerr << "Unsupported console type" << std::endl;
			}
			return 1;
		}
		//Check timing mode
		if(header.timingMode != 0) {
			std::cerr << "Only NTSC games supported" << std::endl;
			return 1;
		}

		romInfo.mirroringMode = header.mirroring;
		romInfo.batteryPresent = header.BattRAM;
		romInfo.trainer = header.trainer;
		romInfo.fourScreenMode = header.FourScreenMode;
		if(header.PRGROM_MSB == 0xF) //Use exponent-multiplier notation
			romInfo.PRGROMsize = std::pow((header.PRGROM_LSB >> 2),2) * ((header.PRGROM_LSB & 3) * 2 + 1);
		else //Use 16 KiB units
			romInfo.PRGROMsize = ((header.PRGROM_MSB << 8) | header.PRGROM_LSB) * 0x4000;
		if(header.CHRROM_MSB == 0xF) //Use exponent-multiplier notation
			romInfo.CHRROMsize = std::pow((header.CHRROM_LSB >> 2),2) * ((header.CHRROM_LSB & 3) * 2 + 1);
		else //Use 16 KiB units
			romInfo.CHRROMsize = ((header.CHRROM_MSB << 8) | header.CHRROM_LSB) * 0x2000;
		if(header.PRGRAM_shiftcnt == 0)
			romInfo.PRGRAMsize = 0;
		else
			romInfo.PRGRAMsize = 64 << header.PRGRAM_shiftcnt;
		if(header.PRGNVRAM_shiftcnt == 0)
			romInfo.PRGNVRAMsize = 0;
		else
			romInfo.PRGNVRAMsize = 64 << header.PRGNVRAM_shiftcnt;
		if(header.CHRRAM_shiftcnt == 0)
			romInfo.CHRRAMsize = 0;
		else
			romInfo.CHRRAMsize = 64 << header.CHRRAM_shiftcnt;
		if(header.CHRNVRAM_shiftcnt == 0)
			romInfo.CHRNVRAMsize = 0;
		else
			romInfo.CHRNVRAMsize = 64 << header.CHRNVRAM_shiftcnt;
		
		mapperNum = (header.mapperNib3 << 8) | (header.mapperNib2 << 4) | header.mapperNib1;
		submapperNum = header.submapper;
		romInfo.iNESversion = 2;
	}
	else {
		//iNES header
		iNES_Header header;
		memcpy(&header, headerdata.data(), sizeof(header));

		//Check if game console type is currently supported
		if(header.consoleType != 0) {
			switch(header.consoleType) {
				case 1:
					std::cerr << "Nintendo Vs. System not supported" << std::endl;
					break;
				case 2:
					std::cerr << "Nintendo Playchoice 10 not supported" << std::endl;
					break;
				default:
					std::cerr << "Unsupported console type" << std::endl;
			}
			return 1;
		}

		romInfo.mirroringMode = header.mirroring;
		romInfo.batteryPresent = header.BattRAM;
		romInfo.trainer = header.trainer;
		romInfo.fourScreenMode = header.FourScreenMode;
		romInfo.PRGROMsize = header.PRGROM_Size * 0x4000;
		romInfo.CHRROMsize = header.CHRROM_Size * 0x2000;
		romInfo.PRGRAMsize = header.PRGRAM_Size;
		if(romInfo.PRGRAMsize == 0) romInfo.PRGRAMsize = 0x2000;
		mapperNum = (header.mapperNib2 << 4) | header.mapperNib1;
		romInfo.iNESversion = 1;
	}

	if((romInfo.PRGROMsize + romInfo.CHRROMsize) > (fileSize - 16 - ((romInfo.trainer) ? 512 : 0))) {
		std::cerr << "File size smaller than expected for defined program size. Unable to load." << std::endl;
		std::cerr << "File size: " << fileSize << " Expected: "
				  << (16 + romInfo.PRGROMsize + romInfo.CHRROMsize + ((romInfo.trainer) ? 512 : 0)) << std::endl;
		return 1;
	}
	else if((romInfo.PRGROMsize + romInfo.CHRROMsize) < (fileSize - 16 - ((romInfo.trainer) ? 512 : 0))) {
		std::cout << "File size larger than expected for defined program size.\n"
				  << "Misc ROM area not currently supported, and will be ignored." << std::endl;
	}
	
	switch(mapperNum) {
		case 0: mapper = new Mapper0(romInfo, file); break;
		case 1: mapper = new Mapper1(romInfo, file); break;
		case 2: mapper = new Mapper2(romInfo, file); break;
		case 3: mapper = new Mapper3(romInfo, file); break;
		case 4: mapper = new Mapper4(romInfo, file); break;
		default:
			std::cerr << "Unsupported mapper: " << (int)mapperNum << std::endl;
			return 1;
	}
	
	return 0;
}

void powerOn() {
	mapper->powerOn();
}

void reset()
{
	mapper->reset();
}

void CPUstep()
{
	mapper->CPUstep();
}

void PPUstep()
{
	mapper->PPUstep();
}

uint8_t CPUmemGet(uint16_t addr, bool peek) {
	return mapper->memGet(addr, peek);
}

void CPUmemSet(uint16_t addr, uint8_t val) {
	mapper->memSet(addr, val);
}

uint8_t PPUmemGet(uint16_t addr, bool peek)
{
	return mapper->PPUmemGet(addr, peek);
}

void PPUmemSet(uint16_t addr, uint8_t val)
{
	mapper->PPUmemSet(addr, val);
}

void PPUbusAddrChanged(uint16_t newAddr)
{
	mapper->PPUbusAddrChanged(newAddr);
}

}