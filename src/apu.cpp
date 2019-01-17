#include "apu.h"
#include "cpu.h"
#include <array>
#include <iostream>

namespace APU {

bool enablePulse1, enablePulse2, enableNoise, enableTriangle, enableDMC;  
bool haltPulse1LC, haltPulse2LC, haltNoiseLC, haltTriangleLC;
bool constVolPulse1, constVolPulse2;
bool sweepEnPulse1, sweepEnPulse2;
bool sweepNPulse1, sweepNPulse2;
bool DMCinterruptInhibit, frameInterruptInhibit;
bool DMCinterruptRequest, frameInterruptRequest;
bool fiveFrameMode;
uint8_t pulse1_lenCntr, pulse2_lenCntr, triangle_lenCntr, noise_lenCntr;
uint8_t volPeriodPulse1, volPeriodPulse2;
uint8_t sweepPeriodPulse1, sweepPeriodPulse2;
uint8_t sweepShiftPulse1, sweepShiftPulse2;
uint16_t timerPulse1, timerPulse2, timerSetPulse1, timerSetPulse2;
uint8_t outputPulse1, outputPulse2, outputTriangle, outputNoise, outputDMC;
int dutyIdxPulse1, dutyIdxPulse2;
unsigned int frameHalfCycle;
unsigned long long cycle = 0;

bool audioBufferReady = false;
int outputBufferIdx = 0;
int outputBufferSel = 0;
uint16_t* outputBufferPtr;
std::array<std::array<uint16_t, OUTPUT_AUDIO_BUFFER_SIZE>,2> outputBuffers;

std::array<int,8> dutyCyclePulse1, dutyCyclePulse2;

std::array<uint8_t,0x20> lengthCounterArray = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

std::array<uint16_t, 31> pulseMixerTable;
std::array<uint16_t, 203> tndMixerTable;

std::array<std::array<int,8>,4> pulseDutyCycleTable = {
    std::array<int,8> {0,1,0,0,0,0,0,0},
    std::array<int,8> {0,1,1,0,0,0,0,0},
    std::array<int,8> {0,1,1,1,1,0,0,0},
    std::array<int,8> {1,0,0,1,1,1,1,1}
};


void init()
{
    generateMixerTables();
    regSet(0x4015, 0); //Disables all channels
    regSet(0x4017, 0);
    for(uint16_t addr = 0x4000; addr < 0x4010; ++addr)
        regSet(addr, 0);
    frameHalfCycle = 0;
}

void step()
{
    switch(frameHalfCycle) {
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
                frameHalfCycle = 0;
            }
            break;
        case 37281: //18640.5 full cycles
            //inc envelopes and trianble linear counter
            decLengthCounters();
            break;
        case 37282: //18641 full cycles
            frameHalfCycle = 0;
            break;
    }
    ++frameHalfCycle;

    if(cycle % 2 == 0) { //On even frameHalfCycles clock channels
        stepPulse1();
        stepPulse2();
        //TODO: May not trigger every other frameHalfCycle. Placeholder for now
        stepTriangle();
        stepNoise();
        stepDMC();
    }

    if(cycle % 75 == 0) { //Every ~48000 Hz
        mixOutput();
    }

    if(outputBufferIdx >= OUTPUT_AUDIO_BUFFER_SIZE) {
        //Start filling other buffer and flag that a full buffer is ready to be outputted
        outputBufferPtr = outputBuffers[outputBufferSel].data();
        audioBufferReady = true;
        outputBufferIdx = 0;
        outputBufferSel = (outputBufferSel == 0) ? 1 : 0;
    }

    if(frameInterruptRequest)
        CPU::triggerIRQ();
    
    cycle++;
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
            dutyCyclePulse1 = pulseDutyCycleTable[val >> 6];
            haltPulse1LC = (val & 0x20) > 0;
            constVolPulse1 = (val & 0x10) > 0;
            volPeriodPulse1 = (val & 0x0F);
            break;
        case 0x4001:
            sweepEnPulse1 = (val & 0x80) > 0;
            sweepPeriodPulse1 = (val >> 4) & 0x7;
            sweepNPulse1 = (val & 0x8) > 0;
            sweepShiftPulse1 = val & 0x7;
            break;
        case 0x4002:
            timerSetPulse1 = (timerPulse1 & 0x700) | val;
            break;
        case 0x4003:
            timerSetPulse1 = (((uint16_t)val & 0x7) << 8) | (timerPulse1 & 0xFF);
            if(enablePulse1) pulse1_lenCntr = lengthCounterArray[val >> 3];
            timerPulse1 = timerSetPulse1;
            dutyIdxPulse1 = 0;
            //TODO: restart envelope
            break;
        case 0x4004:
            dutyCyclePulse2 = pulseDutyCycleTable[val >> 6];
            haltPulse2LC = (val & 0x20) > 0;
            constVolPulse2 = (val & 0x10) > 0;
            volPeriodPulse2 = (val & 0x0F);
            break;
        case 0x4005:
            sweepEnPulse2 = (val & 0x80) > 0;
            sweepPeriodPulse2 = (val >> 4) & 0x7;
            sweepNPulse2 = (val & 0x8) > 0;
            sweepShiftPulse2 = val & 0x7;
            break;
        case 0x4006:
            timerSetPulse1 = (timerPulse1 & 0x700) | val;
            break;
        case 0x4007:
            timerSetPulse2 = (((uint16_t)val & 0x7) << 8) | (timerPulse2 & 0xFF);
            if(enablePulse2) pulse2_lenCntr = lengthCounterArray[val >> 3];
            dutyIdxPulse2 = 0;
            //TODO: restart envelope
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
            frameHalfCycle = 0;
            if(fiveFrameMode) { //Clock immediately
                //inc envelopes and trianble linear counter
                decLengthCounters();
            }
            break;
    }
}

void stepPulse1() {
    if(timerPulse1 > 0) {
        --timerPulse1;
    }
    else {
        timerPulse1 = timerSetPulse1;
        dutyIdxPulse1 = (dutyIdxPulse1 + 1) % 8;
    }

    if(pulse1_lenCntr > 0) {
        outputPulse1 = dutyCyclePulse1[dutyIdxPulse1] * volPeriodPulse1;
    }
    else {
        outputPulse1 = 0;
    }
}

void stepPulse2() {
    if(timerPulse2 > 0) {
        --timerPulse2;
    }
    else {
        timerPulse2 = timerSetPulse2;
        dutyIdxPulse2 = (dutyIdxPulse2 + 1) % 8;
    }

    if(pulse2_lenCntr > 0) {
        outputPulse2 = dutyCyclePulse2[dutyIdxPulse2] * volPeriodPulse2;
    }
    else {
        outputPulse2 = 0;
    }
}

void stepTriangle() {
    //TODO: Implement channel
    outputTriangle = 0;
}

void stepNoise() {
    //TODO: Implement channel
    outputNoise = 0;
}

void stepDMC() {
    //TODO: Implement channel
    outputDMC = 0;
}


void mixOutput() {
    uint16_t output;
    output = pulseMixerTable[outputPulse1 + outputPulse2];
    output += tndMixerTable[3 * outputTriangle + 2 * outputNoise + outputDMC];
    outputBuffers[outputBufferSel][outputBufferIdx] = output;
    ++outputBufferIdx;
}

void generateMixerTables() {
    //Using info from http://wiki.nesdev.com/w/index.php/APU_Mixer
    //Generates lookup tables to speed up processing time
    //Use 0-1000 instead of 0-1
    pulseMixerTable[0] = 0;
    tndMixerTable[0] = 0;
    for(unsigned int i=1; i<pulseMixerTable.size(); ++i) {
        pulseMixerTable[i] = (95.52f / (8128.0f / i + 100.0f)) * 65535;
    }
    for(unsigned int i=1; i<tndMixerTable.size(); ++i) {
        tndMixerTable[i] = (163.67f / (24329.0f / i + 100.0f)) * 65535;
    }
}


}