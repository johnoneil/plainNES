#include "io.h"
#include "nes.h"
#include "cpu.h"
#include <array>

namespace IO
{

std::array<uint8_t, 2> controller_shiftR;
bool controllerStrobe;

void init()
{
    NES::controller_state[0] = 0;
    NES::controller_state[1] = 0;
    controller_shiftR[0] = 0;
    controller_shiftR[1] = 0;
    controllerStrobe = false;
}

uint8_t regGet(uint16_t addr)
{
    uint8_t val = 0;
    if(addr == 0x4016) {
        if(controllerStrobe) {
            val = NES::controller_state[0] & 1;
        }
        else {
            val = controller_shiftR[0] & 1;
            controller_shiftR[0] >>= 1;
        }
    }
    else if(addr == 0x4017) {
        if(controllerStrobe) {
            val = NES::controller_state[1] & 1;
        }
        else {
            val = controller_shiftR[1] & 1;
            controller_shiftR[1] >>= 1;
        }
    }
    else {
        return CPU::busVal;
    }
    return (CPU::busVal & 0xE0) | val;
}

void regSet(uint16_t addr, uint8_t val)
{
    if(addr == 0x4016) {
        if((val & 1) > 0) {
            controllerStrobe = true;
        }
        else {
            controller_shiftR[0] = NES::controller_state[0];
            controller_shiftR[1] = NES::controller_state[1];
            controllerStrobe = false;
        }
    }
}

} // IO
