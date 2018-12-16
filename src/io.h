#ifndef IO_H
#define IO_H

#include <stdint.h>

namespace IO
{

void init();
uint8_t regGet(uint16_t addr);
void regSet(uint16_t addr, uint8_t val);
uint8_t* getControllerStatePtr();

}


#endif