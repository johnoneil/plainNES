#include "mapper.h"
#include "mapper0.h"

uint8_t Mapper::memGet(uint16_t addr) { return 0; };
void Mapper::memSet(uint16_t addr, uint8_t val) {};
uint8_t Mapper::PPUmemGet(uint16_t addr) { return 0; };
void Mapper::PPUmemSet(uint16_t addr, uint8_t val) {};
void Mapper::loadData(GAMEPAK::iNES_Header header, std::ifstream &file) {};