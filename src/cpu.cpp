#include "cpu.h"
#include "nes.h"
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

bool NMI_request, IRQ_request, NMI_triggered, IRQ_triggered, interruptOccured;
unsigned long long cpuCycle;

struct StatusReg {
	union {
		uint8_t value;
		BitWorker<0, 1> C; //Carry
		BitWorker<1, 1> Z; //Zero
		BitWorker<2, 1> I; //Interrupt Disable
		BitWorker<3, 1> D; //Decimal (not used on NES)
		BitWorker<4, 2> B; //"Break" flags
		BitWorker<6, 1> V; //Overflow
		BitWorker<7, 1> N; //Negative
	};
};

struct CPURegisters {
	uint16_t PC;
	uint8_t SP;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	StatusReg P;
} reg;

enum AddressingMode {
	ABSOLUTE,
	ABSOLUTEX,
	ABSOLUTEY,
	ZEROPAGE,
	ZEROPAGEX,
	ZEROPAGEY,
	IMMEDIATE,
	RELATIVE,
	IMPLICIT,
	INDIRECT,
	IDX_INDIRECT,
	INDIRECT_IDX,
};

struct OpInfo {
	CPURegisters lastRegs;
	uint8_t opCode;
	std::string opName;
	AddressingMode addrMode;
	uint8_t addrL;
	uint8_t addrH;
	uint16_t actAddr;
	uint8_t val;
} opInfo;

std::array<uint8_t, 2048> RAM;
uint8_t OAMDMA;
uint8_t busVal;


uint8_t memGet(uint16_t addr) {
	//Logic for grabbing 8bit value at address
	//Handles mirroring, or which component to get data from
	//CPU memory is from 0000-1FFF, but 0800-1FFF is mirrored
	//Value is put onto bus first before returning, to allow for open bus behavior
	
	if(addr < 0x2000) {			//CPU 2k internal memory space or mirrored
		busVal = RAM[addr % 0x800];
	}
	else if(addr < 0x4000) {	//PPU registers or mirrored
		busVal = PPU::regGet(0x2000 + (addr % 8));
	}
	else if(addr < 0x4014) {	//APU registers
		busVal = APU::regGet(addr);
	}
	else if(addr == 0x4014) {	//OAMDMA register is write only
		//Do nothing. busVal remains the same
	}
	else if(addr < 0x4016) {	//APU registers
		busVal = APU::regGet(addr);
	}
	else if(addr < 0x4018) {	//IO registers
		busVal = IO::regGet(addr);
	}
	else if(addr < 0x4020) {
		busVal = IO::regGet(addr);	//APU registers
	}
	else {
		busVal = GAMEPAK::CPUmemGet(addr);	//Gamepak memory
	}
	return busVal;
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
	else if(addr == 0x4017) {
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


void powerOn() {
	reg.SP = 0xFD;
	reg.P.value = 0;
	reg.P.I = 1;
	reg.PC = memGet(0xFFFC) | memGet(0xFFFD) << 8;
	NMI_request = NMI_triggered = false;
	IRQ_request = IRQ_triggered = false;
	interruptOccured = false;
	cpuCycle = 0;
	for(int i = 0; i < 2048; ++i)
		RAM[i] = 0;
}

void reset() {
	//Per https://wiki.nesdev.com/w/index.php/CPU_power_up_state
	reg.SP -= 3;
	reg.P.I = true;
	memSet(0x4015, 0);
	PPU::reset();
	reg.PC = memGet(0xFFFC) | memGet(0xFFFD) << 8;
}

void tick() {
	++cpuCycle;
	PPU::step();
	PPU::step();
	PPU::step();
	APU::step();
}

void step() {
	uint8_t opcode;
	if(NMI_triggered | IRQ_triggered) {
		interruptOccured = true;
		opcode = 0x00; //BRK
		tick();
	}
	else {
		opcode = memGet(reg.PC);
		if(NES::logging) {
			opInfo.lastRegs.PC = reg.PC;
			opInfo.lastRegs.A = reg.A;
			opInfo.lastRegs.X = reg.X;
			opInfo.lastRegs.Y = reg.Y;
			opInfo.lastRegs.SP = reg.SP;
			opInfo.lastRegs.P.value = reg.P.value;
			opInfo.opCode = opcode;
		}
		++reg.PC;
		tick();
	}
	switch (opcode) {
		case 0x69:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(Immediate());
			break;
		case 0x65:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(ZeroPage());
			break;
		case 0x75:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(ZeroPageX());
			break;
		case 0x6D:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(Absolute());
			break;
		case 0x7D:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(AbsoluteX(READ));
			break;
		case 0x79:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(AbsoluteY(READ));
			break;
		case 0x61:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(IndirectX());
			break;
		case 0x71:
			if(NES::logging) opInfo.opName = "ADC";
			opADC(IndirectY(READ));
			break;
		case 0x93:
			if(NES::logging) opInfo.opName = "AHX";
			opAHX(IndirectY(WRITE));
			break;
		case 0x9F:
			if(NES::logging) opInfo.opName = "AHX";
			opAHX(AbsoluteY(WRITE));
			break;
		case 0x4B:
			if(NES::logging) opInfo.opName = "ALR";
			opALR(Immediate());
			break;
		case 0x0B:
			if(NES::logging) opInfo.opName = "ANC";
			opANC(Immediate());
			break;
		case 0x2B:
			if(NES::logging) opInfo.opName = "ANC";
			opANC(Immediate());
			break;	
		case 0x29:
			if(NES::logging) opInfo.opName = "AND";
			opAND(Immediate());
			break;
		case 0x25:
			if(NES::logging) opInfo.opName = "AND";
			opAND(ZeroPage());
			break;
		case 0x35:
			if(NES::logging) opInfo.opName = "AND";
			opAND(ZeroPageX());
			break;
		case 0x2D:
			if(NES::logging) opInfo.opName = "AND";
			opAND(Absolute());
			break;
		case 0x3D:
			if(NES::logging) opInfo.opName = "AND";
			opAND(AbsoluteX(READ));
			break;
		case 0x39:
			if(NES::logging) opInfo.opName = "AND";
			opAND(AbsoluteY(READ));
			break;
		case 0x21:
			if(NES::logging) opInfo.opName = "AND";
			opAND(IndirectX());
			break;
		case 0x31:
			if(NES::logging) opInfo.opName = "AND";
			opAND(IndirectY(READ));
			break;
		case 0x6B:
			if(NES::logging) opInfo.opName = "ARR";
			opARR(Immediate());
			break;
		case 0x0A:
			if(NES::logging) opInfo.opName = "ASL";
			memGet(reg.PC); //Dummy read of PC
			opASL();
			break;
		case 0x06:
			if(NES::logging) opInfo.opName = "ASL";
			opASL(ZeroPage());
			break;
		case 0x16:
			if(NES::logging) opInfo.opName = "ASL";
			opASL(ZeroPageX());
			break;
		case 0x0E:
			if(NES::logging) opInfo.opName = "ASL";
			opASL(Absolute());
			break;
		case 0x1E:
			if(NES::logging) opInfo.opName = "ASL";
			opASL(AbsoluteX(READWRITE));
			break;
		case 0xCB:
			if(NES::logging) opInfo.opName = "AXS";
			opAXS(Immediate());
			break;
		case 0x90:
			if(NES::logging) opInfo.opName = "BCC";
			memGet(reg.PC); //Dummy read of PC
			opBCC();
			break;
		case 0xB0:
			if(NES::logging) opInfo.opName = "BCS";
			memGet(reg.PC); //Dummy read of PC
			opBCS();
			break;
		case 0xF0:
			if(NES::logging) opInfo.opName = "BEQ";
			memGet(reg.PC); //Dummy read of PC
			opBEQ();
			break;
		case 0x24:
			if(NES::logging) opInfo.opName = "BIT";
			opBIT(ZeroPage());
			break;
		case 0x2C:
			if(NES::logging) opInfo.opName = "BIT";
			opBIT(Absolute());
			break;
		case 0x30:
			if(NES::logging) opInfo.opName = "BMI";
			memGet(reg.PC); //Dummy read of PC
			opBMI();
			break;
		case 0xD0:
			if(NES::logging) opInfo.opName = "BNE";
			memGet(reg.PC); //Dummy read of PC
			opBNE();
			break;
		case 0x10:
			if(NES::logging) opInfo.opName = "BPL";
			memGet(reg.PC); //Dummy read of PC
			opBPL();
			break;	
		case 0x00:
			if(NES::logging) opInfo.opName = "BRK";
			memGet(reg.PC); //Dummy read of PC
			opBRK();
			break;
		case 0x50:
			if(NES::logging) opInfo.opName = "BVC";
			memGet(reg.PC); //Dummy read of PC
			opBVC();
			break;
		case 0x70:
			if(NES::logging) opInfo.opName = "BVS";
			memGet(reg.PC); //Dummy read of PC
			opBVS();
			break;
		case 0x18:
			if(NES::logging) opInfo.opName = "CLC";
			memGet(reg.PC); //Dummy read of PC
			opCLC();
			break;
		case 0xD8:
			if(NES::logging) opInfo.opName = "CLD";
			memGet(reg.PC); //Dummy read of PC
			opCLD();
			break;
		case 0x58:
			if(NES::logging) opInfo.opName = "CLI";
			memGet(reg.PC); //Dummy read of PC
			opCLI();
			break;
		case 0xB8:
			if(NES::logging) opInfo.opName = "CLV";
			memGet(reg.PC); //Dummy read of PC
			opCLV();
			break;
		case 0xC9:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(Immediate());
			break;
		case 0xC5:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(ZeroPage());
			break;
		case 0xD5:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(ZeroPageX());
			break;
		case 0xCD:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(Absolute());
			break;
		case 0xDD:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(AbsoluteX(READ));
			break;
		case 0xD9:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(AbsoluteY(READ));
			break;
		case 0xC1:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(IndirectX());
			break;
		case 0xD1:
			if(NES::logging) opInfo.opName = "CMP";
			opCMP(IndirectY(READ));
			break;
		case 0xE0:
			if(NES::logging) opInfo.opName = "CPX";
			opCPX(Immediate());
			break;
		case 0xE4:
			if(NES::logging) opInfo.opName = "CPX";
			opCPX(ZeroPage());
			break;
		case 0xEC:
			if(NES::logging) opInfo.opName = "CPX";
			opCPX(Absolute());
			break;
		case 0xC0:
			if(NES::logging) opInfo.opName = "CPY";
			opCPY(Immediate());
			break;
		case 0xC4:
			if(NES::logging) opInfo.opName = "CPY";
			opCPY(ZeroPage());
			break;
		case 0xCC:
			if(NES::logging) opInfo.opName = "CPY";
			opCPY(Absolute());
			break;
		case 0xC3:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(IndirectX());
			break;
		case 0xC7:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(ZeroPage());
			break;
		case 0xCF:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(Absolute());
			break;
		case 0xD3:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(IndirectY(READWRITE));
			break;
		case 0xD7:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(ZeroPageX());
			break;
		case 0xDB:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(AbsoluteY(READWRITE));
			break;
		case 0xDF:
			if(NES::logging) opInfo.opName = "DCP";
			opDCP(AbsoluteX(READWRITE));
			break;	
		case 0xC6:
			if(NES::logging) opInfo.opName = "DEC";
			opDEC(ZeroPage());
			break;
		case 0xD6:
			if(NES::logging) opInfo.opName = "DEC";
			opDEC(ZeroPageX());
			break;
		case 0xCE:
			if(NES::logging) opInfo.opName = "DEC";
			opDEC(Absolute());
			break;
		case 0xDE:
			if(NES::logging) opInfo.opName = "DEC";
			opDEC(AbsoluteX(READWRITE));
			break;
		case 0xCA:
			if(NES::logging) opInfo.opName = "DEX";
			memGet(reg.PC); //Dummy read of PC
			opDEX();
			break;
		case 0x88:
			if(NES::logging) opInfo.opName = "DEY";
			memGet(reg.PC); //Dummy read of PC
			opDEY();
			break;
		case 0x49:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(Immediate());
			break;
		case 0x45:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(ZeroPage());
			break;
		case 0x55:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(ZeroPageX());
			break;
		case 0x4D:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(Absolute());
			break;
		case 0x5D:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(AbsoluteX(READ));
			break;
		case 0x59:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(AbsoluteY(READ));
			break;
		case 0x41:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(IndirectX());
			break;
		case 0x51:
			if(NES::logging) opInfo.opName = "EOR";
			opEOR(IndirectY(READ));
			break;
		case 0xE6:
			if(NES::logging) opInfo.opName = "INC";
			opINC(ZeroPage());
			break;
		case 0xF6:
			if(NES::logging) opInfo.opName = "INC";
			opINC(ZeroPageX());
			break;
		case 0xEE:
			if(NES::logging) opInfo.opName = "INC";
			opINC(Absolute());
			break;
		case 0xFE:
			if(NES::logging) opInfo.opName = "INC";
			opINC(AbsoluteX(READWRITE));
			break;
		case 0xE8:
			if(NES::logging) opInfo.opName = "INX";
			memGet(reg.PC); //Dummy read of PC
			opINX();
			break;
		case 0xC8:
			if(NES::logging) opInfo.opName = "INY";
			memGet(reg.PC); //Dummy read of PC
			opINY();
			break;
		case 0xE3:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(IndirectX());
			break;
		case 0xE7:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(ZeroPage());
			break;
		case 0xEF:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(Absolute());
			break;
		case 0xF3:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(IndirectY(READWRITE));
			break;
		case 0xF7:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(ZeroPageX());
			break;
		case 0xFB:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(AbsoluteY(READWRITE));
			break;
		case 0xFF:
			if(NES::logging) opInfo.opName = "ISC";
			opISC(AbsoluteX(READWRITE));
			break;	
		case 0x4C:
			if(NES::logging) opInfo.opName = "JMP";
			opJMP(Absolute());
			break;
		case 0x6C:
			if(NES::logging) opInfo.opName = "JMP";
			opJMP(Indirect());
			tick();
			tick();
			break;
		case 0x20:
			if(NES::logging) opInfo.opName = "JSR";
			opJSR(Absolute());
			break;
		case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
		case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
			if(NES::logging) opInfo.opName = "KIL";
			NES::running = false;
			break;
		case 0xBB:
			if(NES::logging) opInfo.opName = "LAS";
			opLAS(AbsoluteY(READ));
			break;
		case 0xA3:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(IndirectX());
			break;
		case 0xA7:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(ZeroPage());
			break;
		case 0xAB:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(Immediate());
			break;
		case 0xAF:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(Absolute());
			break;
		case 0xB3:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(IndirectY(READ));
			break;
		case 0xB7:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(ZeroPageY());
			break;
		case 0xBF:
			if(NES::logging) opInfo.opName = "LAX";
			opLAX(AbsoluteY(READ));
			break;
		case 0xA9:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(Immediate());
			break;
		case 0xA5:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(ZeroPage());
			break;
		case 0xB5:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(ZeroPageX());
			break;
		case 0xAD:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(Absolute());
			break;
		case 0xBD:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(AbsoluteX(READ));
			break;
		case 0xB9:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(AbsoluteY(READ));
			break;
		case 0xA1:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(IndirectX());
			break;
		case 0xB1:
			if(NES::logging) opInfo.opName = "LDA";
			opLDA(IndirectY(READ));
			break;
		case 0xA2:
			if(NES::logging) opInfo.opName = "LDX";
			opLDX(Immediate());
			break;
		case 0xA6:
			if(NES::logging) opInfo.opName = "LDX";
			opLDX(ZeroPage());
			break;
		case 0xB6:
			if(NES::logging) opInfo.opName = "LDX";
			opLDX(ZeroPageY());
			break;
		case 0xAE:
			if(NES::logging) opInfo.opName = "LDX";
			opLDX(Absolute());
			break;
		case 0xBE:
			if(NES::logging) opInfo.opName = "LDX";
			opLDX(AbsoluteY(READ));
			break;
		case 0xA0:
			if(NES::logging) opInfo.opName = "LDY";
			opLDY(Immediate());
			break;
		case 0xA4:
			if(NES::logging) opInfo.opName = "LDY";
			opLDY(ZeroPage());
			break;
		case 0xB4:
			if(NES::logging) opInfo.opName = "LDY";
			opLDY(ZeroPageX());
			break;
		case 0xAC:
			if(NES::logging) opInfo.opName = "LDY";
			opLDY(Absolute());
			break;
		case 0xBC:
			if(NES::logging) opInfo.opName = "LDY";
			opLDY(AbsoluteX(READ));
			break;
		case 0x4A:
			if(NES::logging) opInfo.opName = "LSR";
			memGet(reg.PC); //Dummy read of PC
			opLSR();
			break;
		case 0x46:
			if(NES::logging) opInfo.opName = "LSR";
			opLSR(ZeroPage());
			break;
		case 0x56:
			if(NES::logging) opInfo.opName = "LSR";
			opLSR(ZeroPageX());
			break;
		case 0x4E:
			if(NES::logging) opInfo.opName = "LSR";
			opLSR(Absolute());
			break;
		case 0x5E:
			if(NES::logging) opInfo.opName = "LSR";
			opLSR(AbsoluteX(READWRITE));
			break;
		case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(reg.PC);
			break;
		case 0x80: case 0x82: case 0xC2: case 0xE2: case 0x89:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(Immediate());
			break;
		case 0x04: case 0x44: case 0x64:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(ZeroPage());
			break;
		case 0x0C:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(Absolute());
			break;
		case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(AbsoluteX(READ));
			break;
		case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
			if(NES::logging) opInfo.opName = "NOP";
			opNOP(ZeroPageX());
			break;
		case 0x09:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(Immediate());
			break;
		case 0x05:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(ZeroPage());
			break;
		case 0x15:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(ZeroPageX());
			break;
		case 0x0D:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(Absolute());
			break;
		case 0x1D:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(AbsoluteX(READ));
			break;
		case 0x19:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(AbsoluteY(READ));
			break;
		case 0x01:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(IndirectX());
			break;
		case 0x11:
			if(NES::logging) opInfo.opName = "ORA";
			opORA(IndirectY(READ));
			break;
		case 0x48:
			if(NES::logging) opInfo.opName = "PHA";
			memGet(reg.PC); //Dummy read of PC
			opPHA();
			break;
		case 0x08:
			if(NES::logging) opInfo.opName = "PHP";
			memGet(reg.PC); //Dummy read of PC
			opPHP();
			break;
		case 0x68:
			if(NES::logging) opInfo.opName = "PLA";
			memGet(reg.PC); //Dummy read of PC
			opPLA();
			break;
		case 0x28:
			if(NES::logging) opInfo.opName = "PLP";
			memGet(reg.PC); //Dummy read of PC
			opPLP();
			break;
		case 0x23:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(IndirectX());
			break;
		case 0x27:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(ZeroPage());
			break;
		case 0x2F:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(Absolute());
			break;
		case 0x33:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(IndirectY(READWRITE));
			break;
		case 0x37:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(ZeroPageX());
			break;
		case 0x3B:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(AbsoluteY(READWRITE));
			break;
		case 0x3F:
			if(NES::logging) opInfo.opName = "RLA";
			opRLA(AbsoluteX(READWRITE));
			break;
		case 0x2A:
			if(NES::logging) opInfo.opName = "ROL";
			memGet(reg.PC); //Dummy read of PC
			opROL();
			break;
		case 0x26:
			if(NES::logging) opInfo.opName = "ROL";
			opROL(ZeroPage());
			break;
		case 0x36:
			if(NES::logging) opInfo.opName = "ROL";
			opROL(ZeroPageX());
			break;
		case 0x2E:
			if(NES::logging) opInfo.opName = "ROL";
			opROL(Absolute());
			break;
		case 0x3E:
			if(NES::logging) opInfo.opName = "ROL";
			opROL(AbsoluteX(READWRITE));
			break;
		case 0x6A:
			if(NES::logging) opInfo.opName = "ROR";
			memGet(reg.PC); //Dummy read of PC
			opROR();
			break;
		case 0x66:
			if(NES::logging) opInfo.opName = "ROR";
			opROR(ZeroPage());
			break;
		case 0x76:
			if(NES::logging) opInfo.opName = "ROR";
			opROR(ZeroPageX());
			break;
		case 0x6E:
			if(NES::logging) opInfo.opName = "ROR";
			opROR(Absolute());
			break;
		case 0x7E:
			if(NES::logging) opInfo.opName = "ROR";
			opROR(AbsoluteX(READWRITE));
			break;
		case 0x63:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(IndirectX());
			break;
		case 0x67:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(ZeroPage());
			break;
		case 0x6F:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(Absolute());
			break;
		case 0x73:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(IndirectY(READWRITE));
			break;
		case 0x77:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(ZeroPageX());
			break;
		case 0x7B:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(AbsoluteY(READWRITE));
			break;
		case 0x7F:
			if(NES::logging) opInfo.opName = "RRA";
			opRRA(AbsoluteX(READWRITE));
			break;	
		case 0x40:
			if(NES::logging) opInfo.opName = "RTI";
			memGet(reg.PC); //Dummy read of PC
			opRTI();
			break;
		case 0x60:
			if(NES::logging) opInfo.opName = "RTS";
			memGet(reg.PC); //Dummy read of PC
			opRTS();
			break;
		case 0x83:
			if(NES::logging) opInfo.opName = "SAX";
			opSAX(IndirectX());
			break;
		case 0x87:
			if(NES::logging) opInfo.opName = "SAX";
			opSAX(ZeroPage());
			break;
		case 0x8F:
			if(NES::logging) opInfo.opName = "SAX";
			opSAX(Absolute());
			break;
		case 0x97:
			if(NES::logging) opInfo.opName = "SAX";
			opSAX(ZeroPageY());
			break;
		case 0xE9:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(Immediate());
			break;
		case 0xE5:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(ZeroPage());
			break;
		case 0xEB:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(Immediate());
			break;
		case 0xF5:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(ZeroPageX());
			break;
		case 0xED:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(Absolute());
			break;
		case 0xFD:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(AbsoluteX(READ));
			break;
		case 0xF9:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(AbsoluteY(READ));
			break;
		case 0xE1:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(IndirectX());
			break;
		case 0xF1:
			if(NES::logging) opInfo.opName = "SBC";
			opSBC(IndirectY(READ));
			break;
		case 0x38:
			if(NES::logging) opInfo.opName = "SEC";
			memGet(reg.PC); //Dummy read of PC
			opSEC();
			break;
		case 0xF8:
			if(NES::logging) opInfo.opName = "SED";
			memGet(reg.PC); //Dummy read of PC
			opSED();
			break;
		case 0x78:
			if(NES::logging) opInfo.opName = "SEI";
			memGet(reg.PC); //Dummy read of PC
			opSEI();
			break;
		case 0x9E:
			if(NES::logging) opInfo.opName = "SHX";
			opSHX(AbsoluteY(WRITE));
			break;
		case 0x9C:
			if(NES::logging) opInfo.opName = "SHY";
			opSHY(AbsoluteX(WRITE));
			break;
		case 0x03:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(IndirectX());
			break;
		case 0x07:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(ZeroPage());
			break;
		case 0x0F:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(Absolute());
			break;
		case 0x13:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(IndirectY(READWRITE));
			break;
		case 0x17:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(ZeroPageX());
			break;
		case 0x1B:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(AbsoluteY(READWRITE));
			break;
		case 0x1F:
			if(NES::logging) opInfo.opName = "SLO";
			opSLO(AbsoluteX(READWRITE));
			break;
		case 0x43:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(IndirectX());
			break;
		case 0x47:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(ZeroPage());
			break;
		case 0x4F:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(Absolute());
			break;
		case 0x53:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(IndirectY(READWRITE));
			break;
		case 0x57:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(ZeroPageX());
			break;
		case 0x5B:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(AbsoluteY(READWRITE));
			break;
		case 0x5F:
			if(NES::logging) opInfo.opName = "SRE";
			opSRE(AbsoluteX(READWRITE));
			break;
		case 0x85:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(ZeroPage());
			break;
		case 0x95:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(ZeroPageX());
			break;
		case 0x8D:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(Absolute());
			break;
		case 0x9D:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(AbsoluteX(WRITE));
			break;
		case 0x99:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(AbsoluteY(WRITE));
			break;
		case 0x81:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(IndirectX());
			break;
		case 0x91:
			if(NES::logging) opInfo.opName = "STA";
			opSTA(IndirectY(WRITE));
			break;
		case 0x86:
			if(NES::logging) opInfo.opName = "STX";
			opSTX(ZeroPage());
			break;
		case 0x96:
			if(NES::logging) opInfo.opName = "STX";
			opSTX(ZeroPageY());
			break;
		case 0x8E:
			if(NES::logging) opInfo.opName = "STX";
			opSTX(Absolute());
			break;
		case 0x84:
			if(NES::logging) opInfo.opName = "STY";
			opSTY(ZeroPage());
			break;
		case 0x94:
			if(NES::logging) opInfo.opName = "STY";
			opSTY(ZeroPageX());
			break;
		case 0x8C:
			if(NES::logging) opInfo.opName = "STY";
			opSTY(Absolute());
			break;
		case 0x9B:
			if(NES::logging) opInfo.opName = "TAS";
			opTAS(AbsoluteY(WRITE));
			break;
		case 0xAA:
			if(NES::logging) opInfo.opName = "TAX";
			memGet(reg.PC); //Dummy read of PC
			opTAX();
			break;
		case 0xA8:
			if(NES::logging) opInfo.opName = "TAY";
			memGet(reg.PC); //Dummy read of PC
			opTAY();
			break;
		case 0xBA:
			if(NES::logging) opInfo.opName = "TSX";
			memGet(reg.PC); //Dummy read of PC
			opTSX();
			break;
		case 0x8A:
			if(NES::logging) opInfo.opName = "TXA";
			memGet(reg.PC); //Dummy read of PC
			opTXA();
			break;
		case 0x9A:
			if(NES::logging) opInfo.opName = "TXS";
			memGet(reg.PC); //Dummy read of PC
			opTXS();
			break;
		case 0x98:
			if(NES::logging) opInfo.opName = "TYA";
			memGet(reg.PC); //Dummy read of PC
			opTYA();
			break;
		case 0x8B:
			if(NES::logging) opInfo.opName = "XAA";
			opXAA(Immediate());
			break;
	}

	if(interruptOccured)
		interruptOccured = false;
	else if(NES::logging)
		logStep();

}

void pollInterrupts() {
	if(NMI_request)
		NMI_triggered = true;
	else if(IRQ_request && reg.P.I == 0)
		IRQ_triggered = true;
}


void OAMDMA_write() {
	//dummy read cpuCycle
	tick();
	if(cpuCycle % 2 == 1) {
		//Odd cpuCycle
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

void unsetIRQ() {
	IRQ_request = false;
}

void setPC(uint16_t newPC) {
	reg.PC = newPC;
}

void logStep()
{
	NES::logFile << std::hex << std::uppercase << std::setfill('0')
				 << std::setw(4) << static_cast<int>(opInfo.lastRegs.PC) << "  "
				 << std::setw(2) << static_cast<int>(opInfo.opCode) << " ";

	switch (opInfo.addrMode) {
		case AddressingMode::ABSOLUTE:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << " "
				<< std::setw(2) << static_cast<int>(opInfo.addrH) << "  " << opInfo.opName
				<< " $" << std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t\t\t";
			break;
		case AddressingMode::ABSOLUTEX:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << " "
				<< std::setw(2) << static_cast<int>(opInfo.addrH) << "  " << opInfo.opName
				<< " $" << std::setw(2) << static_cast<int>(opInfo.addrH)
				<< std::setw(2) << static_cast<int>(opInfo.addrL) << ",X @ $"
				<< std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::ABSOLUTEY:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << " "
				<< std::setw(2) << static_cast<int>(opInfo.addrH) << "  " << opInfo.opName
				<< " $" << std::setw(2) << static_cast<int>(opInfo.addrH)
				<< std::setw(2) << static_cast<int>(opInfo.addrL) << ",Y @ $"
				<< std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::ZEROPAGE:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " $" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< "\t\t\t\t\t\t";
			break;
		case AddressingMode::ZEROPAGEX:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " $" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< ",X @ $" << std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::ZEROPAGEY:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " $" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< ",Y @ $" << std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::IMMEDIATE:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " #$" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< "\t\t\t\t\t";
			break;
		case AddressingMode::RELATIVE:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " $" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< "\t\t\t\t\t\t";
			break;
		case AddressingMode::IMPLICIT:
			NES::logFile << "       " << opInfo.opName << "\t\t\t\t\t\t\t";
			break;
		case AddressingMode::INDIRECT:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << " "
				<< std::setw(2) << static_cast<int>(opInfo.addrH) << "  " << opInfo.opName
				<< " $" << std::setw(2) << static_cast<int>(opInfo.addrH)
				<< std::setw(2) << static_cast<int>(opInfo.addrL) << " @ $"
				<< std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::IDX_INDIRECT:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " ($" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< ",X) @ $"	<< std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
		case AddressingMode::INDIRECT_IDX:
			NES::logFile << std::setw(2) << static_cast<int>(opInfo.addrL) << "     "
				<< opInfo.opName << " ($" << std::setw(2) << static_cast<int>(opInfo.addrL)
				<< "),Y @ $"	<< std::setw(4) << static_cast<int>(opInfo.actAddr)
				<< "\t\t\t";
			break;
	}

	NES::logFile << std::hex << std::uppercase << std::setfill('0')
		<< "A:" << std::setw(2) << static_cast<int>(opInfo.lastRegs.A)
		<< " X:" << std::setw(2) << static_cast<int>(opInfo.lastRegs.X)
		<< " Y:" << std::setw(2) << static_cast<int>(opInfo.lastRegs.Y)
		<< " P:" << std::setw(2) << static_cast<int>(opInfo.lastRegs.P.value)
		<< " SP:" << std::setw(2) << static_cast<int>(opInfo.lastRegs.SP)
		<< " CYC:" << std::dec << std::setfill(' ') << std::setw(3) << PPU::dot
		<< " SL:" << std::setw(3) << PPU::scanline << std::endl;
}

void logInterrupt(std::string txt)
{
	NES::logFile << txt << std::endl;
}


//Addressing functions
//Returns final address

uint16_t Immediate() {
	uint16_t addr = reg.PC;
	++reg.PC;
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::IMMEDIATE;
	}
	return addr;
}

uint16_t ZeroPage() {
	uint8_t addr = memGet(reg.PC);
	++reg.PC;
	tick();
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ZEROPAGE;
		opInfo.actAddr = opInfo.addrL = addr;
	}
	return addr;
}

uint16_t ZeroPageX() {
	uint8_t addr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ZEROPAGEX;
		opInfo.addrL = addr;
	}
	++reg.PC;
	tick();
	memGet(addr); //Dummy read
	addr += reg.X;
	tick();
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t ZeroPageY() {
	uint8_t addr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ZEROPAGEY;
		opInfo.addrL = addr;
	}
	++reg.PC;
	tick();
	memGet(addr); //Dummy read
	addr += reg.Y;
	tick();
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t Absolute() {
	uint16_t addr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ABSOLUTE;
		opInfo.addrL = addr;
	}
	++reg.PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(reg.PC) << 8);
	if(NES::logging) opInfo.addrH = addr2 >> 8;
	addr |= addr2;
	++reg.PC;
	tick();
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t AbsoluteX(OpType optype) {
	uint16_t addr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ABSOLUTEX;
		opInfo.addrL = addr;
	}
	++reg.PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(reg.PC) << 8);
	if(NES::logging) opInfo.addrH = addr2 >> 8;
	addr |= addr2;
	++reg.PC;
	tick();

	if(optype == READ) {
		if((addr + reg.X) != ((addr & 0xFF00) | ((addr + reg.X) & 0xFF))) {
			memGet((addr & 0xFF00) | ((addr + reg.X) & 0xFF)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | ((addr + reg.X) & 0xFF)); //Dummy read
		tick();
	}
	
	addr += reg.X;	
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t AbsoluteY(OpType optype) {
	uint16_t addr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::ABSOLUTEY;
		opInfo.addrL = addr;
	}
	++reg.PC;
	tick();
	uint16_t addr2 = ((uint16_t)memGet(reg.PC) << 8);
	if(NES::logging) opInfo.addrH = addr2 >> 8;
	addr |= addr2;
	++reg.PC;
	tick();

	if(optype == READ) {
		if ((addr + reg.Y) != ((addr & 0xFF00) | ((addr + reg.Y) & 0xFF))) {
			memGet((addr & 0xFF00) | ((addr + reg.Y) & 0xFF)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | ((addr + reg.Y) & 0xFF)); //Dummy read
		tick();
	}
	
	addr += reg.Y;	
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t Indirect() {
	uint16_t addr_loc = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::INDIRECT;
		opInfo.addrL = addr_loc;
	}
	++reg.PC;
	tick();
	uint16_t addr_loc2 = ((uint16_t)memGet(reg.PC) << 8);
	if(NES::logging) opInfo.addrH = addr_loc2;
	addr_loc |= addr_loc2;
	++reg.PC;
	tick();
	uint16_t addr = memGet(addr_loc);
	//Implemented 6502 bug. If address if xxFF, next address is xx00 (eg 02FF and 0200)
	addr |= ((uint16_t)memGet((addr_loc&0xFF00)|((uint8_t)(addr_loc+1))) << 8);
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t IndirectX() {
	uint8_t iaddr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::IDX_INDIRECT;
		opInfo.addrL = iaddr;
	}
	++reg.PC;
	tick();
	memGet(iaddr); //Dummy read
	iaddr += reg.X;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}

uint16_t IndirectY(OpType optype) {
	uint8_t iaddr = memGet(reg.PC);
	if(NES::logging) {
		opInfo.addrMode = AddressingMode::INDIRECT_IDX;
		opInfo.addrL = iaddr;
	}
	++reg.PC;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	if(optype == READ) {
		if ((addr + reg.Y) != ((addr & 0xFF00) | ((addr + reg.Y) & 0xFF))) {
			memGet((addr & 0xFF00) | ((addr + reg.Y) & 0xFF)); //Dummy read
			tick();
		}
	}
	else if(optype == WRITE || optype == READWRITE) {
		memGet((addr & 0xFF00) | ((addr + reg.Y) & 0xFF)); //Dummy read
		tick();
	}
	
	addr += reg.Y;
	if(NES::logging) opInfo.actAddr = addr;
	return addr;
}



//CPU operation functions
void opADC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	uint16_t sum = reg.A + M + reg.P.C;
	reg.P.C = (sum > 0xFF) ? 1 : 0;
	reg.P.V = (~(reg.A^M) & (reg.A^((uint8_t)sum)) & 0x80) ? 1 : 0;
	reg.A = (uint8_t)sum;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	pollInterrupts();
}

void opAHX(uint16_t addr) {
	uint8_t val = reg.A & reg.X & (addr >> 8);
	if(NES::logging) opInfo.val = val;
	pollInterrupts();
	tick();
	memSet(addr, val);
}

void opALR(uint16_t addr) {
	// AND M followed by LSR A
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	pollInterrupts();
	tick();
	reg.A &= M;
	reg.P.C = reg.A & 1; //Set to old A per LSR behavior
	reg.A = reg.A >> 1;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
}

void opANC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	pollInterrupts();
	tick();
	reg.A &= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	reg.P.C = reg.P.N;
}

void opAND(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	pollInterrupts();
	tick();
	reg.A &= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
}

void opARR(uint16_t addr) {
	//AND M followed by ROR A
	//Uses strange logic for flags
	//See http://www.6502.org/users/andre/petindex/local/64doc.txt

	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	pollInterrupts();
	tick();
	reg.A &= M;

	reg.A = (reg.A >> 1) | (reg.P.C << 7);
	reg.P.N = reg.P.C;
	reg.P.Z = (reg.A == 0);
	reg.P.C = (reg.A & 0x40) ? 1 : 0;
	reg.P.V = (reg.P.C ^ ((reg.A >> 5) & 1)) ? 1 : 0;
}

void opASL() {
	reg.P.C = ((reg.A >> 7) > 0) ? 1 : 0;
	reg.A = reg.A << 1;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	pollInterrupts();
	tick();
}

void opASL(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	reg.P.C = ((M >> 7) > 0) ? 1 : 0;
	M = M << 1;
	reg.P.Z = (M == 0) ? 1 : 0;
	reg.P.N = ((M >> 7) > 0) ? 1 : 0;
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opAXS(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	reg.P.C = ((reg.A & reg.X) >= M) ? 1 : 0;
	reg.X = (reg.A & reg.X) - M;
	reg.P.N = ((reg.X >> 7) > 0) ? 1 : 0;
	reg.P.Z = (reg.X == 0) ? 1 : 0;
	pollInterrupts();
}

void opBCC() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.C == 0) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBCS() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.C) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBEQ() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.Z) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBIT(uint16_t addr) {
	tick();
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	reg.P.Z = (reg.A & M) == 0;
	reg.P.N = (M & 1<<7) != 0;
	reg.P.V = (M & 1<<6) != 0;
}

void opBMI() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.N) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBNE() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.Z == 0) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBPL() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.N == 0) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBRK() {
	memGet(reg.PC); //Dummy read
	if(interruptOccured == false) //PC increment suppressed during interrupt commanded BRK
		++reg.PC;
	tick();

	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.PC >> 8);
	--reg.SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.PC);
	--reg.SP;
	tick();

	//Copy P since the B flag is only passed onto stack
	int8_t oldP = reg.P.value;

	uint16_t newaddr;
	//Will catch interrupts here
	if(NMI_triggered || NMI_request) {
		newaddr = 0xFFFA;
		reg.P.B = 2;
		NMI_request = NMI_triggered = false;
		if(NES::logging) logInterrupt("[NMI Interrupt]");
	}
	else if(IRQ_triggered || IRQ_request) {
		newaddr = 0xFFFE;
		reg.P.B = 2;
		IRQ_triggered = false;
		if(NES::logging) logInterrupt("[IRQ Interrupt]");
	}
	else {
		newaddr = 0xFFFE;
		reg.P.B = 3;
		++reg.PC;
	}
	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.P.value);
	reg.P.value = oldP;
	reg.P.I = 1;
	--reg.SP;
	tick();
	uint16_t addr = memGet(newaddr);
	tick();
	addr |= memGet(newaddr+1) << 8;
	reg.PC = addr;
	tick();
	pollInterrupts();
}

void opBVC()  {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.V == 0) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opBVS() {
	int8_t delta = static_cast<int8_t>(memGet(reg.PC));
	if(NES::logging) opInfo.val = delta;
	++reg.PC;
	tick();
	if(reg.P.V) {
		uint16_t oldPC = reg.PC;
		reg.PC += delta;
		if((oldPC & 0xFF00) != (reg.PC & 0xFF00))
			tick(); //Moved to different page
		tick();
	}
	pollInterrupts();
}

void opCLC() {
	pollInterrupts();
	reg.P.C = 0;
	tick();
}

void opCLD() {
	pollInterrupts();
	reg.P.D = 0;
	tick();
}

void opCLI() {
	pollInterrupts();
	reg.P.I = 0;
	tick();
}

void opCLV() {
	pollInterrupts();
	reg.P.V = 0;
	tick();
}

void opCMP(uint8_t M) {
	pollInterrupts();
	reg.P.C = (reg.A >= M);
	reg.P.Z = (reg.A == M);
	reg.P.N = ((uint8_t)(reg.A-M)>>7) == 1;
	tick();
}

void opCMP(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	reg.P.C = (reg.A >= M);
	reg.P.Z = (reg.A == M);
	reg.P.N = ((uint8_t)(reg.A-M)>>7) == 1;
	//tick();
}

void opCPX(uint8_t M) {
	pollInterrupts();
	if(NES::logging) opInfo.val = M;
	reg.P.C = (reg.X >= M);
	reg.P.Z = (reg.X == M);
	reg.P.N = ((uint8_t)(reg.X-M)>>7) == 1;
	tick();
}

void opCPX(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	reg.P.C = (reg.X >= M);
	reg.P.Z = (reg.X == M);
	reg.P.N = ((uint8_t)(reg.X-M)>>7) == 1;
}

void opCPY(uint8_t M) {
	pollInterrupts();
	if(NES::logging) opInfo.val = M;
	reg.P.C = (reg.Y >= M);
	reg.P.Z = (reg.Y == M);
	reg.P.N = ((uint8_t)(reg.Y-M)>>7) == 1;
	tick();
}

void opCPY(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	reg.P.C = (reg.Y >= M);
	reg.P.Z = (reg.Y == M);
	reg.P.N = ((uint8_t)(reg.Y-M)>>7) == 1;
}

void opDCP(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	M -= 1;
	reg.P.C = (reg.A >= M);
	reg.P.Z = (reg.A == M);
	reg.P.N = ((uint8_t)(reg.A-M)>>7) == 1;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opDEC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	M -= 1;
	reg.P.Z = (M == 0);
	reg.P.N = (M >> 7) > 0;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opDEX() {
	pollInterrupts();
	reg.X -= 1;
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	tick();
}

void opDEY() {
	pollInterrupts();
	reg.Y -= 1;
	reg.P.Z = (reg.Y == 0);
	reg.P.N = (reg.Y >> 7) > 0;
	tick();
}

void opEOR(void) {
	pollInterrupts();
	uint8_t M = memGet(reg.PC);
	if(NES::logging) opInfo.val = M;
	reg.A ^= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
}

void opEOR(uint16_t addr) {
	pollInterrupts();
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	reg.A ^= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
}

void opINC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	M += 1;
	reg.P.Z = (M == 0);
	reg.P.N = (M >> 7) > 0;
	tick();
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opINX() {
	pollInterrupts();
	reg.X += 1;
	tick();
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
}

void opINY() {
	pollInterrupts();
	reg.Y += 1;
	tick();
	reg.P.Z = (reg.Y == 0);
	reg.P.N = (reg.Y >> 7) > 0;
}

void opISC(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	M += 1;
	memSet(addr, M);
	tick();
	M = ~M;
	uint16_t sum = reg.A + M + reg.P.C;
	reg.P.C = (sum > 0xFF) ? 1 : 0;
	reg.P.V = (~(reg.A^M) & (reg.A^((uint8_t)sum)) & 0x80) ? 1 : 0;
	reg.A = (uint8_t)sum;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	pollInterrupts();
}

void opJMP(uint16_t addr) {
	reg.PC = addr;
	pollInterrupts();
}

void opJSR(uint16_t addr) {
	memSet(((uint16_t)0x01 << 8) | reg.SP, (reg.PC-1) >> 8);
	--reg.SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.PC-1);
	--reg.SP;
	tick();
	reg.PC = addr;
	tick();	
	pollInterrupts();
}

void opLAS(uint16_t addr) {
	uint8_t val = memGet(addr);
	if(NES::logging) opInfo.val = val;
	val &= reg.SP;
	reg.A = val;
	reg.SP = val;
	reg.X = val;
	reg.P.Z = (val == 0);
	reg.P.N = ((val & 0x80) > 0);
	tick();
	pollInterrupts();
}

void opLAX(uint16_t addr) {
	reg.A = memGet(addr);
	if(NES::logging) opInfo.val = reg.A;
	tick();
	reg.X = memGet(addr);
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	pollInterrupts();
}

void opLDA() {
	pollInterrupts();
	reg.A = memGet(reg.PC);
	if(NES::logging) opInfo.val = reg.A;
	++reg.PC;
	tick();
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
}

void opLDA(uint16_t addr) {
	reg.A = memGet(addr);
	if(NES::logging) opInfo.val = reg.A;
	tick();
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	pollInterrupts();
}

void opLDX() {
	reg.X = memGet(reg.PC);
	if(NES::logging) opInfo.val = reg.X;
	++reg.PC;
	tick();
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	pollInterrupts();
}

void opLDX(uint16_t addr) {
	reg.X = memGet(addr);
	if(NES::logging) opInfo.val = reg.X;
	tick();
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	pollInterrupts();
}

void opLDY() {
	pollInterrupts();
	reg.Y = memGet(reg.PC);
	if(NES::logging) opInfo.val = reg.Y;
	++reg.PC;
	tick();
	reg.P.Z = (reg.Y == 0);
	reg.P.N = (reg.Y >> 7) > 0;
}

void opLDY(uint16_t addr) {
	reg.Y = memGet(addr);
	if(NES::logging) opInfo.val = reg.Y;
	tick();
	reg.P.Z = (reg.Y == 0);
	reg.P.N = (reg.Y >> 7) > 0;
	pollInterrupts();
}

void opLSR() {
	pollInterrupts();
	reg.P.C = reg.A & 1;
	reg.A = reg.A >> 1;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
}

void opLSR(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	reg.P.C = M & 1;
	M = M >> 1;
	memSet(addr, M);
	reg.P.Z = (M == 0);
	reg.P.N = (M >> 7) > 0;
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
	if(NES::logging) opInfo.val = M;
	tick();
	reg.A |= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	pollInterrupts();
}

void opPHA() {
	pollInterrupts();
	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.A);
	--reg.SP;
	tick();
	tick();
}

void opPHP() {
	pollInterrupts();
	int8_t Pold = reg.P.value;
	reg.P.B = 3;
	memSet(((uint16_t)0x01 << 8) | reg.SP, reg.P.value);
	reg.P.value = Pold;
	--reg.SP;
	tick();
	tick();
}

void opPLA() {
	++reg.SP;
	tick();
	reg.A = memGet(((uint16_t)0x01 << 8) | reg.SP);
	tick();
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
	pollInterrupts();
}

void opPLP() {
	pollInterrupts();
	++reg.SP;
	tick();
	reg.P.value = memGet(((uint16_t)0x01 << 8) | reg.SP);
	reg.P.B = 0;
	tick();
	tick();
}

void opRLA(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	uint8_t C0 = reg.P.C;
	reg.P.C = (M >> 7) > 0;
	M = M << 1;
	M |= C0;
	tick();
	memSet(addr, M);
	tick();
	reg.A &= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	pollInterrupts();
}

void opROL() {
	pollInterrupts();
	uint8_t C0 = reg.P.C;
	reg.P.C = ((reg.A >> 7) > 0) ? 1 : 0;
	reg.A = (reg.A << 1) | C0;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	tick();
}

void opROL(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	uint8_t C0 = reg.P.C;
	reg.P.C = ((M >> 7) > 0) ? 1 : 0;
	M = (M << 1) | C0;
	reg.P.Z = (M == 0) ? 1 : 0;
	reg.P.N = ((M >> 7) > 0) ? 1 : 0;
	memSet(addr, M);
	tick();
	pollInterrupts();
}

void opROR() {
	pollInterrupts();
	bool C0 = reg.P.C;
	reg.P.C = (reg.A & 1);
	reg.A = reg.A >> 1;
	reg.A |= C0 ? (1 << 7) : 0;
	reg.P.Z = (reg.A == 0);
	reg.P.N = C0;
	tick();
}

void opROR(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	uint8_t C0 = reg.P.C;
	reg.P.C = (M & 1);
	M = M >> 1;
	M |= C0 ? (1 << 7) : 0;
	memSet(addr, M);
	reg.P.Z = (M == 0);
	reg.P.N = C0;
	tick();
	pollInterrupts();
}

void opRRA(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M); //Dummy write
	tick();
	uint8_t C0 = reg.P.C;
	reg.P.C = (M & 1);
	M = M >> 1;
	M |= C0 ? (1 << 7) : 0;
	memSet(addr, M);
	tick();
	uint16_t sum = reg.A + M + reg.P.C;
	reg.P.C = (sum > 0xFF) ? 1 : 0;
	reg.P.V = (~(reg.A^M) & (reg.A^((uint8_t)sum)) & 0x80) ? 1 : 0;
	reg.A = (uint8_t)sum;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	pollInterrupts();
}

void opRTI() {
	++reg.SP;
	reg.P.value = memGet(((uint16_t)0x01 << 8) | reg.SP);
	reg.P.B = 0;
	tick();
	++reg.SP;
	uint16_t addr = memGet(((uint16_t)0x01 << 8) | reg.SP);
	tick();
	++reg.SP;
	addr |= memGet(((uint16_t)0x01 << 8) | reg.SP) << 8;
	tick();
	reg.PC = addr;
	tick();
	tick();
	pollInterrupts();
}

void opRTS() {
	++reg.SP;
	uint16_t addr = memGet(((uint16_t)0x01 << 8) | reg.SP);
	tick();
	++reg.SP;
	addr |= memGet(((uint16_t)0x01 << 8) | reg.SP) << 8;
	tick();
	reg.PC = addr + 1;
	tick();
	tick();
	tick();
	pollInterrupts();
}

void opSAX(uint16_t addr) {
	memSet(addr, reg.A&reg.X);
	tick();
	pollInterrupts();
}

void opSBC(uint16_t addr) {
	//SBC works the same as ADC, with the value from memory bit flipped
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	M = ~M;
	tick();
	uint16_t sum = reg.A + M + reg.P.C;
	reg.P.C = (sum > 0xFF) ? 1 : 0;
	reg.P.V = (~(reg.A^M) & (reg.A^((uint8_t)sum)) & 0x80) ? 1 : 0;
	reg.A = (uint8_t)sum;
	reg.P.Z = (reg.A == 0) ? 1 : 0;
	reg.P.N = ((reg.A >> 7) > 0) ? 1 : 0;
	pollInterrupts();
}

void opSEC() {
	pollInterrupts();
	reg.P.C = 1;
	tick();
}

void opSED() {
	pollInterrupts();
	reg.P.D = 1;
	tick();
}

void opSEI() {
	pollInterrupts();
	reg.P.I = 1;
	tick();
}

void opSHX(uint16_t addr) {
	uint8_t H = addr >> 8;
	uint8_t L = addr & 0xFF;
	uint8_t val = reg.X & (H + 1);
	memSet(((uint16_t)val << 8) | L, val);
	tick();
	pollInterrupts();
}

void opSHY(uint16_t addr) {
	uint8_t H = addr >> 8;
	uint8_t L = addr & 0xFF;
	uint8_t val = reg.Y & (H + 1);
	memSet(((uint16_t)val << 8) | L, val);
	tick();
	pollInterrupts();
}

void opSLO(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	reg.P.C = (M >> 7) > 0;
	M = M << 1;
	tick();
	memSet(addr, M);
	tick();
	reg.A |= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	pollInterrupts();
}

void opSRE(uint16_t addr) {
	uint8_t M = memGet(addr);
	if(NES::logging) opInfo.val = M;
	tick();
	memSet(addr, M);
	reg.P.C = M & 1;
	M = M >> 1;
	tick();
	memSet(addr, M);
	tick();
	reg.A ^= M;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	pollInterrupts();
}

void opSTA(uint16_t addr) {
	memSet(addr, reg.A);
	tick();
	pollInterrupts();
}

void opSTX(uint16_t addr) {
	memSet(addr, reg.X);
	tick();
	pollInterrupts();
}

void opSTY(uint16_t addr) {
	memSet(addr, reg.Y);
	tick();
	pollInterrupts();
}

void opTAS(uint16_t addr) {
	reg.SP = reg.A & reg.X;
	uint8_t val = reg.A & reg.X & (addr >> 8);
	memSet(addr, val);
	tick();
	pollInterrupts();
}

void opTAX() {
	pollInterrupts();
	reg.X = reg.A;
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	tick();
}

void opTAY() {
	pollInterrupts();
	reg.Y = reg.A;
	reg.P.Z = (reg.Y == 0);
	reg.P.N = (reg.Y >> 7) > 0;
	tick();
}

void opTSX() {
	pollInterrupts();
	reg.X = reg.SP;
	reg.P.Z = (reg.X == 0);
	reg.P.N = (reg.X >> 7) > 0;
	tick();
}

void opTXA() {
	pollInterrupts();
	reg.A = reg.X;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
}

void opTXS() {
	pollInterrupts();
	reg.SP = reg.X;
	tick();
}

void opTYA() {
	pollInterrupts();
	reg.A = reg.Y;
	reg.P.Z = (reg.A == 0);
	reg.P.N = (reg.A >> 7) > 0;
	tick();
}

void opXAA(uint16_t addr) {
	uint8_t val = memGet(addr);
	if(NES::logging) opInfo.val = val;
	tick();
	reg.A = reg.X & val;
	reg.P.N = (reg.A & 0x80) > 0;
	reg.P.Z = (reg.A == 0);
	pollInterrupts();
}

}