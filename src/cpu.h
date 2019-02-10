#pragma once

#include <stdint.h>
#include <string>

namespace CPU {

enum OpType {
	//To change behavior of addressing modes which depend on type
	//In terms of reading/writing to memory. Not internal registers or flags
	READ,
	WRITE,
	READWRITE,
};

extern uint8_t busVal;

extern unsigned long long cpuCycle;

void powerOn();
void reset();
void step();
void incCycle();
uint8_t cpuRead(uint16_t addr);
void cpuWrite(uint16_t addr, uint8_t val);

uint8_t memGet(uint16_t addr, bool peek = false);
void memSet(uint16_t addr, uint8_t val);

void OAMDMA_write();
void interruptDetect();
void setNMI(bool setLow);
void forceNMI(bool setLow);
void setIRQfromAPU(bool setLow);
void setIRQfromCart(bool setLow);
void setIRQ(bool setLow);
void setPC(uint16_t newPC);

void logStep();
void logInterrupt(std::string txt);

//Addressing functions
uint16_t Immediate();
uint16_t ZeroPage();
uint16_t ZeroPageX();
uint16_t ZeroPageY();
uint16_t Absolute();
uint16_t AbsoluteX(OpType optype);
uint16_t AbsoluteY(OpType optype);
uint16_t Indirect();
uint16_t IndirectX();
uint16_t IndirectY(OpType optype);

//CPU operation functions
void opADC(uint16_t addr);
void opAHX(uint16_t addr);
void opALR(uint16_t addr);
void opANC(uint16_t addr);
void opAND(uint16_t addr);
void opARR(uint16_t addr);
void opASL();
void opASL(uint16_t addr);
void opAXS(uint16_t addr);
void opBCC();
void opBCS();
void opBEQ();
void opBIT(uint16_t addr);
void opBMI();
void opBNE();
void opBPL();
void opBRK();
void opBRKonIRQ();
void opBVC();
void opBVS();
void opCLC();
void opCLD();
void opCLI();
void opCLV();
void opCMP(uint8_t M);
void opCMP(uint16_t addr);
void opCPX(uint8_t M);
void opCPX(uint16_t addr);
void opCPY(uint8_t M);
void opCPY(uint16_t addr);
void opDCP(uint16_t addr);
void opDEC(uint16_t addr);
void opDEX();
void opDEY();
void opEOR(void);
void opEOR(uint16_t addr);
void opINC(uint16_t addr);
void opINX();
void opINY();
void opISC(uint16_t addr);
void opJMP(uint16_t addr);
void opJSR(uint16_t addr);
void opLAS(uint16_t addr);
void opLAX(uint16_t addr);
void opLDA();
void opLDA(uint16_t addr);
void opLDX();
void opLDX(uint16_t addr);
void opLDY();
void opLDY(uint16_t addr);
void opLSR();
void opLSR(uint16_t addr);
void opNOP(uint16_t addr);
void opORA(uint16_t addr);
void opPHA();
void opPHP();
void opPLA();
void opPLP();
void opRLA(uint16_t addr);
void opROL();
void opROL(uint16_t addr);
void opROR();
void opROR(uint16_t addr);
void opRRA(uint16_t addr);
void opRTI();
void opRTS();
void opSAX(uint16_t addr);
void opSBC(uint16_t addr);
void opSEC();
void opSED();
void opSEI();
void opSHX(uint16_t addr);
void opSHY(uint16_t addr);
void opSLO(uint16_t addr);
void opSRE(uint16_t addr);
void opSTA(uint16_t addr);
void opSTX(uint16_t addr);
void opSTY(uint16_t addr);
void opTAS(uint16_t addr);
void opTAX();
void opTAY();
void opTSX();
void opTXA();
void opTXS();
void opTYA();
void opXAA(uint16_t addr);

}