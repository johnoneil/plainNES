#pragma once

#include <stdint.h>
#include <fstream>

namespace GAMEPAK {

struct iNES_Header {
	char NES[4];				//bytes 0-3
	uint8_t PRGROM_Size;		//bytes 4
	uint8_t CHRROM_Size;		//bytes 5
	uint8_t mirroring : 1;		//bytes 6.0
	uint8_t BattRAM : 1;		//bytes 6.1
	uint8_t trainer : 1;		//bytes 6.2
	uint8_t FourScreenMode : 1;	//bytes 6.3
	uint8_t mapperNib1 : 4;		//bytes 6.4-7
	uint8_t consoleType : 2;	//bytes 7.0-1
	uint8_t iNES2 : 2;			//bytes 7.2-3
	uint8_t mapperNib2 : 4;		//bytes 7.4-7
	uint8_t PRGRAM_Size;		//bytes 8
}__attribute__((packed));

struct iNES2_Header {
	char NES[4];					//bytes 0-3
	uint8_t PRGROM_LSB;				//bytes 4
	uint8_t CHRROM_LSB;				//bytes 5
	uint8_t mirroring : 1;			//bytes 6.0
	uint8_t BattRAM : 1;			//bytes 6.1
	uint8_t trainer : 1;			//bytes 6.2
	uint8_t FourScreenMode : 1;		//bytes 6.3
	uint8_t mapperNib1 : 4;			//bytes 6.4-7
	uint8_t consoleType : 2;		//bytes 7.0-1
	uint8_t iNES2 : 2;				//bytes 7.2-3
	uint8_t mapperNib2 : 4;			//bytes 7.4-7
	uint8_t mapperNib3 : 4;			//bytes 8.0-3
	uint8_t submapper : 4;			//bytes 8.4-7
	uint8_t PRGROM_MSB : 4;			//bytes 9.0-3
	uint8_t CHRROM_MSB : 4;			//bytes 9.4-7
	uint8_t PRGRAM_shiftcnt : 4;	//bytes 10.0-3
	uint8_t PRGNVRAM_shiftcnt : 4;	//bytes 10.4-7
	uint8_t CHRRAM_shiftcnt : 4;	//bytes 11.0-3
	uint8_t CHRNVRAM_shiftcnt : 4;	//bytes 11.4-7
	uint8_t timingMode : 2;			//bytes 12.0-1
	uint8_t junkByte12 : 6;			//bytes 12.2-7
	uint8_t PPU_Console_Type : 4;	//bytes 13.0-3
	uint8_t vsHardwareType : 4;		//bytes 13.4-7
	uint8_t miscRomCnt	: 2;		//bytes 14.0-1
	uint8_t junkByte14 : 6;			//bytes 14.2-7
	uint8_t defExpDevice : 6;		//bytes 15.0-5
	uint8_t junkByte15 : 2;			//bytes 15.6-7
}__attribute__((packed));

struct ROMInfo {
	bool trainer;
	long PRGROMsize;
	long CHRROMsize;
	long PRGRAMsize;
	long PRGNVRAMsize;
	long CHRRAMsize;
	long CHRNVRAMsize;
	uint8_t mirroringMode;
	bool batteryPresent;
	bool fourScreenMode;
	uint8_t iNESversion;
};

int loadROM(std::ifstream &file);
void powerOn();
void reset();
void step();

uint8_t CPUmemGet(uint16_t addr);
void CPUmemSet(uint16_t addr, uint8_t val);
uint8_t PPUmemGet(uint16_t addr);
void PPUmemSet(uint16_t addr, uint8_t val);


} //GAMEPAK