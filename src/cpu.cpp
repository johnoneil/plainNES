#include "cpu.h"
//#include "ppu.h"
//#include "io.h"
#include "gamepak.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

namespace CPU {
	

uint16_t PC;
uint8_t SP, A, X, Y;

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
uint8_t RAM[2048];
uint8_t OAMDMA;

//Logging variables
uint16_t PC_init;
std::string opTxt;
std::stringstream initVals;
int memCnt;
uint8_t memVals[3];
std::ofstream logFile("log.txt",std::ios::trunc);


uint8_t memGet(uint16_t addr) {
	//Logic for grabbing 8bit value at address
	//Handles mirroring, or which component to get data from
	//CPU memory is from 0000-1FFF, but 0800-1FFF is mirrored
	
	if(addr < 0x2000) {			//CPU memory space or mirrored
		return RAM[addr % 0x800];
	}
	else if(addr < 0x4000) {	//PPU memory or mirrored
		return 0;
		//return PPU::memGet(addr);
	}
	else if(addr < 0x4014) {	//IO (APU) memory
		return 0;
		//return IO::memGet(addr);
	}
	else if(addr == 0x4014) {	//OAMDMA register
		return OAMDMA;
	}
	else if(addr < 0x4020) {
		return 0;
		//return IO::memGet(addr);	//More IO (APU) memory
	}
	else {
		return GAMEPAK::memGet(addr);	//Gamepak memory
	}
}

void memSet(uint16_t addr, uint8_t val) {
	//Logic for setting 8bit value at address
	//Handles mirroring, or which component to send data to
	
	if(addr < 0x2000) {			//CPU memory space or mirrored
		RAM[addr % 0x800] = val;
	}
	else if(addr < 0x4000) {	//PPU memory or mirrored
		//PPU::memSet(addr, val);
	}
	else if(addr < 0x4014) {	//IO (APU) memory
		//IO::memSet(addr, val);
	}
	else if(addr == 0x4014) {	//OAMDMA register
		OAMDMA = val;
		OAMDMA_write();
	}
	else if(addr < 0x4020) {
		//IO::memSet(addr, val);	//More IO (APU) memory
	}
	else {
		GAMEPAK::memSet(addr, val);	//Gamepak memory
	}
}


void init() {
	SP = 0xFD;
	P.raw = 0x24;
	PC = memGet(0xFFFC) | memGet(0xFFFD) << 8;
	PC_init = PC;
	for(int i = 0; i < 2048; ++i)
		RAM[i] = 0;
}

void tick() {
	//PPU::tick();
	//PPU::tick();
	//PPU::tick();
	//APU::tick();
}

void step() {
	getStateString();
	PC_init = PC;
	uint8_t opcode = memGet(PC);
	memVals[0] = opcode;
	memCnt = 1; //Default logging value if no specific addressing function used
	++PC;
	tick();
	switch (opcode) {
		case 0x69:
			opTxt = "ADC ";
			opADC(Immediate());
			break;
		case 0x65:
			opTxt = "ADC ";
			opADC(ZeroPage());
			break;
		case 0x75:
			opTxt = "ADC ";
			opADC(ZeroPageX());
			break;
		case 0x6D:
			opTxt = "ADC ";
			opADC(Absolute());
			break;
		case 0x7D:
			opTxt = "ADC ";
			opADC(AbsoluteX());
			break;
		case 0x79:
			opTxt = "ADC ";
			opADC(AbsoluteY());
			break;
		case 0x61:
			opTxt = "ADC ";
			opADC(IndirectX());
			break;
		case 0x71:
			opTxt = "ADC ";
			opADC(IndirectY());
			break;
		case 0x29:
			opTxt = "AND ";
			opAND(Immediate());
			break;
		case 0x25:
			opTxt = "AND ";
			opAND(ZeroPage());
			break;
		case 0x35:
			opTxt = "AND ";
			opAND(ZeroPageX());
			break;
		case 0x2D:
			opTxt = "AND ";
			opAND(Absolute());
			break;
		case 0x3D:
			opTxt = "AND ";
			opAND(AbsoluteX());
			break;
		case 0x39:
			opTxt = "AND ";
			opAND(AbsoluteY());
			break;
		case 0x21:
			opTxt = "AND ";
			opAND(IndirectX());
			break;
		case 0x31:
			opTxt = "AND ";
			opAND(IndirectY());
			break;
		case 0x0A:
			opTxt = "ASL ";
			opASL();
			break;
		case 0x06:
			opTxt = "ASL ";
			opASL(ZeroPage());
			break;
		case 0x16:
			opTxt = "ASL ";
			opASL(ZeroPageX());
			break;
		case 0x0E:
			opTxt = "ASL ";
			opASL(Absolute());
			break;
		case 0x1E:
			opTxt = "ASL ";
			opASL(AbsoluteX());
			break;
		case 0x90:
			opTxt = "BCC ";
			opBCC();
			break;
		case 0xB0:
			opTxt = "BCS ";
			opBCS();
			break;
		case 0xF0:
			opTxt = "BEQ ";
			opBEQ();
			break;
		case 0x24:
			opTxt = "BIT ";
			opBIT(ZeroPage());
			break;
		case 0x2C:
			opTxt = "BIT ";
			opBIT(Absolute());
			break;
		case 0x30:
			opTxt = "BMI ";
			opBMI();
			break;
		case 0xD0:
			opTxt = "BNE ";
			opBNE();
			break;
		case 0x10:
			opTxt = "BPL ";
			opBPL();
			break;	
		case 0x00:
			opTxt = "BRK ";
			opBRK();
			break;
		case 0x50:
			opTxt = "BVC ";
			opBVC();
			break;
		case 0x70:
			opTxt = "BVS ";
			opBVS();
			break;
		case 0x18:
			opTxt = "CLC ";
			opCLC();
			break;
		case 0xD8:
			opTxt = "CLD ";
			opCLD();
			break;
		case 0x58:
			opTxt = "CLI ";
			opCLI();
			break;
		case 0xB8:
			opTxt = "CLV ";
			opCLV();
			break;
		case 0xC9:
			opTxt = "CMP ";
			opCMP(Immediate());
			break;
		case 0xC5:
			opTxt = "CMP ";
			opCMP(ZeroPage());
			break;
		case 0xD5:
			opTxt = "CMP ";
			opCMP(ZeroPageX());
			break;
		case 0xCD:
			opTxt = "CMP ";
			opCMP(Absolute());
			break;
		case 0xDD:
			opTxt = "CMP ";
			opCMP(AbsoluteX());
			break;
		case 0xD9:
			opTxt = "CMP ";
			opCMP(AbsoluteY());
			break;
		case 0xC1:
			opTxt = "CMP ";
			opCMP(IndirectX());
			break;
		case 0xD1:
			opTxt = "CMP ";
			opCMP(IndirectY());
			break;
		case 0xE0:
			opTxt = "CPX ";
			opCPX(Immediate());
			break;
		case 0xE4:
			opTxt = "CPX ";
			opCPX(ZeroPage());
			break;
		case 0xEC:
			opTxt = "CPX ";
			opCPX(Absolute());
			break;
		case 0xC0:
			opTxt = "CPY ";
			opCPY(Immediate());
			break;
		case 0xC4:
			opTxt = "CPY ";
			opCPY(ZeroPage());
			break;
		case 0xCC:
			opTxt = "CPY ";
			opCPY(Absolute());
			break;
		case 0xC6:
			opTxt = "DEC ";
			opDEC(ZeroPage());
			break;
		case 0xD6:
			opTxt = "DEC ";
			opDEC(ZeroPageX());
			break;
		case 0xCE:
			opTxt = "DEC ";
			opDEC(Absolute());
			break;
		case 0xDE:
			opTxt = "DEC ";
			opDEC(AbsoluteX());
			break;
		case 0xCA:
			opTxt = "DEX ";
			opDEX();
			break;
		case 0x88:
			opTxt = "DEY ";
			opDEY();
			break;
		case 0x49:
			opTxt = "EOR ";
			opEOR(Immediate());
			break;
		case 0x45:
			opTxt = "EOR ";
			opEOR(ZeroPage());
			break;
		case 0x55:
			opTxt = "EOR ";
			opEOR(ZeroPageX());
			break;
		case 0x4D:
			opTxt = "EOR ";
			opEOR(Absolute());
			break;
		case 0x5D:
			opTxt = "EOR ";
			opEOR(AbsoluteX());
			break;
		case 0x59:
			opTxt = "EOR ";
			opEOR(AbsoluteY());
			break;
		case 0x41:
			opTxt = "EOR ";
			opEOR(IndirectX());
			break;
		case 0x51:
			opTxt = "EOR ";
			opEOR(IndirectY());
			break;
		case 0xE6:
			opTxt = "INC ";
			opINC(ZeroPage());
			break;
		case 0xF6:
			opTxt = "INC ";
			opINC(ZeroPageX());
			break;
		case 0xEE:
			opTxt = "INC ";
			opINC(Absolute());
			break;
		case 0xFE:
			opTxt = "INC ";
			opINC(AbsoluteX());
			break;
		case 0xE8:
			opTxt = "INX ";
			opINX();
			break;
		case 0xC8:
			opTxt = "INY ";
			opINY();
			break;
		case 0x4C:
			opTxt = "JMP ";
			opJMP(Absolute());
			break;
		case 0x6C:
			opTxt = "JMP ";
			opJMP(Indirect());
			break;
		case 0x20:
			opTxt = "JSR ";
			opJSR(Absolute());
			break;
		case 0xA9:
			opTxt = "LDA ";
			opLDA(Immediate());
			break;
		case 0xA5:
			opTxt = "LDA ";
			opLDA(ZeroPage());
			break;
		case 0xB5:
			opTxt = "LDA ";
			opLDA(ZeroPageX());
			break;
		case 0xAD:
			opTxt = "LDA ";
			opLDA(Absolute());
			break;
		case 0xBD:
			opTxt = "LDA ";
			opLDA(AbsoluteX());
			break;
		case 0xB9:
			opTxt = "LDA ";
			opLDA(AbsoluteY());
			break;
		case 0xA1:
			opTxt = "LDA ";
			opLDA(IndirectX());
			break;
		case 0xB1:
			opTxt = "LDA ";
			opLDA(IndirectY());
			break;
		case 0xA2:
			opTxt = "LDX ";
			opLDX(Immediate());
			break;
		case 0xA6:
			opTxt = "LDX ";
			opLDX(ZeroPage());
			break;
		case 0xB6:
			opTxt = "LDX ";
			opLDX(ZeroPageY());
			break;
		case 0xAE:
			opTxt = "LDX ";
			opLDX(Absolute());
			break;
		case 0xBE:
			opTxt = "LDX ";
			opLDX(AbsoluteY());
			break;
		case 0xA0:
			opTxt = "LDY ";
			opLDY(Immediate());
			break;
		case 0xA4:
			opTxt = "LDY ";
			opLDY(ZeroPage());
			break;
		case 0xB4:
			opTxt = "LDY ";
			opLDY(ZeroPageX());
			break;
		case 0xAC:
			opTxt = "LDY ";
			opLDY(Absolute());
			break;
		case 0xBC:
			opTxt = "LDY ";
			opLDY(AbsoluteX());
			break;
		case 0x4A:
			opTxt = "LSR ";
			opLSR();
			break;
		case 0x46:
			opTxt = "LSR ";
			opLSR(ZeroPage());
			break;
		case 0x56:
			opTxt = "LSR ";
			opLSR(ZeroPageX());
			break;
		case 0x4E:
			opTxt = "LSR ";
			opLSR(Absolute());
			break;
		case 0x5E:
			opTxt = "LSR ";
			opLSR(AbsoluteX());
			break;
		case 0xEA:
			opTxt = "NOP ";
			opNOP();
			break;
		case 0x09:
			opTxt = "ORA ";
			opORA(Immediate());
			break;
		case 0x05:
			opTxt = "ORA ";
			opORA(ZeroPage());
			break;
		case 0x15:
			opTxt = "ORA ";
			opORA(ZeroPageX());
			break;
		case 0x0D:
			opTxt = "ORA ";
			opORA(Absolute());
			break;
		case 0x1D:
			opTxt = "ORA ";
			opORA(AbsoluteX());
			break;
		case 0x19:
			opTxt = "ORA ";
			opORA(AbsoluteY());
			break;
		case 0x01:
			opTxt = "ORA ";
			opORA(IndirectX());
			break;
		case 0x11:
			opTxt = "ORA ";
			opORA(IndirectY());
			break;
		case 0x48:
			opTxt = "PHA ";
			opPHA();
			break;
		case 0x08:
			opTxt = "PHP ";
			opPHP();
			break;
		case 0x68:
			opTxt = "PLA ";
			opPLA();
			break;
		case 0x28:
			opTxt = "PLP ";
			opPLP();
			break;
		case 0x2A:
			opTxt = "ROL ";
			opROL();
			break;
		case 0x26:
			opTxt = "ROL ";
			opROL(ZeroPage());
			break;
		case 0x36:
			opTxt = "ROL ";
			opROL(ZeroPageX());
			break;
		case 0x2E:
			opTxt = "ROL ";
			opROL(Absolute());
			break;
		case 0x3E:
			opTxt = "ROL ";
			opROL(AbsoluteX());
			break;
		case 0x6A:
			opTxt = "ROR ";
			opROR();
			break;
		case 0x66:
			opTxt = "ROR ";
			opROR(ZeroPage());
			break;
		case 0x76:
			opTxt = "ROR ";
			opROR(ZeroPageX());
			break;
		case 0x6E:
			opTxt = "ROR ";
			opROR(Absolute());
			break;
		case 0x7E:
			opTxt = "ROR ";
			opROR(AbsoluteX());
			break;
		case 0x40:
			opTxt = "RTI ";
			opRTI();
			break;
		case 0x60:
			opTxt = "RTS ";
			opRTS();
			break;
		case 0xE9:
			opTxt = "SBC ";
			opSBC(Immediate());
			break;
		case 0xE5:
			opTxt = "SBC ";
			opSBC(ZeroPage());
			break;
		case 0xF5:
			opTxt = "SBC ";
			opSBC(ZeroPageX());
			break;
		case 0xED:
			opTxt = "SBC ";
			opSBC(Absolute());
			break;
		case 0xFD:
			opTxt = "SBC ";
			opSBC(AbsoluteX());
			break;
		case 0xF9:
			opTxt = "SBC ";
			opSBC(AbsoluteY());
			break;
		case 0xE1:
			opTxt = "SBC ";
			opSBC(IndirectX());
			break;
		case 0xF1:
			opTxt = "SBC ";
			opSBC(IndirectY());
			break;
		case 0x38:
			opTxt = "SEC ";
			opSEC();
			break;
		case 0xF8:
			opTxt = "SED ";
			opSED();
			break;
		case 0x78:
			opTxt = "SEI ";
			opSEI();
			break;
		case 0x85:
			opTxt = "STA ";
			opSTA(ZeroPage());
			break;
		case 0x95:
			opTxt = "STA ";
			opSTA(ZeroPageX());
			break;
		case 0x8D:
			opTxt = "STA ";
			opSTA(Absolute());
			break;
		case 0x9D:
			opTxt = "STA ";
			opSTA(AbsoluteX());
			break;
		case 0x99:
			opTxt = "STA ";
			opSTA(AbsoluteY());
			break;
		case 0x81:
			opTxt = "STA ";
			opSTA(IndirectX());
			break;
		case 0x91:
			opTxt = "STA ";
			opSTA(IndirectY());
			break;
		case 0x86:
			opTxt = "STX ";
			opSTX(ZeroPage());
			break;
		case 0x96:
			opTxt = "STX ";
			opSTX(ZeroPageY());
			break;
		case 0x8E:
			opTxt = "STX ";
			opSTX(Absolute());
			break;
		case 0x84:
			opTxt = "STY ";
			opSTY(ZeroPage());
			break;
		case 0x94:
			opTxt = "STY ";
			opSTY(ZeroPageX());
			break;
		case 0x8C:
			opTxt = "STY ";
			opSTY(Absolute());
			break;
		case 0xAA:
			opTxt = "TAX ";
			opTAX();
			break;
		case 0xA8:
			opTxt = "TAY ";
			opTAY();
			break;
		case 0xBA:
			opTxt = "TSX ";
			opTSX();
			break;
		case 0x8A:
			opTxt = "TXA ";
			opTXA();
			break;
		case 0x9A:
			opTxt = "TXS ";
			opTXS();
			break;
		case 0x98:
			opTxt = "TYA ";
			opTYA();
			break;
		default:
			std::cout << std::hex << std::uppercase;
			std::cout << std::setw(4) << static_cast<int>(PC-1) << ": "
			<< std::setw(2) << static_cast<int>(opcode) << " is invalid opcode"
			<< std::endl;
			throw 1;
			break;
	}
	
	logStep();

}


void OAMDMA_write() {
	
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
		<< " CYC:" << std::dec << std::setfill(' ') << std::setw(3) << 0
		<< " SL:" << std::setw(3) << 0;
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
	opTxt += "#$" + int_to_hex(memVals[1]);
	return addr;
}

uint16_t ZeroPage() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	opTxt += "$" + int_to_hex(addr);
	return addr;
}

uint16_t ZeroPageX() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	addr += X;
	tick();
	opTxt += "$" + int_to_hex(memVals[1]) + ",X @ " + int_to_hex(addr);
	return addr;
}

uint16_t ZeroPageY() {
	uint8_t addr = memGet(PC);
	memVals[1] = addr;
	memCnt = 2;
	++PC;
	tick();
	addr += Y;
	tick();
	opTxt += "$" + int_to_hex(memVals[1]) + ",Y @ " + int_to_hex(addr);
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
	opTxt += "$" + int_to_hex(addr);
	return addr;
}

uint16_t AbsoluteX() {
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
	if (((addr&0x0F) + X) > 0x0F)
		tick();
	opTxt += "$" + int_to_hex(addr) + ",X @ ";
	addr += X;	
	opTxt += int_to_hex(addr);
	return addr;
}

uint16_t AbsoluteY() {
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
	if (((addr&0x0F) + Y) > 0x0F)
		tick();
	opTxt += "$" + int_to_hex(addr) + ",Y @ ";
	addr += Y;
	opTxt += int_to_hex(addr);	
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
	opTxt += "$" + int_to_hex(addr_loc);
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
	iaddr += X;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	opTxt += "($" + int_to_hex(memVals[1]) + ",X) @ " + int_to_hex(iaddr) + " = " + int_to_hex(addr);
	return addr;
}

uint16_t IndirectY() {
	uint8_t iaddr = memGet(PC);
	memVals[1] = static_cast<uint8_t>(iaddr);
	memCnt = 2;
	++PC;
	tick();
	uint16_t addr = memGet(iaddr);
	tick();
	addr |= ((uint16_t)memGet((uint8_t)(iaddr+1)) << 8);
	tick();
	if (((addr&0x0F) + Y) > 0x0F)
		tick();
	opTxt += "($" + int_to_hex(memVals[1]) + "),Y = " + int_to_hex(addr) + " @ ";
	addr += Y;
	opTxt += int_to_hex(addr);
	return addr;
}



//CPU operation functions
void opADC(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	uint8_t oldA = A;
	A += M + P.b.C;
	P.b.C = (A < oldA);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
}

void opAND(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	A &= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opASL() {
	P.b.C = (A >> 7) > 0;
	A = A << 1;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opASL(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	P.b.Z = (A == 0);
	P.b.N = (M >> 7) > 0;
	memSet(addr, M);
	tick();
}

void opBCC() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.C == 0) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBCS() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.C) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBEQ() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.Z) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBIT(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
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
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBNE() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.Z == 0) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBPL() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.N == 0) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBRK() {
	memSet(((uint16_t)0x01 << 8) | SP, PC >> 8);
	--SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | SP, PC);
	--SP;
	tick();
	memSet(((uint16_t)0x01 << 8) | SP, P.raw);
	--SP;
	tick();
	uint16_t addr = memGet(0xFFFE);
	tick();
	addr |= memGet(0xFFFF) << 8;
	tick();
	PC = addr;
	tick();
	P.b.B = 1;
}

void opBVC()  {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.V == 0) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opBVS() {
	int8_t delta = static_cast<int8_t>(memGet(PC));
	memVals[1] = static_cast<uint8_t>(delta);
	memCnt = 2;
	++PC;
	tick();
	if(P.b.V) {
		if (((PC&0x0F) + delta) > 0x0F)
			tick();
		PC += delta;
		tick();
	}
}

void opCLC() {
	P.b.C = 0;
	tick();
}

void opCLD() {
	P.b.D = 0;
	tick();
}

void opCLI() {
	P.b.I = 0;
	tick();
}

void opCLV() {
	P.b.V = 0;
	tick();
}

void opCMP(uint8_t M) {
	P.b.C = (A >= M);
	P.b.Z = (A == M);
	P.b.N = ((uint8_t)(A-M)>>7) == 1;
	tick();
}

void opCMP(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	P.b.C = (A >= M);
	P.b.Z = (A == M);
	P.b.N = ((uint8_t)(A-M)>>7) == 1;
	tick();
}

void opCPX(uint8_t M) {
	opTxt += int_to_hex(M);
	P.b.C = (X >= M);
	P.b.Z = (X == M);
	P.b.N = ((uint8_t)(X-M)>>7) == 1;
	tick();
}

void opCPX(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	P.b.C = (X >= M);
	P.b.Z = (X == M);
	P.b.N = ((uint8_t)(X-M)>>7) == 1;
	tick();
}

void opCPY(uint8_t M) {
	opTxt += int_to_hex(M);
	P.b.C = (Y >= M);
	P.b.Z = (Y == M);
	P.b.N = ((uint8_t)(Y-M)>>7) == 1;
	tick();
}

void opCPY(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	P.b.C = (Y >= M);
	P.b.Z = (Y == M);
	P.b.N = ((uint8_t)(Y-M)>>7) == 1;
	tick();
}

void opDEC(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	M -= 1;
	memSet(addr, M);
	tick();
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
}

void opDEX() {
	X -= 1;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opDEY() {
	Y -= 1;
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	tick();
}

void opEOR(void) {
	uint8_t M = memGet(PC);
	opTxt += int_to_hex(M);
	A ^= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opEOR(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	A ^= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opINC(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	M += 1;
	memSet(addr, M);
	tick();
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
}

void opINX() {
	X += 1;
	tick();
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opINY() {
	Y += 1;
	tick();
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	tick();
}

void opJMP(uint16_t addr) {
	PC = addr;
	tick();
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
}

void opLDA() {
	A = memGet(PC);
	++PC;
	tick();
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opLDA(uint16_t addr) {
	A = memGet(addr);
	opTxt += " = " + int_to_hex(A);
	tick();
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opLDX() {
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
}

void opLDY() {
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
}

void opLSR() {
	P.b.C = A & 1;
	A = A >> 1;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opLSR(uint16_t addr) {
	uint8_t M = memGet(addr);
	tick();
	P.b.C = M & 1;
	M = M >> 1;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (M >> 7) > 0;
	tick();
}

void opNOP() {
	tick();
}

void opORA(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	A |= M;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
}

void opPHA() {
	memSet(((uint16_t)0x01 << 8) | SP, A);
	--SP;
	tick();
	tick();
}

void opPHP() {
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
}

void opPLP() {
	++SP;
	tick();
	P.raw = memGet(((uint16_t)0x01 << 8) | SP);
	tick();
	tick();
}

void opROL() {
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
	opTxt += int_to_hex(M);
	tick();
	uint8_t C0 = P.b.C;
	P.b.C = (M >> 7) > 0;
	M = M << 1;
	M |= C0;
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opROR() {
	uint8_t C0 = P.b.C;
	P.b.C = (A & 1);
	A = A >> 1;
	A |= (C0 << 7);
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opROR(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
	tick();
	uint8_t C0 = P.b.C;
	P.b.C = (M & 1);
	M = M >> 1;
	M |= (C0 << 7);
	memSet(addr, M);
	P.b.Z = (M == 0);
	P.b.N = (A >> 7) > 0;
	tick();
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
}

void opSBC(uint16_t addr) {
	uint8_t M = memGet(addr);
	opTxt += int_to_hex(M);
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

	P.b.C = (A < oldA);
	P.b.V = ((oldA^A)&(M^A)&0x80) != 0;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) == 1;
}

void opSEC() {
	P.b.C = 1;
	tick();
}

void opSED() {
	P.b.D = 1;
	tick();
}

void opSEI() {
	P.b.I = 1;
	tick();
}

void opSTA(uint16_t addr) {
	memSet(addr, A);
	tick();
}

void opSTX(uint16_t addr) {
	memSet(addr, X);
	tick();
}

void opSTY(uint16_t addr) {
	memSet(addr, Y);
	tick();
}

void opTAX() {
	X = A;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opTAY() {
	Y = A;
	P.b.Z = (Y == 0);
	P.b.N = (Y >> 7) > 0;
	tick();
}

void opTSX() {
	X = SP;
	P.b.Z = (X == 0);
	P.b.N = (X >> 7) > 0;
	tick();
}

void opTXA() {
	A = X;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

void opTXS() {
	SP = X;
	tick();
}

void opTYA() {
	A = Y;
	P.b.Z = (A == 0);
	P.b.N = (A >> 7) > 0;
	tick();
}

}