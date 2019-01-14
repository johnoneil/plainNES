#include "mapper0.h"
#include "ppu.h"
#include "cpu.h"

Mapper0::Mapper0(GAMEPAK::iNES_Header header)
{
	horzMirroring = header.mirroring;
    VRAM.resize(0x800);
    PRGROM.resize(header.prgROM16cnt * 0x4000);
    CHRROM.resize(0x2000);
}

uint8_t Mapper0::memGet(uint16_t addr)
{
    //No PRG RAM
    if(addr >= 0x6000 && addr < 0x8000) {
		return CPU::busVal;
	}
	else if(addr >= 0x8000) {
		return PRGROM[(addr - 0x8000) % PRGROM.size()];
	}
	else {
		return CPU::busVal;
	}
}

void Mapper0::memSet(uint16_t addr, uint8_t val)
{
    //No PRG RAM
    return;
}

uint8_t Mapper0::PPUmemGet(uint16_t addr)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
		return CHRROM[addr];
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(horzMirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				return VRAM[addr % 0x0400];
			else
				return VRAM[0x0400 + addr % 0x0400];
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				return VRAM[addr - 0x2000];
			else
				return VRAM[addr - 0x2800];
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
		CHRROM[addr] = val;
	}
	else if(addr < 0x3F00) {
		addr = 0x2000 + addr % 0x1000;
		if(horzMirroring == 0) { //Horizontal mirroring
			if(addr < 0x2800)
				VRAM[addr % 0x0400] = val;
			else
				VRAM[0x0400 + addr % 0x0400] = val;
		}
		else {	//Vertical mirroring
			if(addr < 0x2800)
				VRAM[addr - 0x2000] = val;
			else
				VRAM[addr - 0x2800] = val;
		}
	}
	else if(addr < 0x4000) {
		//Internal to PPU. Never mapped.
		PPU::setPalette((uint8_t)(addr % 0x20), val);
	}
}

void Mapper0::loadData(GAMEPAK::iNES_Header header, std::ifstream &file)
{
	for(unsigned int idx=0; idx < PRGROM.size(); ++idx) {
		if(file.eof())
			break;
		file.read((char*)&PRGROM[idx],1);
	}

	for(unsigned int idx=0; idx < CHRROM.size(); ++idx) {
		if(file.eof())
			break;
		file.read((char*)&CHRROM[idx],1);
	}
}