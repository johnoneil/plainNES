#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "io.h"
#include "gamepak.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <array>

namespace CPU {

uint16_t PC;
uint8_t SP, A, X, Y;
bool NMI_request, IRQ_request, NMI_triggered, IRQ_triggered, interruptOccured;
unsigned long long cycle;
bool alive;

union StatusReg {
	uint8_t raw;
	struct StatusBits {
		bool C : 1; //Carry
		bool Z : 1; //Zero
		bool I : 1; //Interrupt Disable
		bool D : 1; //Decimal Mode (not used on NES)
		bool B : 2; //Break
		bool V : 1; //Overflow
		bool N : 1; //Negative
	} b;
} P;
std::array<uint8_t, 2048> RAM;
uint8_t OAMDMA;

//Logging variables
uint16_t PC_init;
std::string opTxt;
std::stringstream initVals;
int memCnt;
std::array<uint8_t, 3> memVals;
std::ofstream logFile;
bool enableLogging;


uint8_t memGet(uint16_t addr) {
	//Logic for grabbing 8bit value at address
	//Handles mirroring, or which component to get data from
	//CPU memory is from 0000-1FFF, but 0800-1FFF is mirrored
	
	if(addr < 0x2000) {			//CPU 2k internal memory space or mirrored
		return RAM[addr % 0x800];
	}
	else if(addr < 0x4000) {	//PPU registers or mirrored
		return PPU::regGet(0x2000 + (addr % 8));
	}
	else if(addr < 0x4014) {	//APU registers
		return APU::regGet(addr);
	}
	else if(addr == 0x4014) {	//OAMDMA register
		return OAMDMA;
	}
	else if(addr < 0x4016) {	//APU registers
		return APU::regGet(addr);
	}
	else if(addr < 0x4018) {	//IO registers
		return IO::regGet(addr);
	}
	else if(addr < 0x4020) {
		return IO::regGet(addr);	//APU registers
	}
	else {
		return GAMEPAK::CPUmemGet(addr);	//Gamepak memory
	}
}

void memSet(uint16_t addr, uint8_t val) {
	//Logic for setting 8bit value at address
	//Handles mirroring, or which component to send data to
	
	if(addr < 0x2000) {			//CPU memory space or mirrored
		RAM[addr % 0x800] = val;
	}
	else if(addr < 0x4000) {	//PPU memory or mirrored
		PPU::regSet(0x2000 + (addr % 8), val);
	}
	else if(addr < 0x4014) {	//APU registers
		APU::regSet(addr, val);
	}
	else if(addr == 0x4014) {	//OAMDMA register
		OAMDMA = val;
		OAMDMA_write();
	}
	else if(addr < 0x4016) {	//APU registers
		APU::regSet(addr, val);
	}
	else if(addr < 0x4018) {	//IO registers
		IO::regSet(addr, val);
	}
	else if(addr < 0x4020) {
		APU::regSet(addr, val);	//APU registers
	}
	else {
		GAMEPAK::CPUmemSet(addr, val);	//Gamepak memory
	}
}


void init(bool logging) {
	if(logging) {
		enableLogging = true;
		logFile.open("log.txt",std::ios::trunc);
	}
	SP = 0xFD;
	P.raw = 0x34;
	PC = memGet(0xFFFC) | memGet(0xFFFD) << 8;
	PC_init = PC;
	NMI_request = NMI_triggered = false;
	IRQ_request = IRQ_triggered = false;
	interruptOccured = false;
	cycle = 0;
	for(int i = 0; i < 2048; ++i)
		RAM[i] = 0;
	alive = true;
}

void tick() {
	++cycle;
	PPU::step();
	PPU::step();
	PPU::step();
	APU::step();
}

long long getCycles()
{
	return cycle;
}

void step() {
	uint8_t opcode;
	if(NMI_triggered | IRQ_triggered) {
		interruptOccured = true;
		opcode = 0x00; //BRK
		tick();
	}
	else {
		if(enableLogging)
			getStateString();
		PC_init = PC;
		opcode = memGet(PC);
		memVals[0] = opcode;
		memCnt = 1; //Default logging value if no specific addressing function used
		++PC;
		tick();
	}
	switch (opcode) {
		case 0x69:
			if(enableLogging) opTxt = "ADC ";
			opADC(Immediate());
			break;
		case 0x65:
			if(enableLogging) opTxt = "ADC ";
			opADC(ZeroPage());
			break;
		case 0x75:
			if(enableLogging) opTxt = "ADC ";
			opADC(ZeroPageX());
			break;
		case 0x6D:
			if(enableLogging) opTxt = "ADC ";
			opADC(Absolute());
			break;
		case 0x7D:
			if(enableLogging) opTxt = "ADC ";
			opADC(AbsoluteX(READ));
			break;
		case 0x79:
			if(enableLogging) opTxt = "ADC ";
			opADC(AbsoluteY(READ));
			break;
		case 0x61:
			if(enableLogging) opTxt = "ADC ";
			opADC(IndirectX());
			break;
		case 0x71:
			if(enableLogging) opTxt = "ADC ";
			opADC(IndirectY(READ));
			break;
		case 0x93:
			if(enableLogging) opTxt = "AHX ";
			opAHX(IndirectY(WRITE));
			break;
		case 0x9F:
			if(enableLogging) opTxt = "AHX ";
			opAHX(AbsoluteY(WRITE));
			break;
		case 0x4B:
			if(enableLogging) opTxt = "ALR ";
			opALR(Immediate());
			break;
		case 0x0B:
			if(enableLogging) opTxt = "ANC ";
			opANC(Immediate());
			break;
		case 0x2B:
			if(enableLogging) opTxt = "ANC ";
			opANC(Immediate());
			break;	
		case 0x29:
			if(enableLogging) opTxt = "AND ";
			opAND(Immediate());
			break;
		case 0x25:
			if(enableLogging) opTxt = "AND ";
			opAND(ZeroPage());
			break;
		case 0x35:
			if(enableLogging) opTxt = "AND ";
			opAND(ZeroPageX());
			break;
		case 0x2D:
			if(enableLogging) opTxt = "AND ";
			opAND(Absolute());
			break;
		case 0x3D:
			if(enableLogging) opTxt = "AND ";
			opAND(AbsoluteX(READ));
			break;
		case 0x39:
			if(enableLogging) opTxt = "AND ";
			opAND(AbsoluteY(READ));
			break;
		case 0x21:
			if(enableLogging) opTxt = "AND ";
			opAND(IndirectX());
			break;
		case 0x31:
			if(enableLogging) opTxt = "AND ";
			opAND(IndirectY(READ));
			break;
		case 0x6B:
			if(enableLogging) opTxt = "ARR ";
			opARR(Immediate());
			break;
		case 0x0A:
			if(enableLogging) opTxt = "ASL ";
			memGet(PC); //Dummy read of PC
			opASL();
			break;
		case 0x06:
			if(enableLogging) opTxt = "ASL ";
			opASL(ZeroPage());
			break;
		case 0x16:
			if(enableLogging) opTxt = "ASL ";
			opASL(ZeroPageX());
			break;
		case 0x0E:
			if(enableLogging) opTxt = "ASL ";
			opASL(Absolute());
			break;
		case 0x1E:
			if(enableLogging) opTxt = "ASL ";
			opASL(AbsoluteX(READWRITE));
			break;
		case 0xCB:
			if(enableLogging) opTxt = "AXS ";
			opAXS(Immediate());
			break;
		case 0x90:
			if(enableLogging) opTxt = "BCC ";
			memGet(PC); //Dummy read of PC
			opBCC();
			break;
		case 0xB0:
			if(enableLogging) opTxt = "BCS ";
			memGet(PC); //Dummy read of PC
			opBCS();
			break;
		case 0xF0:
			if(enableLogging) opTxt = "BEQ ";
			memGet(PC); //Dummy read of PC
			opBEQ();
			break;
		case 0x24:
			if(enableLogging) opTxt = "BIT ";
			opBIT(ZeroPage());
			break;
		case 0x2C:
			if(enableLogging) opTxt = "BIT ";
			opBIT(Absolute());
			break;
		case 0x30:
			if(enableLogging) opTxt = "BMI ";
			memGet(PC); //Dummy read of PC
			opBMI();
			break;
		case 0xD0:
			if(enableLogging) opTxt = "BNE ";
			memGet(PC); //Dummy read of PC
			opBNE();
			break;
		case 0x10:
			if(enableLogging) opTxt = "BPL ";
			memGet(PC); //Dummy read of PC
			opBPL();
			break;	
		case 0x00:
			if(enableLogging) opTxt = "BRK ";
			memGet(PC); //Dummy read of PC
			opBRK();
			break;
		case 0x50:
			if(enableLogging) opTxt = "BVC ";
			memGet(PC); //Dummy read of PC
			opBVC();
			break;
		case 0x70:
			if(enableLogging) opTxt = "BVS ";
			memGet(PC); //Dummy read of PC
			opBVS();
			break;
		case 0x18:
			if(enableLogging) opTxt = "CLC ";
			memGet(PC); //Dummy read of PC
			opCLC();
			break;
		case 0xD8:
			if(enableLogging) opTxt = "CLD ";
			memGet(PC); //Dummy read of PC
			opCLD();
			break;
		case 0x58:
			if(enableLogging) opTxt = "CLI ";
			memGet(PC); //Dummy read of PC
			opCLI();
			break;
		case 0xB8:
			if(enableLogging) opTxt = "CLV ";
			memGet(PC); //Dummy read of PC
			opCLV();
			break;
		case 0xC9:
			if(enableLogging) opTxt = "CMP ";
			opCMP(Immediate());
			break;
		case 0xC5:
			if(enableLogging) opTxt = "CMP ";
			opCMP(ZeroPage());
			break;
		case 0xD5:
			if(enableLogging) opTxt = "CMP ";
			opCMP(ZeroPageX());
			break;
		case 0xCD:
			if(enableLogging) opTxt = "CMP ";
			opCMP(Absolute());
			break;
		case 0xDD:
			if(enableLogging) opTxt = "CMP ";
			opCMP(AbsoluteX(READ));
			break;
		case 0xD9:
			if(enableLogging) opTxt = "CMP ";
			opCMP(AbsoluteY(READ));
			break;
		case 0xC1:
			if(enableLogging) opTxt = "CMP ";
			opCMP(IndirectX());
			break;
		case 0xD1:
			if(enableLogging) opTxt = "CMP ";
			opCMP(IndirectY(READ));
			break;
		case 0xE0:
			if(enableLogging) opTxt = "CPX ";
			opCPX(Immediate());
			break;
		case 0xE4:
			if(enableLogging) opTxt = "CPX ";
			opCPX(ZeroPage());
			break;
		case 0xEC:
			if(enableLogging) opTxt = "CPX ";
			opCPX(Absolute());
			break;
		case 0xC0:
			if(enableLogging) opTxt = "CPY ";
			opCPY(Immediate());
			break;
		case 0xC4:
			if(enableLogging) opTxt = "CPY ";
			opCPY(ZeroPage());
			break;
		case 0xCC:
			if(enableLogging) opTxt = "CPY ";
			opCPY(Absolute());
			break;
		case 0xC3:
			if(enableLogging) opTxt = "DCP ";
			opDCP(IndirectX());
			break;
		case 0xC7:
			if(enableLogging) opTxt = "DCP ";
			opDCP(ZeroPage());
			break;
		case 0xCF:
			if(enableLogging) opTxt = "DCP ";
			opDCP(Absolute());
			break;
		case 0xD3:
			if(enableLogging) opTxt = "DCP ";
			opDCP(IndirectY(READWRITE));
			break;
		case 0xD7:
			if(enableLogging) opTxt = "DCP ";
			opDCP(ZeroPageX());
			break;
		case 0xDB:
			if(enableLogging) opTxt = "DCP ";
			opDCP(AbsoluteY(READWRITE));
			break;
		case 0xDF:
			if(enableLogging) opTxt = "DCP ";
			opDCP(AbsoluteX(READWRITE));
			break;	
		case 0xC6:
			if(enableLogging) opTxt = "DEC ";
			opDEC(ZeroPage());
			break;
		case 0xD6:
			if(enableLogging) opTxt = "DEC ";
			opDEC(ZeroPageX());
			break;
		case 0xCE:
			if(enableLogging) opTxt = "DEC ";
			opDEC(Absolute());
			break;
		case 0xDE:
			if(enableLogging) opTxt = "DEC ";
			opDEC(AbsoluteX(READWRITE));
			break;
		case 0xCA:
			if(enableLogging) opTxt = "DEX ";
			memGet(PC); //Dummy read of PC
			opDEX();
			break;
		case 0x88:
			if(enableLogging) opTxt = "DEY ";
			memGet(PC); //Dummy read of PC
			opDEY();
			break;
		case 0x49:
			if(enableLogging) opTxt = "EOR ";
			opEOR(Immediate());
			break;
		case 0x45:
			if(enableLogging) opTxt = "EOR ";
			opEOR(ZeroPage());
			break;
		case 0x55:
			if(enableLogging) opTxt = "EOR ";
			opEOR(ZeroPageX());
			break;
		case 0x4D:
			if(enableLogging) opTxt = "EOR ";
			opEOR(Absolute());
			break;
		case 0x5D:
			if(enableLogging) opTxt = "EOR ";
			opEOR(AbsoluteX(READ));
			break;
		case 0x59:
			if(enableLogging) opTxt = "EOR ";
			opEOR(AbsoluteY(READ));
			break;
		case 0x41:
			if(enableLogging) opTxt = "EOR ";
			opEOR(IndirectX());
			break;
		case 0x51:
			if(enableLogging) opTxt = "EOR ";
			opEOR(IndirectY(READ));
			break;
		case 0xE6:
			if(enableLogging) opTxt = "INC ";
			opINC(ZeroPage());
			break;
		case 0xF6:
			if(enableLogging) opTxt = "INC ";
			opINC(ZeroPageX());
			break;
		case 0xEE:
			if(enableLogging) opTxt = "INC ";
			opINC(Absolute());
			break;
		case 0xFE:
			if(enableLogging) opTxt = "INC ";
			opINC(AbsoluteX(READWRITE));
			break;
		case 0xE8:
			if(enableLogging) opTxt = "INX ";
			memGet(PC); //Dummy read of PC
			opINX();
			break;
		case 0xC8:
			if(enableLogging) opTxt = "INY ";
			memGet(PC); //Dummy read of PC
			opINY();
			break;
		case 0xE3:
			if(enableLogging) opTxt = "ISC ";
			opISC(IndirectX());
			break;
		case 0xE7:
			if(enableLogging) opTxt = "ISC ";
			opISC(ZeroPage());
			break;
		case 0xEF:
			if(enableLogging) opTxt = "ISC ";
			opISC(Absolute());
			break;
		case 0xF3:
			if(enableLogging) opTxt = "ISC ";
			opISC(IndirectY(READWRITE));
			break;
		case 0xF7:
			if(enableLogging) opTxt = "ISC ";
			opISC(ZeroPageX());
			break;
		case 0xFB:
			if(enableLogging) opTxt = "ISC ";
			opISC(AbsoluteY(READWRITE));
			break;
		case 0xFF:
			if(enableLogging) opTxt = "ISC ";
			opISC(AbsoluteX(READWRITE));
			break;	
		case 0x4C:
			if(enableLogging) opTxt = "JMP ";
			opJMP(Absolute());
			break;
		case 0x6C:
			if(enableLogging) opTxt = "JMP ";
			opJMP(Indirect());
			tick();
			tick();
			break;
		case 0x20:
			if(enableLogging) opTxt = "JSR ";
			opJSR(Absolute());
			break;
		case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
		case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
			if(enableLogging) opTxt = "KIL ";
			alive = false;
			break;
		case 0xBB:
			if(enableLogging) opTxt = "LAS ";
			opLAS(AbsoluteY(READ));
			break;
		case 0xA3:
			if(enableLogging) opTxt = "LAX ";
			opLAX(IndirectX());
			break;
		case 0xA7:
			if(enableLogging) opTxt = "LAX ";
			opLAX(ZeroPage());
			break;
		case 0xAB:
			if(enableLogging) opTxt = "LAX ";
			opLAX(Immediate());
			break;
		case 0xAF:
			if(enableLogging) opTxt = "LAX ";
			opLAX(Absolute());
			break;
		case 0xB3:
			if(enableLogging) opTxt = "LAX ";
			opLAX(IndirectY(READ));
			break;
		case 0xB7:
			if(enableLogging) opTxt = "LAX ";
			opLAX(ZeroPageY());
			break;
		case 0xBF:
			if(enableLogging) opTxt = "LAX ";
			opLAX(AbsoluteY(READ));
			break;
		case 0xA9:
			if(enableLogging) opTxt = "LDA ";
			opLDA(Immediate());
			break;
		case 0xA5:
			if(enableLogging) opTxt = "LDA ";
			opLDA(ZeroPage());
			break;
		case 0xB5:
			if(enableLogging) opTxt = "LDA ";
			opLDA(ZeroPageX());
			break;
		case 0xAD:
			if(enableLogging) opTxt = "LDA ";
			opLDA(Absolute());
			break;
		case 0xBD:
			if(enableLogging) opTxt = "LDA ";
			opLDA(AbsoluteX(READ));
			break;
		case 0xB9:
			if(enableLogging) opTxt = "LDA ";
			opLDA(AbsoluteY(READ));
			break;
		case 0xA1:
			if(enableLogging) opTxt = "LDA ";
			opLDA(IndirectX());
			break;
		case 0xB1:
			if(enableLogging) opTxt = "LDA ";
			opLDA(IndirectY(READ));
			break;
		case 0xA2:
			if(enableLogging) opTxt = "LDX ";
			opLDX(Immediate());
			break;
		case 0xA6:
			if(enableLogging) opTxt = "LDX ";
			opLDX(ZeroPage());
			break;
		case 0xB6:
			if(enableLogging) opTxt = "LDX ";
			opLDX(ZeroPageY());
			break;
		case 0xAE:
			if(enableLogging) opTxt = "LDX ";
			opLDX(Absolute());
			break;
		case 0xBE:
			if(enableLogging) opTxt = "LDX ";
			opLDX(AbsoluteY(READ));
			break;
		case 0xA0:
			if(enableLogging) opTxt = "LDY ";
			opLDY(Immediate());
			break;
		case 0xA4:
			if(enableLogging) opTxt = "LDY ";
			opLDY(ZeroPage());
			break;
		case 0xB4:
			if(enableLogging) opTxt = "LDY ";
			opLDY(ZeroPageX());
			break;
		case 0xAC:
			if(enableLogging) opTxt = "LDY ";
			opLDY(Absolute());
			break;
		case 0xBC:
			if(enableLogging) opTxt = "LDY ";
			opLDY(AbsoluteX(READ));
			break;
		case 0x4A:
			if(enableLogging) opTxt = "LSR ";
			memGet(PC); //Dummy read of PC
			opLSR();
			break;
		case 0x46:
			if(enableLogging) opTxt = "LSR ";
			opLSR(ZeroPage());
			break;
		case 0x56:
			if(enableLogging) opTxt = "LSR ";
			opLSR(ZeroPageX());
			break;
		case 0x4E:
			if(enableLogging) opTxt = "LSR ";
			opLSR(Absolute());
			break;
		case 0x5E:
			if(enableLogging) opTxt = "LSR ";
			opLSR(AbsoluteX(READWRITE));
			break;
		case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA:
			if(enableLogging) opTxt = "NOP ";
			opNOP(PC);
			break;
		case 0x80: case 0x82: case 0xC2: case 0xE2: case 0x89:
			if(enableLogging) opTxt = "NOP ";
			opNOP(Immediate());
			break;
		case 0x04: case 0x44: case 0x64:
			if(enableLogging) opTxt = "NOP ";
			opNOP(ZeroPage());
			break;
		case 0x0C:
			if(enableLogging) opTxt = "NOP ";
			opNOP(Absolute());
			break;
		case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
			if(enableLogging) opTxt = "NOP ";
			opNOP(AbsoluteX(READ));
			break;
		case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
			if(enableLogging) opTxt = "NOP ";
			opNOP(ZeroPageX());
			break;
		case 0x09:
			if(enableLogging) opTxt = "ORA ";
			opORA(Immediate());
			break;
		case 0x05:
			if(enableLogging) opTxt = "ORA ";
			opORA(ZeroPage());
			break;
		case 0x15:
			if(enableLogging) opTxt = "ORA ";
			opORA(ZeroPageX());
			break;
		case 0x0D:
			if(enableLogging) opTxt = "ORA ";
			opORA(Absolute());
			break;
		case 0x1D:
			if(enableLogging) opTxt = "ORA ";
			opORA(AbsoluteX(READ));
			break;
		case 0x19:
			if(enableLogging) opTxt = "ORA ";
			opORA(AbsoluteY(READ));
			break;
		case 0x01:
			if(enableLogging) opTxt = "ORA ";
			opORA(IndirectX());
			break;
		case 0x11:
			if(enableLogging) opTxt = "ORA ";
			opORA(IndirectY(READ));
			break;
		case 0x48:
			if(enableLogging) opTxt = "PHA ";
			memGet(PC); //Dummy read of PC
			opPHA();
			break;
		case 0x08:
			if(enableLogging) opTxt = "PHP ";
			memGet(PC); //Dummy read of PC
			opPHP();
			break;
		case 0x68:
			if(enableLogging) opTxt = "PLA ";
			memGet(PC); //Dummy read of PC
			opPLA();
			break;
		case 0x28:
			if(enableLogging) opTxt = "PLP ";
			memGet(PC); //Dummy read of PC
			opPLP();
			break;
		case 0x23:
			if(enableLogging) opTxt = "RLA ";
			opRLA(IndirectX());
			break;
		case 0x27:
			if(enableLogging) opTxt = "RLA ";
			opRLA(ZeroPage());
			break;
		case 0x2F:
			if(enableLogging) opTxt = "RLA ";
			opRLA(Absolute());
			break;
		case 0x33:
			if(enableLogging) opTxt = "RLA ";
			opRLA(IndirectY(READWRITE));
			break;
		case 0x37:
			if(enableLogging) opTxt = "RLA ";
			opRLA(ZeroPageX());
			break;
		case 0x3B:
			if(enableLogging) opTxt = "RLA ";
			opRLA(AbsoluteY(READWRITE));
			break;
		case 0x3F:
			if(enableLogging) opTxt = "RLA ";
			opRLA(AbsoluteX(READWRITE));
			break;
		case 0x2A:
			if(enableLogging) opTxt = "ROL ";
			memGet(PC); //Dummy read of PC
			opROL();
			break;
		case 0x26:
			if(enableLogging) opTxt = "ROL ";
			opROL(ZeroPage());
			break;
		case 0x36:
			if(enableLogging) opTxt = "ROL ";
			opROL(ZeroPageX());
			break;
		case 0x2E:
			if(enableLogging) opTxt = "ROL ";
			opROL(Absolute());
			break;
		case 0x3E:
			if(enableLogging) opTxt = "ROL ";
			opROL(AbsoluteX(READWRITE));
			break;
		case 0x6A:
			if(enableLogging) opTxt = "ROR ";
			memGet(PC); //Dummy read of PC
			opROR();
			break;
		case 0x66:
			if(enableLogging) opTxt = "ROR ";
			opROR(ZeroPage());
			break;
		case 0x76:
			if(enableLogging) opTxt = "ROR ";
			opROR(ZeroPageX());
			break;
		case 0x6E:
			if(enableLogging) opTxt = "ROR ";
			opROR(Absolute());
			break;
		case 0x7E:
			if(enableLogging) opTxt = "ROR ";
			opROR(AbsoluteX(READWRITE));
			break;
		case 0x63:
			if(enableLogging) opTxt = "RRA ";
			opRRA(IndirectX());
			break;
		case 0x67:
			if(enableLogging) opTxt = "RRA ";
			opRRA(ZeroPage());
			break;
		case 0x6F:
			if(enableLogging) opTxt = "RRA ";
			opRRA(Absolute());
			break;
		case 0x73:
			if(enableLogging) opTxt = "RRA ";
			opRRA(IndirectY(READWRITE));
			break;
		case 0x77:
			if(enableLogging) opTxt = "RRA ";
			opRRA(ZeroPageX());
			break;
		case 0x7B:
			if(enableLogging) opTxt = "RRA ";
			opRRA(AbsoluteY(READWRITE));
			break;
		case 0x7F:
			if(enableLogging) opTxt = "RRA ";
			opRRA(AbsoluteX(READWRITE));
			break;	
		case 0x40:
			if(enableLogging) opTxt = "RTI ";
			memGet(PC); //Dummy read of PC
			opRTI();
			break;
		case 0x60:
			if(enableLogging) opTxt = "RTS ";
			memGet(PC); //Dummy read of PC
			opRTS();
			break;
		case 0x83:
			if(enableLogging) opTxt = "SAX ";
			opSAX(IndirectX());
			break;
		case 0x87:
			if(enableLogging) opTxt = "SAX ";
			opSAX(ZeroPage());
			break;
		case 0x8F:
			if(enableLogging) opTxt = "SAX ";
			opSAX(Absolute());
			break;
		case 0x97:
			if(enableLogging) opTxt = "SAX ";
			opSAX(ZeroPageY());
			break;
		case 0xE9:
			if(enableLogging) opTxt = "SBC ";
			opSBC(Immediate());
			break;
		case 0xE5:
			if(enableLogging) opTxt = "SBC ";
			opSBC(ZeroPage());
			break;
		case 0xEB:
			if(enableLogging) opTxt = "SBC ";
			opSBC(Immediate());
			break;
		case 0xF5:
			if(enableLogging) opTxt = "SBC ";
			opSBC(ZeroPageX());
			break;
		case 0xED:
			if(enableLogging) opTxt = "SBC ";
			opSBC(Absolute());
			break;
		case 0xFD:
			if(enableLogging) opTxt = "SBC ";
			opSBC(AbsoluteX(READ));
			break;
		case 0xF9:
			if(enableLogging) opTxt = "SBC ";
			opSBC(AbsoluteY(READ));
			break;
		case 0xE1:
			if(enableLogging) opTxt = "SBC ";
			opSBC(IndirectX());
			break;
		case 0xF1:
			if(enableLogging) opTxt = "SBC ";
			opSBC(IndirectY(READ));
			break;
		case 0x38:
			if(enableLogging) opTxt = "SEC ";
			memGet(PC); //Dummy read of PC
			opSEC();
			break;
		case 0xF8:
			if(enableLogging) opTxt = "SED ";
			memGet(PC); //Dummy read of PC
			opSED();
			break;
		case 0x78:
			if(enableLogging) opTxt = "SEI ";
			memGet(PC); //Dummy read of PC
			opSEI();
			break;
		case 0x9E:
			if(enableLogging) opTxt = "SHX ";
			opSHX(AbsoluteY(WRITE));
			break;
		case 0x9C:
			if(enableLogging) opTxt = "SHY ";
			opSHY(AbsoluteX(WRITE));
			break;
		case 0x03:
			if(enableLogging) opTxt = "SLO ";
			opSLO(IndirectX());
			break;
		case 0x07:
			if(enableLogging) opTxt = "SLO ";
			opSLO(ZeroPage());
			break;
		case 0x0F:
			if(enableLogging) opTxt = "SLO ";
			opSLO(Absolute());
			break;
		case 0x13:
			if(enableLogging) opTxt = "SLO ";
			opSLO(IndirectY(READWRITE));
			break;
		case 0x17:
			if(enableLogging) opTxt = "SLO ";
			opSLO(ZeroPageX());
			break;
		case 0x1B:
			if(enableLogging) opTxt = "SLO ";
			opSLO(AbsoluteY(READWRITE));
			break;
		case 0x1F:
			if(enableLogging) opTxt = "SLO ";
			opSLO(AbsoluteX(READWRITE));
			break;
		case 0x43:
			if(enableLogging) opTxt = "SRE ";
			opSRE(IndirectX());
			break;
		case 0x47:
			if(enableLogging) opTxt = "SRE ";
			opSRE(ZeroPage());
			break;
		case 0x4F:
			if(enableLogging) opTxt = "SRE ";
			opSRE(Absolute());
			break;
		case 0x53:
			if(enableLogging) opTxt = "SRE ";
			opSRE(IndirectY(READWRITE));
			break;
		case 0x57:
			if(enableLogging) opTxt = "SRE ";
			opSRE(ZeroPageX());
			break;
		case 0x5B:
			if(enableLogging) opTxt = "SRE ";
			opSRE(AbsoluteY(READWRITE));
			break;
		case 0x5F:
			if(enableLogging) opTxt = "SRE ";
			opSRE(AbsoluteX(READWRITE));
			break;
		case 0x85:
			if(enableLogging) opTxt = "STA ";
			opSTA(ZeroPage());
			break;
		case 0x95:
			if(enableLogging) opTxt = "STA ";
			opSTA(ZeroPageX());
			break;
		case 0x8D:
			if(enableLogging) opTxt = "STA ";
			opSTA(Absolute());
			break;
		case 0x9D:
			if(enableLogging) opTxt = "STA ";
			opSTA(AbsoluteX(WRITE));
			break;
		case 0x99:
			if(enableLogging) opTxt = "STA ";
			opSTA(AbsoluteY(WRITE));
			break;
		case 0x81:
			if(enableLogging) opTxt = "STA ";
			opSTA(IndirectX());
			break;
		case 0x91:
			if(enableLogging) opTxt = "STA ";
			opSTA(IndirectY(WRITE));
			break;
		case 0x86:
			if(enableLogging) opTxt = "STX ";
			opSTX(ZeroPage());
			break;
		case 0x96:
			if(enableLogging) opTxt = "STX ";
			opSTX(ZeroPageY());
			break;
		case 0x8E:
			if(enableLogging) opTxt = "STX ";
			opSTX(Absolute());
			break;
		case 0x84:
			if(enableLogging) opTxt = "STY ";
			opSTY(ZeroPage());
			break;
		case 0x94:
			if(enableLogging) opTxt = "STY ";
			opSTY(ZeroPageX());
			break;
		case 0x8C:
			if(enableLogging) opTxt = "STY ";
			opSTY(Absolute());
			break;
		case 0x9B:
			if(enableLogging) opTxt = "TAS ";
			opTAS(AbsoluteX(WRITE));
			break;
		case 0xAA:
			if(enableLogging) opTxt = "TAX ";
			memGet(PC); //Dummy read of PC
			opTAX();
			break;
		case 0xA8:
			if(enableLogging) opTxt = "TAY ";
			memGet(PC); //Dummy read of PC
			opTAY();
			break;
		case 0xBA:
			if(enableLogging) opTxt = "TSX ";
			memGet(PC); //Dummy read of PC
			opTSX();
			break;
		case 0x8A:
			if(enableLogging) opTxt = "TXA ";
			memGet(PC); //Dummy read of PC
			opTXA();
			break;
		case 0x9A:
			if(enableLogging) opTxt = "TXS ";
			memGet(PC); //Dummy read of PC
			opTXS();
			break;
		case 0x98:
			if(enableLogging) opTxt = "TYA ";
			memGet(PC); //Dummy read of PC
			opTYA();
			break;
		case 0x8B:
			if(enableLogging) opTxt = "XAA ";
			opXAA(Immediate());
			break;
		default:
			std::cout << std::hex << std::uppercase;
			std::cout << std::setw(4) << static_cast<int>(PC-1) << ": "
			<< std::setw(2) << static_cast<int>(opcode) << " is invalid opcode"
			<< std::endl;
			//throw 1;
			break;
	}

	if(interruptOccured)
		interruptOccured = false;
	else if(enableLogging)
		logStep();

}

void pollInterrupts() {
	if(NMI_request)
		NMI_triggered = true;
	else if(IRQ_request && P.b.I == 0)
		IRQ_triggered = true;
}


void OAMDMA_write() {
	//dummy read cycle
	tick();
	if(cycle % 2 == 1) {
		//Odd cycle
		tick();
	}
	for(int i = 0; i<256; ++i) {
		tick();
		PPU::regSet(0x2004,memGet((((uint16_t)OAMDMA)<<8)|((uint8_t)i)));
		tick();
	}
}

void triggerNMI() {
	NMI_request = true;
}

void triggerIRQ() {
	IRQ_request = true;
}

void setPC(uint16_t newPC) {
	PC = newPC;
	PC_init = PC;
}

void getStateString() {
	initVals << std::hex << std::uppercase << std::setfill('0')
		<< "  A:" << std::setw(2) << static_cast<int>(A)
		<< " X:" << std::setw(2) << static_cast<int>(X)
		<< " Y:" << std::setw(2) << static_cast<int>(Y)
		<< " P:" << std::setw(2) << static_cast<int>(P.raw)
		<< " SP:" << std::setw(2) << static_cast<int>(SP)
		<< " CYC:" << std::dec << std::setfill(' ') << std::setw(3) << PPU::dot
		<< " SL:" << std::setw(3) << PPU::scanline;
}

void logStep() {
	logFile << std::hex << std::uppercase << std::setfill('0');
	logFile << std::setw(4) << static_cast<int>(PC_init) << "  ";
	int cnt = 0;
	while(cnt < memCnt)
	{
		logFile << std::setw(2) << static_cast<int>(memVals[cnt]) << " ";
		++cnt;
	}
	while(cnt < 3)
	{
		logFile << "   ";
		++cnt;
	}
	logFile << " " << opTxt;
	for(cnt = opTxt.size(); cnt < 30; ++cnt)
		logFile << " ";
	logFile << initVals.str() << std::endl;
	initVals.str("");
}

void closeLog() {
	logFile.close();
}


//Addressing functions
//Returns final address

uint16_t Immediate() {
	uint16_t addr = PC;
	memVals[1] = memGet(addr);
	memCnt = 2;
	++PC;
	if(enableLogging) opTxt += "#$" + int_to_hex(memVals[1]);
	return addr;
}

uint16_t ZeroPage() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(addr);
	return addr;
}

uint16_t ZeroPageX() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	memGet(addr); //Dummy read
	addr += X;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(memVals[1]) + ",X @ " + int_to_hex(addr);
	return addr;
}

uint16_t ZeroPageY() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	memGet(addr); //Dummy read
	addr += Y;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(memVals[1]) + ",Y @ " + int_to_hex(addr);
	return addr;
}

uint16_t Absolute() {
	uint16_t addr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(addr);
	++PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(PC) << 8);
	memVals[2] = static_cast<uint8_t>(addr2 >> 8);
	memCnt = 3;
	addr |= addr2;
	++PC;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(addr);
	return addr;
}

uint16_t AbsoluteX(OpType optype) {
	uint16_t addr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(addr);
	++PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(PC) << 8);
	memVals[2] = static_cast<uint8_t>(addr2 >> 8);
	memCnt = 3;
	addr |= addr2;
	++PC;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(addr) + ",X @ ";

	if(optype == READ) {
		if((addr + X) != ((addr & 0xFF00) | ((addr + X) & 0xFF))) {
			memGet((addr & 0xFF00) | ((addr + X) & 0xFF)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | ((addr + X) & 0xFF)); //Dummy read
		tick();
	}
	
	addr += X;	
	if(enableLogging) opTxt += int_to_hex(addr);
	return addr;
}

uint16_t AbsoluteY(OpType optype) {
	uint16_t addr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(addr);
	++PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(PC) << 8);
	memVals[2] = static_cast<uint8_t>(addr2 >> 8);
	memCnt = 3;
	addr |= addr2;
	++PC;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(addr) + ",Y @ ";

	if(optype == READ) {
		if ((addr + Y) != ((addr & 0xFF00) | ((addr + Y) & 0xFF))) {
			memGet((addr & 0xFF00) | ((addr + Y) & 0xFF)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | ((addr + Y) & 0xFF)); //Dummy read
		tick();
	}
	
	addr += Y;	
	if(enableLogging) opTxt += int_to_hex(addr);
	return addr;
}

uint16_t Indirect() {
	uint16_t addr_loc = memGet(PC);
	memVals[1] = static_cast<uint8_t>(addr_loc);
	++PC;
	tick();
	uint16_t addr_loc2 = ((uint16_t)memGet(PC) << 8);
	memVals[2] = static_cast<uint8_t>(addr_loc2 >> 8);
	memCnt = 3;
	addr_loc |= addr_loc2;
	++PC;
	tick();
	if(enableLogging) opTxt += "$" + int_to_hex(addr_loc);
	uint16_t addr = memGet(addr_loc);
	//Implemented intentional bug. If address if xxFF, next address is xx00 (eg 02FF and 0200)
	addr |= ((uint16_t)memGet((addr_loc&0xFF00)|((uint8_t)(addr_loc+1))) << 8);
	return addr;
}

uint16_t IndirectX() {
	uint8_t iaddr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(iaddr);
	memCnt = 2;
	++PC;
	tick();
	memGet(iaddr); //Dummy read
	iaddr += X;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	if(enableLogging) opTxt += "($" + int_to_hex(memVals[1]) + ",X) @ " + int_to_hex(iaddr) + " = " + int_to_hex(addr);
	return addr;
}

uint16_t IndirectY(OpType optype) {
	uint8_t iaddr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(iaddr);
	memCnt = 2;
	++PC;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	if(optype == READ) {
		if (((addr&0xFF) + Y) > 0xFF) {
			memGet((addr & 0xFF00) | (((uint8_t)(addr))+Y)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | (((uint8_t)(addr))+Y)); //Dummy read
		tick();
	}
	
	if(enableLogging) opTxt += "($" + int_to_hex(memVals[1]) + "),Y = " + int_to_hex(addr) + " @ ";
	addr += Y;
	if(enableLogging) opTxt += int_to_hex(addr);
	return addr;
}



//CPU operation functions
void opADC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	uint8_t oldA = A;
	A += M + P.b.C;
	P.b.C = (A <= oldA);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
	pollInterrupts();
}

void opAHX(uint16_t addr) {
	uint8_t val = A & X & (addr >> 8);
	pollInterrupts();
	tick();
	memSet(addr, val);
}

void opALR(uint16_t addr) {
	uint8_t M = memGet(addr);
	pollInterrupts();
	tick();
	A &= M;
	P.b.C = M & 1;
	M = M >> 1;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
}

void opANC(uint16_t addr) {
	uint8_t M = memGet(addr);
	pollInterrupts();
	tick();
	A &= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	P.b.C = P.b.N;
}

void opAND(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	pollInterrupts();
	tick();
	A &= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opARR(uint16_t addr) {
	uint8_t M = memGet(addr);
	pollInterrupts();
	tick();
	A &= M;
	uint8_t C0 = P.b.C;
	P.b.C = (M & 1);
	M = M >> 1;
	M |= (C0 << 7);
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (A >> 7) > 0;
}

void opASL() {
	P.b.C = (A >> 7) > 0;
	A = A << 1;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
	tick();
}

void opASL(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	P.b.Z = (A == 0);
	P.b.N = (M >> 7) > 0;
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opAXS(uint16_t addr) {
	memSet(addr, A&X);
	pollInterrupts();
	tick();
}

void opBCC() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.C == 0) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBCS() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.C) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBEQ() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.Z) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBIT(uint16_t addr) {
	tick();
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	P.b.Z = (A & M) == 0;
	P.b.N = (M & 1<<7) != 0;
	P.b.V = (M & 1<<6) != 0;
}

void opBMI() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.N) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBNE() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.Z == 0) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBPL() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.N == 0) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBRK() {
	uint16_t newaddr;
	uint8_t bFlag;

	memSet(((uint16_t)0x01 << 8) | SP, PC >> 8);
	--SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | SP, PC);
	--SP;
	tick();
	//Will catch interrupts here
	if(NMI_triggered || NMI_request) {
		newaddr = 0xFFFA;
		bFlag = 0x20;
		NMI_request = NMI_triggered = false;
	}
	else if(IRQ_triggered || IRQ_request) {
		newaddr = 0xFFFE;
		bFlag = 0x20;
		IRQ_triggered = false;
	}
	else {
		newaddr = 0xFFFE;
		bFlag = 0x30;
		++PC;
	}
	memSet(((uint16_t)0x01 << 8) | SP, P.raw | bFlag);
	P.b.I = 1;
	--SP;
	tick();
	uint16_t addr = memGet(newaddr);
	tick();
	addr |= memGet(newaddr+1) << 8;
	tick();
	PC = addr;
	tick();
	pollInterrupts();
}

void opBVC()  {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.V == 0) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBVS() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.V) {
		uint16_t oldPC = PC;
		PC += delta;
		if((oldPC & 0xFF00) != (PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opCLC() {
	pollInterrupts();
	P.b.C = 0;
	tick();
}

void opCLD() {
	pollInterrupts();
	P.b.D = 0;
	tick();
}

void opCLI() {
	pollInterrupts();
	P.b.I = 0;
	tick();
}

void opCLV() {
	pollInterrupts();
	P.b.V = 0;
	tick();
}

void opCMP(uint8_t M) {
	pollInterrupts();
	P.b.C = (A >= M);
	P.b.Z = (A == M);
	P.b.N = ((uint8_t)(A-M)>>7) == 1;
	tick();
}

void opCMP(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	P.b.C = (A >= M);
	P.b.Z = (A == M);
	P.b.N = ((uint8_t)(A-M)>>7) == 1;
	//tick();
}

void opCPX(uint8_t M) {
	pollInterrupts();
	if(enableLogging) opTxt += int_to_hex(M);
	P.b.C = (X >= M);
	P.b.Z = (X == M);
	P.b.N = ((uint8_t)(X-M)>>7) == 1;
	tick();
}

void opCPX(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	P.b.C = (X >= M);
	P.b.Z = (X == M);
	P.b.N = ((uint8_t)(X-M)>>7) == 1;
}

void opCPY(uint8_t M) {
	pollInterrupts();
	if(enableLogging) opTxt += int_to_hex(M);
	P.b.C = (Y >= M);
	P.b.Z = (Y == M);
	P.b.N = ((uint8_t)(Y-M)>>7) == 1;
	tick();
}

void opCPY(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	P.b.C = (Y >= M);
	P.b.Z = (Y == M);
	P.b.N = ((uint8_t)(Y-M)>>7) == 1;
}

void opDCP(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M);
	M -= 1;
	P.b.C = (A >= M);
	P.b.Z = (A == M);
	P.b.N = ((uint8_t)(A-M)>>7) == 1;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opDEC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	memSet(addr, M);
	M -= 1;
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opDEX() {
	pollInterrupts();
	X -= 1;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opDEY() {
	pollInterrupts();
	Y -= 1;
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	tick();
}

void opEOR(void) {
	pollInterrupts();
	uint8_t M = memGet(PC);
	if(enableLogging) opTxt += int_to_hex(M);
	A ^= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opEOR(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	A ^= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opINC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	memSet(addr, M);
	M += 1;
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opINX() {
	pollInterrupts();
	X += 1;
	tick();
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
}

void opINY() {
	pollInterrupts();
	Y += 1;
	tick();
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
}

void opISC(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	M += 1;
	memSet(addr, M);
	tick();
	M = ~M;
	if(P.b.C)
		M += 1;
	uint8_t oldA = A;
	A += M;

	P.b.C = (A <= oldA);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
	pollInterrupts();
}

void opJMP(uint16_t addr) {
	PC = addr;
	pollInterrupts();
}

void opJSR(uint16_t addr) {
	memSet(((uint16_t)0x01 << 8) | SP, (PC-1) >> 8);
	--SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | SP, PC-1);
	--SP;
	tick();
	PC = addr;
	tick();	
	pollInterrupts();
}

void opLAS(uint16_t addr) {
	uint8_t val = memGet(addr);
	val &= SP;
	A = val;
	SP = val;
	X = val;
	P.b.Z = (val == 0);
	P.b.N = ((val & 0x80) > 0);
	tick();
	pollInterrupts();
}

void opLAX(uint16_t addr) {
	A = memGet(addr);
	tick();
	X = memGet(addr);
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	pollInterrupts();
}

void opLDA() {
	pollInterrupts();
	A = memGet(PC);
	++PC;
	tick();
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opLDA(uint16_t addr) {
	A = memGet(addr);
	if(enableLogging) opTxt += " = " + int_to_hex(A);
	tick();
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
}

void opLDX() {
	pollInterrupts();
	X = memGet(PC);
	++PC;
	tick();
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
}

void opLDX(uint16_t addr) {
	X = memGet(addr);
	tick();
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	pollInterrupts();
}

void opLDY() {
	pollInterrupts();
	Y = memGet(PC);
	++PC;
	tick();
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
}

void opLDY(uint16_t addr) {
	Y = memGet(addr);
	tick();
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	pollInterrupts();
}

void opLSR() {
	pollInterrupts();
	P.b.C = A & 1;
	A = A >> 1;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opLSR(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	P.b.C = M & 1;
	M = M >> 1;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
	pollInterrupts();
}

void opNOP(uint16_t addr) {
	memGet(addr); //Dummy read
	tick();
	pollInterrupts();
}

void opORA(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	A |= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
}

void opPHA() {
	pollInterrupts();
	memSet(((uint16_t)0x01 << 8) | SP, A);
	--SP;
	tick();
	tick();
}

void opPHP() {
	pollInterrupts();
	memSet(((uint16_t)0x01 << 8) | SP, P.raw);
	--SP;
	tick();
	tick();
}

void opPLA() {
	++SP;
	tick();
	A = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
	pollInterrupts();
}

void opPLP() {
	pollInterrupts();
	++SP;
	tick();
	P.raw = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	tick();
}

void opRLA(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M);
	uint8_t C0 = P.b.C;
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	M |= C0;
	tick();
	memSet(addr, M);
	tick();
	A &= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
}

void opROL() {
	pollInterrupts();
	uint8_t C0 = P.b.C;
	P.b.C = (A >> 7) > 0;
	A = A << 1;
	A |= C0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opROL(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	uint8_t C0 = P.b.C;
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	M |= C0;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (A >> 7) > 0;
	tick();
	pollInterrupts();
}

void opROR() {
	pollInterrupts();
	bool C0 = P.b.C;
	P.b.C = (A & 1);
	A = A >> 1;
	A |= C0 ? (1 << 7) : 0;
	P.b.Z = (A == 0);
	P.b.N = C0;
	tick();
}

void opROR(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	bool C0 = P.b.C;
	P.b.C = (M & 1);
	M = M >> 1;
	M |= C0 ? (1 << 7) : 0;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = C0;
	tick();
	pollInterrupts();
}

void opRRA(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M); //Dummy write
	tick();
	uint8_t C0 = P.b.C;
	P.b.C = (M & 1);
	M = M >> 1;
	M |= (C0 << 7);
	memSet(addr, M);
	tick();
	uint8_t oldA = A;
	A += M + P.b.C;
	P.b.C = (A < oldA);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
	pollInterrupts();
}

void opRTI() {
	++SP;
	P.raw = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	++SP;
	uint16_t addr = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	++SP;
	addr |= memGet(((uint16_t)0x01 << 8) | SP) << 8;
	tick();
	PC = addr;
	tick();
	tick();
	pollInterrupts();
}

void opRTS() {
	++SP;
	uint16_t addr = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	++SP;
	addr |= memGet(((uint16_t)0x01 << 8) | SP) << 8;
	tick();
	PC = addr + 1;
	tick();
	tick();
	tick();
	pollInterrupts();
}

void opSAX(uint16_t addr) {
	memSet(addr, A&X);
	tick();
	pollInterrupts();
}

void opSBC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(enableLogging) opTxt += int_to_hex(M);
	tick();
	//Subtraction in binary works by flipping bits of value to subtract by and adding one
	//Essentially this makes it the signed negative number of itself
	//This value is than added in binary. Eg 3-2 -> 3+(-2)
	//The adding one part is only done if the carry bit is set
	//The rest acts just like ADC

	M = ~M;
	if(P.b.C)
		M += 1;
	uint8_t oldA = A;
	A += M;

	P.b.C = (A < oldA) || (M == 0);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
	pollInterrupts();
}

void opSEC() {
	pollInterrupts();
	P.b.C = 1;
	tick();
}

void opSED() {
	pollInterrupts();
	P.b.D = 1;
	tick();
}

void opSEI() {
	pollInterrupts();
	P.b.I = 1;
	tick();
}

void opSHX(uint16_t addr) {
	uint8_t val = X & (addr >> 8);
	memSet(addr, val);
	tick();
	pollInterrupts();
}

void opSHY(uint16_t addr) {
	uint8_t val = Y & (addr >> 8);
	memSet(addr, val);
	tick();
	pollInterrupts();
}

void opSLO(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M);
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	tick();
	memSet(addr, M);
	tick();
	A |= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
}

void opSRE(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	memSet(addr, M);
	P.b.C = M & 1;
	M = M >> 1;
	tick();
	memSet(addr, M);
	tick();
	A ^= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	pollInterrupts();
}

void opSTA(uint16_t addr) {
	memSet(addr, A);
	tick();
	pollInterrupts();
}

void opSTX(uint16_t addr) {
	memSet(addr, X);
	tick();
	pollInterrupts();
}

void opSTY(uint16_t addr) {
	memSet(addr, Y);
	tick();
	pollInterrupts();
}

void opTAS(uint16_t addr) {
	SP = A & X;
	uint8_t val = A & X & (addr >> 8);
	memSet(addr, val);
	tick();
	pollInterrupts();
}

void opTAX() {
	pollInterrupts();
	X = A;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opTAY() {
	pollInterrupts();
	Y = A;
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	tick();
}

void opTSX() {
	pollInterrupts();
	X = SP;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opTXA() {
	pollInterrupts();
	A = X;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opTXS() {
	pollInterrupts();
	SP = X;
	tick();
}

void opTYA() {
	pollInterrupts();
	A = Y;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opXAA(uint16_t addr) {
	uint8_t val = memGet(addr);
	tick();
	A = X & val;
	P.b.N = (A & 0x80) > 0;
	P.b.Z = (A == 0);
	pollInterrupts();
}

}