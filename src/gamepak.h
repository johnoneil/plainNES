#pragma once

#include <stdint.h>
#include <fstream>

namespace GAMEPAK {

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
}__attribute__((packed));

int loadROM(std::ifstream &file);
void powerOn();
void reset();

uint8_t CPUmemGet(uint16_t addr);
void CPUmemSet(uint16_t addr, uint8_t val);
uint8_t PPUmemGet(uint16_t addr);
void PPUmemSet(uint16_t addr, uint8_t val);


} //GAMEPAK