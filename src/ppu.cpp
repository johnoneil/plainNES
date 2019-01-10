#include "ppu.h"
#include "gamepak.h"
#include "cpu.h"
#include "utils.h"
#include <iostream>
#include <cstring>

namespace PPU {

unsigned int scanline, dot; //Dots are also called cycles and can be considered the pixel column
unsigned long frame;
unsigned long long ppuClock;
uint8_t pixelMap[240*256];
bool frameReady;


////////////////////////////////////////////////////
/////Internal registers, memory, and flags//////////
////////////////////////////////////////////////////

struct VRAM {
	union {
		uint16_t value;
		BitWorker<0, 5, uint16_t> coarseX;
		BitWorker<5, 5, uint16_t> coarseY;
		BitWorker<10, 2, uint16_t> NTsel;
		BitWorker<12, 3, uint16_t> fineY;
		BitWorker<8, 6, uint16_t> addrWrite1;
		BitWorker<14, 1, uint16_t> bit14;
		BitWorker<0, 8, uint16_t> addrWrite2;
		BitWorker<10, 1, uint16_t> bit10;
		BitWorker<11, 4, uint16_t> upperY;
	};
}  currVRAM_addr, tempVRAM_addr;
/*union VRAMreg {
	uint16_t raw;
	struct __attribute__ ((__packed__)) ctrlVals {
		uint8_t coarseX : 5;
		uint8_t coarseY : 5;
		uint8_t NTsel : 2;
		uint8_t fineY : 3;
		uint8_t junk : 1; //Junk bit
	} v;
} currVRAM_addr, tempVRAM_addr; //v and t in nesdev wiki*/
//uint16_t currVRAM_addr, tempVRAM_addr; //v and t in nesdev wiki
uint8_t fineXscroll;	//x in nesdev wiki
bool writeToggle;		//w in nesdev wiki
uint16_t BGtiledata_upper, BGtiledata_lower;
uint8_t BGattri_upper, BGattri_lower;

//Background latches and shift registers
uint8_t NTlatch, ATlatch, BGLlatch, BGHlatch;
uint16_t ATshiftL, ATshiftH;
uint16_t BGshiftL, BGshiftH;

//Sprite memory, latches, shift registers, and counters
uint8_t oam_data[256];
uint8_t oam_sec[32]; 
uint8_t sprite_shiftL[8];
uint8_t sprite_shiftH[8];
uint8_t spriteL[8];
uint8_t spriteCounter[8];
bool spr0onNextLine, spr0onLine;

uint8_t VRAM_buffer;
uint8_t paletteRAM[32]; //Internal palette control RAM. Can't be mapped

//PPUCTRL flags
//nametable select modifies part of tempVRAM_addr
bool NMIenable, spriteSize, backgroundTileSel, spriteTileSel, incrementMode;

//PPUMASK
bool greyscale, showleftBG, showleftSpr, showBG, showSpr, emphRed, emphGrn, emphBlu;
bool rendering;

//PPUSTATUS
bool sprOverflow, spr0hit, vblank;

//OAMADDR
uint8_t OAMaddr;


////////////////////////////////////////////////////
/////////////////// Functions //////////////////////
////////////////////////////////////////////////////

void init()
{
	scanline = 241; //To be consistant with nintendulator
	dot = 0;
	frame = 0;
	ppuClock = 0;
	frameReady = false;
	sprOverflow = spr0hit = vblank = writeToggle = false;

	regSet(0x2000,0);
	regSet(0x2001,0);
	regSet(0x2003,0);
	regSet(0x2005,0);
	regSet(0x2005,0);
	regSet(0x2006,0);
	regSet(0x2006,0);

}

void step()
{
	//Check for skipped cycle. Occurs at 0,0 on odd frames when rendering enabled
	if((scanline == 261) && (dot == 339) && (showBG || showSpr) && (frame % 2 == 1))
		dot++;

	if(scanline == 241 && dot == 1) {
		vblank = true;
		if(NMIenable)
			CPU::triggerNMI();
	}
	else if(scanline == 261 && dot == 1) {
		vblank = spr0hit = sprOverflow = false;
	}

	if(scanline <= 239 || scanline == 261) {
		renderPixel();
		if(rendering) renderFrameStep();
	}

	++dot;
	if(dot >= 341) {
		dot = 0;
		++scanline;
	}

	if(scanline >= 262) {
		++frame;
		scanline = 0;
		frameReady = true;
	}

	++ppuClock;
}

uint8_t regGet(uint16_t addr)
{
	switch(addr) {
		case 0x2000: //PPUCTRL
			//Write only
			return 0;
			break;
		case 0x2001: //PPUMASK
			//Write only
			return 0;
			break;
		case 0x2002: //PPUSTATUS
			{
			uint8_t status = (vblank << 7) | (spr0hit << 6) | (sprOverflow << 5);
			vblank = 0;
			writeToggle = 0;
			return status;
			}
			break;
			
		case 0x2003: //OAMADDR
			//Write only
			return 0;
			break;

		case 0x2004: //OAMDATA
			return oam_data[OAMaddr];
			break;

		case 0x2005: //PPUSCROLL
			//Write only
			return 0;
			break;

		case 0x2006: //PPUADDR
			//Write only
			return 0;
			break;

		case 0x2007: //PPUDATA
			{
			//TODO: Implement special behavior during rendering
			uint8_t data;
			//For normal VRAM, reads work this way: VRAM ---> buffer ---> CPU
			//For palette, reads work this way: Palette ---> CPU  |  VRAM(palette addr - 0x1000) ---> buffer
			//Can directly access palette since never mapped
			if((currVRAM_addr.value % 0x4000) >= 0x3F00) {	//Palette read
				data = getPalette(currVRAM_addr.value % 0x20);
				VRAM_buffer = GAMEPAK::PPUmemGet(currVRAM_addr.value - 0x1000);
			}
			else {
				data = VRAM_buffer;
				VRAM_buffer = GAMEPAK::PPUmemGet(currVRAM_addr.value);
			}
			if(incrementMode == 0) ++currVRAM_addr.value;
			else currVRAM_addr.value += 32;
			return data;
			}
			break;
		default:
			std::cout << "Invalid PPU register access" << std::endl;
			throw 2;
		return 0;
	}
}

void regSet(uint16_t addr, uint8_t val)
{
	switch(addr) {
		case 0x2000: //PPUCTRL
			NMIenable = (val >> 7) > 0;
			if(NMIenable && vblank)
				CPU::triggerNMI();
			spriteSize = (val & 0x20) > 0;
			backgroundTileSel = (val & 0x10) > 0;
			spriteTileSel = (val & 0x08) > 0;
			incrementMode = (val & 0x04) > 0;
			tempVRAM_addr.NTsel = val & 3;
			//tempVRAM_addr = (tempVRAM_addr & ~0xC00) | ((val << 10) & 0xC00);
			break;
		case 0x2001: //PPUMASK
			greyscale = (val & 0x01) > 0;
			showleftBG = (val & 0x02) > 0;
			showleftSpr = (val & 0x04) > 0;
			showBG = (val & 0x08) > 0;
			showSpr = (val & 0x10) > 0;
			emphRed = (val & 0x20) > 0;
			emphGrn = (val & 0x40) > 0;
			emphBlu = (val >> 7) > 0;
			rendering = showBG | showSpr;
			break;
		case 0x2002: //PPUSTATUS
			break;
			
		case 0x2003: //OAMADDR
			OAMaddr = val;
			break;

		case 0x2004: //OAMDATA
			//Only write while not rendering
			if(rendering == false || ((scanline >= 240) && (scanline != 261))) {
				oam_data[OAMaddr] = val;
				++OAMaddr;
			}
			break;

		case 0x2005: //PPUSCROLL
			if(writeToggle == 0) {
				tempVRAM_addr.coarseX = ((val >> 3) & 0x001F);
				//tempVRAM_addr = (tempVRAM_addr & ~0x001F) | ((val >> 3) & 0x001F);
				fineXscroll = val & 0x07;
				writeToggle = true;
			}
			else {
				tempVRAM_addr.coarseY = (val >> 3) & 0x1F;
				tempVRAM_addr.fineY = val & 0x7;
				//tempVRAM_addr = (tempVRAM_addr & ~0x7000) | ((val & 0x07) << 12);
				//tempVRAM_addr = (tempVRAM_addr & ~0x03E0) | ((val & 0xF8) << 5);
				writeToggle = false;
			}
			break;

		case 0x2006: //PPUADDR
			if(writeToggle == 0) {
				tempVRAM_addr.bit14 = 0;
				tempVRAM_addr.addrWrite1 = val & 0x3F;
				//tempVRAM_addr = (tempVRAM_addr & 0xFF) | ((uint16_t)(val & 0x3F) << 8);
				writeToggle = true;
			}
			else {
				tempVRAM_addr.addrWrite2 = val;
				//tempVRAM_addr = (tempVRAM_addr & 0xFF00) | val;
				writeToggle = false;
				currVRAM_addr.value = tempVRAM_addr.value;
			}
			break;

		case 0x2007: //PPUDATA
			//TODO: Implement special behavior during rendering
			GAMEPAK::PPUmemSet(currVRAM_addr.value, val);
			if(incrementMode == 0) ++currVRAM_addr.value;
			else currVRAM_addr.value += 32;
			break;
		default:
			std::cout << "Invalid PPU register access" << std::endl;
			throw 2;
	}
}

uint8_t getPalette(uint16_t addr)
{
	addr %= 0x20;
	switch(addr) {
		case 0x10:
			addr = 0x00;
			break;
		case 0x14:
			addr = 0x04;
			break;
		case 0x18:
			addr = 0x08;
			break;
		case 0x1C:
			addr = 0x0C;
			break;
	}
	return paletteRAM[addr];
}

void setPalette(uint16_t addr, uint8_t val)
{
	addr %= 0x20;
	switch(addr) {
		case 0x10:
			addr = 0x00;
			break;
		case 0x14:
			addr = 0x04;
			break;
		case 0x18:
			addr = 0x08;
			break;
		case 0x1C:
			addr = 0x0C;
			break;
	}
	paletteRAM[addr] = val;
}

void renderFrameStep()
{
	spriteEval();
	//Perform action based on current dot
	switch(dot) {
		case 0:
			//Do nothing
			break;
		case 1 ... 256: case 321 ... 336:
			switch(dot % 8) {
				case 1:
					NTlatch = GAMEPAK::PPUmemGet(0x2000 + (currVRAM_addr.value & 0x0FFF));
					break;
				case 3:
					ATlatch = GAMEPAK::PPUmemGet( 0x23C0 + (currVRAM_addr.NTsel << 10) + ((currVRAM_addr.coarseX) / 4) + (((currVRAM_addr.coarseY) / 4) << 3));
					if((currVRAM_addr.coarseY % 4) >= 2) ATlatch >>= 4;
					if((currVRAM_addr.coarseX % 4) >= 2) ATlatch >>= 2;
					ATlatch &= 0x3;
					break;
				case 5:
					BGLlatch = GAMEPAK::PPUmemGet((NTlatch << 4) + currVRAM_addr.fineY + (backgroundTileSel ? 0x1000 : 0));
					break;
				case 7:
					BGHlatch = GAMEPAK::PPUmemGet((NTlatch << 4) + currVRAM_addr.fineY + (backgroundTileSel ? 0x1000 : 0) + 8);
					break;
				case 0:					
					if(dot != 256)
						incrementHorz();
					else
						incrementVert();
					//Load latches into shift registers every 8 cycles
					//Treating AT shift registers as 16 bit makes things easier
					BGshiftL = (BGshiftL & 0xFF00) | BGLlatch;
					BGshiftH = (BGshiftH & 0xFF00) | BGHlatch;
					ATshiftL = (ATshiftL & 0xFF00) | (((ATlatch & 1) > 0) ? 0xFF : 0);
					ATshiftH = (ATshiftH & 0xFF00) | (((ATlatch & 2) > 0) ? 0xFF : 0);
					break;
			}
			break;
		case 257: {
			//hori(v) = hori(t)
			currVRAM_addr.coarseX = tempVRAM_addr.coarseX;
			currVRAM_addr.bit10 = tempVRAM_addr.bit10;
		}
			break;
		case 280 ... 304:
			//Do nothing if not pre-render
			if(scanline == 261) {
				//vert(v) = vert(t) each dot
				currVRAM_addr.coarseY = tempVRAM_addr.coarseY;
				currVRAM_addr.upperY = tempVRAM_addr.upperY;
			}
			break;
		case 337: case 339:
			//Fetch unused NT byte
			GAMEPAK::PPUmemGet(0x2000 + (currVRAM_addr.value & 0x0FFF));
			break;
	}
}

void spriteEval()
{
	if(scanline == 261) return;
	switch(dot) {
		case 1:
			//Clear sec OAM
			memset(oam_sec, 0xFF, 32);
			break;
		case 65:
			{
			//Sprite eval
			int oam_sec_idx = 0;
			int n, m;
			spr0onNextLine = false;
			for(n=0; n<64; ++n) {
				if(oam_sec_idx >= 32) break;
				unsigned int yCoord = oam_data[n*4];
				unsigned int yMax = yCoord + 8;
				if(spriteSize) yMax += 8;
				if(scanline >= yCoord && scanline < yMax) {
					if(n == 0) spr0onNextLine = true;
					for(m=0; m<4; ++m)
						oam_sec[oam_sec_idx+m] = oam_data[n*4+m];
					oam_sec_idx += 4;
				}
			}
			//Simulates buggy overflow behavior
			m = 0;
			while(n < 64) {
				unsigned int yCoord = oam_data[n*4+m];
				unsigned int yMax = yCoord + 8;
				if(spriteSize) yMax += 8;
				if(scanline >= yCoord && scanline < yMax) {
					sprOverflow = true;
					break;
					//NES would eval all OAM sprites, but we'll stop here
				}
				++n;
				++m;
				if(m >= 4) m = 0;
			}
			}
			break;
		case 257:
			//Fetch sprite data
			spr0onLine = spr0onNextLine;
			for(int i = 0; i<8; ++i) {
				uint8_t yPos = scanline - oam_sec[i*4];
				uint16_t addr = oam_sec[i*4 + 1];
				spriteL[i] = oam_sec[i*4 + 2];
				spriteCounter[i] = oam_sec[i*4 + 3];
				if(spriteSize == 0) { //8x8 bit sprite
					//Check if flipped vertically
					if((spriteL[i] & 0x80) > 0) yPos = 7 - yPos;
					addr = (addr << 4) + yPos;
					if(spriteTileSel) addr += 0x1000;
					sprite_shiftL[i] = GAMEPAK::PPUmemGet(addr);
					sprite_shiftH[i] = GAMEPAK::PPUmemGet(addr + 8);
				}
				else { //8x16 bit sprite
					if((spriteL[i] & 0x80) > 0) yPos = 15 - yPos;
					if((addr & 1) == 0)
						addr = (addr << 4) + yPos;
					else
						addr = ((addr & 0xFE) << 4) + 0x1000 + yPos;
					sprite_shiftL[i] = GAMEPAK::PPUmemGet(addr);
					sprite_shiftH[i] = GAMEPAK::PPUmemGet(addr + 16);
				}
			}
			break;
	}
}

void renderPixel()
{
	uint8_t BGpixelColor, SPRpixelColor, pixelColor; //Which color to use from palette
	BGpixelColor = SPRpixelColor = pixelColor = 0;
	bool sprPriority = false;
	bool usingSpr0 = false;
	if(scanline < 240 && dot > 0 && dot <= 256) {
		if(rendering) {
			if(showBG) {
				BGpixelColor = (BGshiftL >> (15-fineXscroll)) & (1);
				BGpixelColor |= (BGshiftH >> (14-fineXscroll)) & (2);
				BGpixelColor |= (ATshiftL >> (13-fineXscroll)) & (4);
				BGpixelColor |= (ATshiftH >> (12-fineXscroll)) & (8);
			}
			if(showSpr) {
				for(int i = 0; i<8; ++i) {
					if(spriteCounter[i] > 0) {
						--spriteCounter[i];
						continue;
					}
					if(SPRpixelColor == 0) { //Still looking for a sprite pixel
						uint8_t currSprColor = 0;
						if((spriteL[i] & 0x40) == 0) { //Not flipped horizontally
							currSprColor = (sprite_shiftL[i] >> 7) | ((sprite_shiftH[i] >> 6) & 2);
							sprite_shiftL[i] <<= 1;
							sprite_shiftH[i] <<= 1;
						}
						else {
							currSprColor = (sprite_shiftL[i] & 1) | ((sprite_shiftH[i] & 1) << 1);
							sprite_shiftL[i] >>= 1;
							sprite_shiftH[i] >>= 1;
						}
						if(currSprColor == 0) continue; //Transparent, so we can move on
						sprPriority = (spriteL[i] & 0x20) == 0;
						SPRpixelColor = ((spriteL[i] & 3) << 2) + currSprColor;
						if(SPRpixelColor != 0 && i == 0 && spr0onLine) {
							usingSpr0 = true;
						}
						else {
							usingSpr0 = false;
						}
					}
				}
			}

			//Add 16 for sprite colors to ensure we pull from sprite portion of palette memory
			if(BGpixelColor == 0)
				pixelColor = SPRpixelColor + 16;
			else if(SPRpixelColor == 0)
				pixelColor = BGpixelColor;
			else if(sprPriority)
				pixelColor = SPRpixelColor + 16;
			else
				pixelColor = BGpixelColor;

			if(usingSpr0 && BGpixelColor != 0 && SPRpixelColor != 0)
				spr0hit = true;

			if((pixelColor & 3) == 0) pixelColor = 0; //Set to universal background color

			pixelMap[scanline*256 + dot - 1] = getPalette(0x3F00 + (uint16_t)pixelColor);
		}
		else {
			//TODO allow feature for color to be chosen by current VRAM address
			pixelMap[scanline*256 + dot - 1] = getPalette(0x3F00 + (uint16_t)pixelColor);
		}
	}
	
	if(dot > 0 && dot < 337 && (dot < 257 || dot > 320)) {
		//Shift registers
		BGshiftL <<= 1;
		BGshiftH <<= 1;
		ATshiftL <<= 1;
		ATshiftH <<= 1;
	}
}

void incrementHorz()
{
	//Will wrap around when hitting edge of nametable space
	/*uint8_t coarseX = currVRAM_addr & 0x1F;
	coarseX = (coarseX+1) & 0x1F;
	currVRAM_addr = (currVRAM_addr & ~0x1F) | (coarseX);
	if((coarseX) == 0)
		currVRAM_addr ^= 0x0400;*/
	++currVRAM_addr.coarseX;
	if(currVRAM_addr.coarseX == 0)
		currVRAM_addr.value ^= 0x0400; //Swap NT bit
}

void incrementVert()
{
	++currVRAM_addr.fineY;
	if(currVRAM_addr.fineY == 0) { //Overflows into coarseY
		++currVRAM_addr.coarseY;
		if(currVRAM_addr.coarseY == 0) {
			currVRAM_addr.value ^= 0x0800; //Swap NT bit
		}
	}
}

std::array<std::array<uint8_t, 16*16*64>, 2> getPatternTableBuffers() //Used in displaying pattern tables for debugging
{
	std::array<uint8_t,4> palette;
	for(int i=0; i<4; ++i)
		palette[i] = getPalette(i);
	
	std::array<std::array<uint8_t, 16*16*64>, 2> PTpixelmap;
	uint16_t addr, tilerowL, tilerowH;
	for(int pixelRow=0; pixelRow<16*8; ++pixelRow) {
		for(int tileCol=0; tileCol<16; ++tileCol) {
			addr = ((pixelRow/8) << 8) | (tileCol << 4) | (pixelRow % 8);
			tilerowL = GAMEPAK::PPUmemGet(addr);
			tilerowH = GAMEPAK::PPUmemGet(addr+8);
			for(int pixelCol=0; pixelCol<8; ++pixelCol) {
				//uint8_t value = (((tilerowH >> (7-pixelCol))&1) << 1) | ((tilerowL >> (7-pixelCol))&1);
				uint8_t value = palette[(((tilerowH >> (7-pixelCol))&1) << 1) | ((tilerowL >> (7-pixelCol))&1)];
				PTpixelmap[0][pixelRow*16*8+tileCol*8+pixelCol] = value;
			}
		}
	}
	for(int pixelRow=0; pixelRow<16*8; ++pixelRow) {
		for(int tileCol=0; tileCol<16; ++tileCol) {
			addr = (((pixelRow/8) << 8) | (tileCol << 4) | (pixelRow % 8)) + 0x1000;
			tilerowL = GAMEPAK::PPUmemGet(addr);
			tilerowH = GAMEPAK::PPUmemGet(addr+8);
			for(int pixelCol=0; pixelCol<8; ++pixelCol) {
				uint8_t value = palette[(((tilerowH >> (7-pixelCol))&1) << 1) | ((tilerowL >> (7-pixelCol))&1)];
				PTpixelmap[1][pixelRow*16*8+tileCol*8+pixelCol] = value;
			}
		}
	}
	return PTpixelmap;
}

uint8_t* getPixelMap()
{
	return pixelMap;
}

bool isframeReady() {
	return frameReady;
}

void setframeReady(bool set) {
	frameReady = set;
}


}