#include "mapper.h"
#include "mapper0.h"

uint8_t Mapper::memGet(uint16_t addr, bool peek) { return 0; };
void Mapper::memSet(uint16_t addr, uint8_t val) {};
uint8_t Mapper::PPUmemGet(uint16_t addr, bool peek) { return 0; };
void Mapper::PPUmemSet(uint16_t addr, uint8_t val) {};
void Mapper::loadData(std::ifstream &file) {};
void Mapper::powerOn() {};
void Mapper::reset() {};
void Mapper::CPUstep() {};
void Mapper::PPUstep() {};
void Mapper::PPUbusAddrChanged(uint16_t newAddr) {};