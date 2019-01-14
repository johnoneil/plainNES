#include "apu.h"
#include "cpu.h"
#include <array>

namespace APU {

bool enablePulse1, enablePulse2, enableNoise, enableTriangle, enableDMC;  
bool haltPulse1LC, haltPulse2LC, haltNoiseLC, haltTriangleLC;
bool DMCinterruptInhibit, frameInterruptInhibit;
bool DMCinterruptRequest, frameInterruptRequest;
bool fiveFrameMode;
uint8_t pulse1_lenCntr, pulse2_lenCntr, triangle_lenCntr, noise_lenCntr;
unsigned int halfcycle;

std::array<uint8_t,0x20> lengthCounterArray = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


void init()
{
    regSet(0x4015, 0); //Disables all channels
    regSet(0x4017, 0);
    for(uint16_t addr = 0x4000; addr < 0x4010; ++addr)
        regSet(addr, 0);
    halfcycle = 0;
}

void step()
{
    ++halfcycle;
    switch(halfcycle) {
        case 7457: //3728.5 full cycles
            //inc envelopes and triangle linear counter
            break;
        case 14913: //7456.5 full cycles
            //inc envelopes and triangle linear counter
            decLengthCounters();
            break;
        case 22371: //11185.5 full cycles
            //inc envelopes and triangle linear counter
            break;
        case 29828: //14914 full cycles
            if(fiveFrameMode == 0) {
                if(frameInterruptInhibit == 0) frameInterruptRequest = true;
            }
            break;
        case 29829: //14914.5 full cycles
            if(fiveFrameMode == 0) {
                if(frameInterruptInhibit == 0) frameInterruptRequest = true;
                decLengthCounters();
            }
            break;
        case 29830: //14915 full cycles
            if(fiveFrameMode == 0) {
                if(frameInterruptInhibit == 0) frameInterruptRequest = true;
                halfcycle = 0;
            }
            break;
        case 37281: //18640.5 full cycles
            //inc envelopes and trianble linear counter
            decLengthCounters();
            break;
        case 37282: //18641 full cycles
            halfcycle = 0;
            break;
    }

    if(frameInterruptRequest)
        CPU::triggerIRQ();
}

void decLengthCounters()
{
    if(pulse1_lenCntr) {
        if(enablePulse1 == 0)
            pulse1_lenCntr = 0;
        else if(haltPulse1LC == 0)
            --pulse1_lenCntr;
    }
    if(pulse2_lenCntr) {
        if(enablePulse2 == 0)
            pulse2_lenCntr = 0;
        else if(haltPulse2LC == 0)
            --pulse2_lenCntr;
    }
    if(noise_lenCntr) {
        if(enableNoise == 0)
            noise_lenCntr = 0;
        else if(haltNoiseLC == 0)
            --noise_lenCntr;
    }
    if(triangle_lenCntr) {
        if(enableTriangle == 0)
            triangle_lenCntr = 0;
        else if(haltTriangleLC == 0)
            --triangle_lenCntr;
    }
}

uint8_t regGet(uint16_t addr)
{   
    if(addr == 0x4015) {
        uint8_t val = ((DMCinterruptRequest) ? 1 : 0) << 7;
        val |= ((frameInterruptRequest) ? 1 : 0) << 6;
        //TODO: DMC bytes remaining
        val |= ((noise_lenCntr>0) ? 1 : 0) << 3;
        val |= ((triangle_lenCntr>0) ? 1 : 0) << 2;
        val |= ((pulse2_lenCntr>0) ? 1 : 0) << 1;
        val |= ((pulse1_lenCntr>0) ? 1: 0);
        frameInterruptRequest = false;
        return val;
    }
    return CPU::busVal; //Open bus behavior
}

void regSet(uint16_t addr, uint8_t val)
{
    switch(addr) {
        case 0x4000:
            haltPulse1LC = (val & 0x20) > 0;
            break;
        case 0x4003:
            if(enablePulse1) pulse1_lenCntr = lengthCounterArray[val >> 3];
            break;
        case 0x4004:
            haltPulse2LC = (val & 0x20) > 0;
            break;
        case 0x4007:
            if(enablePulse2) pulse2_lenCntr = lengthCounterArray[val >> 3];
            break;
        case 0x4008:
            haltTriangleLC = (val & 0x80) > 0;
            break;
        case 0x400B:
            if(enableTriangle) triangle_lenCntr = lengthCounterArray[val >> 3];
            break;
        case 0x400C:
            haltTriangleLC = (val & 0x20) > 0;
            break;
        case 0x400F:
            if(enableTriangle) triangle_lenCntr = lengthCounterArray[val >> 3];
            break;
        case 0x4015:
            enableDMC = (val & 0x10) > 0;
            enableNoise = (val & 8) > 0;
            enableTriangle = (val & 4) > 0;
            enablePulse2 = (val & 2) > 0;
            enablePulse1 = (val & 1) > 0;
            break;
        case 0x4017:
            fiveFrameMode = (val & 0x80) > 0;
            frameInterruptInhibit = (val & 0x40) > 0;
            if(frameInterruptInhibit) frameInterruptRequest = false;
            halfcycle = 0;
            if(fiveFrameMode) { //Clock immediately
                //inc envelopes and trianble linear counter
                decLengthCounters();
            }
            break;
    }
}


}