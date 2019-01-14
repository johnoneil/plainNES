#include "mapper3.h"
#include "ppu.h"
#include "cpu.h"

Mapper3::Mapper3(GAMEPAK::iNES_Header header)
{
	horzMirroring = header.mirroring;
    VRAM.resize(0x800);
    PRGROM.resize(header.prgROM16cnt * 0x4000);
    CHR.resize(header.chrROM8cnt, std::vector<uint8_t>(0x2000, 0));
	CHRbank = 0;
}

uint8_t Mapper3::memGet(uint16_t addr)
{
    //No PRGRAM
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

void Mapper3::memSet(uint16_t addr, uint8_t val)
{
    //No PRG RAM. CHR bank select at 0x8000-0xFFFF
    if(addr >= 0x8000) {
        CHRbank = val % CHR.size();
    }
}

uint8_t Mapper3::PPUmemGet(uint16_t addr)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
        //Bankable CHR
		return CHR[CHRbank][addr];
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

void Mapper3::PPUmemSet(uint16_t addr, uint8_t val)
{
    //Mirror addresses higher than 0x3FFF
	addr %= 0x4000;
	if(addr < 0x2000) {
        //Bankable CHR
		CHR[CHRbank][addr] = val;
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

void Mapper3::loadData(GAMEPAK::iNES_Header header, std::ifstream &file)
{
	for(unsigned int idx=0; idx < PRGROM.size(); ++idx) {
		if(file.eof())
			break;
		file.read((char*)&PRGROM[idx],1);
	}

	for(unsigned int bank=0; bank < header.chrROM8cnt; ++bank) {
		for(unsigned int idx=0; idx < CHR[bank].size(); ++idx) {
			if(file.eof())
				break;
			file.read((char*)&CHR[bank][idx],1);
		}
	}
}