#include "apu.h"
#include "nes.h"
#include "cpu.h"
#include "utils.h"
#include <array>
#include <iostream>

namespace APU {

//Registers
struct PulseReg0 {
	union {
		uint8_t value;
        BitWorker<0, 4> volPeriod;
        BitWorker<4, 1> constVol;
        BitWorker<5, 1> loopDisableLC;
        BitWorker<6, 2> dutyCycleSel;
	};
}  pulse1Reg0, pulse2Reg0;

struct PulseReg1 {
	union {
		uint8_t value;
        BitWorker<0, 3> sweepShiftCount;
        BitWorker<3, 1> sweepNegative;
        BitWorker<4, 3> sweepPeriod;
        BitWorker<7, 1> sweepEnable;
	};
}  pulse1Reg1, pulse2Reg1;

uint8_t pulse1TimerLow, pulse2TimerLow;

struct PulseReg3 {
	union {
		uint8_t value;
        BitWorker<0, 3> timerHigh;
        BitWorker<3, 5> lenCtrLoad;
	};
}  pulse1Reg3, pulse2Reg3;

struct TriReg0 {
	union {
		uint8_t value;
        BitWorker<0, 7> linCtrReloadVal;
        BitWorker<7, 1> lenCtrDisable;
	};
}  triReg0;

uint8_t triangleTimerLow;

struct TriReg2 {
	union {
		uint8_t value;
        BitWorker<0, 3> timerHigh;
        BitWorker<3, 5> lenCtrLoad;
	};
}  triReg2;

struct NoiseReg0 {
	union {
		uint8_t value;
        BitWorker<0, 4> volPeriod;
        BitWorker<4, 1> constVol;
        BitWorker<5, 1> loopDisableLC;
	};
}  noiseReg0;

struct NoiseReg1 {
	union {
		uint8_t value;
        BitWorker<0, 4> noisePeriodSel;
        BitWorker<7, 1> noiseMode;
	};
}  noiseReg1;

struct NoiseReg2 {
	union {
		uint8_t value;
        BitWorker<3, 5> lenCtrLoad;
	};
}  noiseReg2;

struct DMCReg0 {
	union {
		uint8_t value;
        BitWorker<0, 4> freqIdx;
        BitWorker<6, 1> loopSample;
        BitWorker<7, 1> IRQenable;
	};
}  dmcReg0;

struct DMCReg1 {
	union {
		uint8_t value;
        BitWorker<0, 7> directLoad;
	};
}  dmcReg1;

uint16_t dmcTargetAddr;
uint16_t dmcTargetLen;

struct ControlReg {
	union {
		uint8_t value;
        BitWorker<0, 1> enableLCpulse1;
        BitWorker<1, 1> enableLCpulse2;
        BitWorker<2, 1> enableLCtriangle;
        BitWorker<3, 1> enableLCnoise;
        BitWorker<4, 1> enableDMC;
	};
}  controlReg;

struct StatusReg {
	union {
		uint8_t value;
        BitWorker<0, 1> LCpulse1;
        BitWorker<1, 1> LCpulse2;
        BitWorker<2, 1> LCtriangle;
        BitWorker<3, 1> LCnoise;
        BitWorker<4, 1> DMCactive;
        BitWorker<5, 1> openBus;
        BitWorker<6, 1> frameIRQ;
        BitWorker<7, 1> dmcIRQ;
	};
}  statusReg;

struct FrameReg {
	union {
		uint8_t value;
        BitWorker<6, 1> IRQinhibit;
        BitWorker<7, 1> frameMode;
	};
}  frameReg;

//Pulse 1
uint8_t pulse1Volume = 0;
uint8_t pulse1EnvDecay = 0;
bool pulse1StartEnv = false;
uint8_t pulse1EnvDivider = 0;
uint8_t pulse1SweepDivider = 0;
bool pulse1SweepMute = false;
bool pulse1SweepReload = false;
uint8_t pulse1_lenCntr;
uint16_t timerPeriodTargetPulse1;
uint16_t timerPeriodPulse1;
uint16_t timerPulse1;
uint8_t outputPulse1;
int dutyIdxPulse1;
std::array<int,8> dutyCyclePulse1;
std::array<std::array<int,8>,4> pulseDutyCycleTable = {
    std::array<int,8> {0,1,0,0,0,0,0,0},
    std::array<int,8> {0,1,1,0,0,0,0,0},
    std::array<int,8> {0,1,1,1,1,0,0,0},
    std::array<int,8> {1,0,0,1,1,1,1,1}
};
std::array<uint8_t,0x20> lengthCounterArray = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

//Pulse 2
uint8_t pulse2Volume = 0;
uint8_t pulse2EnvDecay = 0;
bool pulse2StartEnv = false;
uint8_t pulse2EnvDivider = 0;
uint8_t pulse2SweepDivider = 0;
bool pulse2SweepMute = false;
bool pulse2SweepReload = false;
uint8_t pulse2_lenCntr;
uint16_t timerPeriodTargetPulse2;
uint16_t timerPeriodPulse2;
uint16_t timerPulse2;
uint8_t outputPulse2;
int dutyIdxPulse2;
std::array<int,8> dutyCyclePulse2;

//Triangle
uint8_t triangle_lenCntr;
uint8_t triangle_linearCntr;
bool triangle_linearCntrReload = false;
uint8_t outputTriangle;
uint16_t timerSetTriangle;
uint16_t timerTriangle;
std::array<uint8_t, 32> triangleOutputArray {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                                     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t triangleOutputArrayIdx = 0;

//Noise
uint8_t noiseVolume = 0;
uint8_t noiseEnvDecay = 0;
bool noiseStartEnv = false;
uint8_t noiseEnvDivider = 0;
uint8_t noise_lenCntr;
uint16_t noiseShiftRegister;
uint16_t timerNoise;
uint8_t outputNoise;
std::array<uint16_t, 16> noiseTimerTable {0x04, 0x08, 0x10, 0x20, 0x40, 0x60, 0x80, 0xA0,
                                          0xCA, 0xFE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4};

//DMC
uint16_t timerDMC;
uint16_t dmcCurrAddr;
uint16_t dmcBytesRemaining = 0;
uint8_t dmcBuffer = 0;
uint8_t dmcShiftRegister = 0;
uint8_t dmcBitsRemaining = 0;
bool dmcSilence = true;
bool DMCinterruptRequest = false;
uint8_t outputDMC = 0;
std::array<uint16_t, 16> dmcRateTable {428, 380, 340, 320, 286, 254, 226, 214,
                                       190, 160, 142, 128, 106, 84, 72, 54};

//Frame Counter
bool frameInterruptRequest = false;
unsigned int frameHalfCycle;
unsigned long long cycle = 0;
bool frameReset = false;

//Audio Mixer
std::array<float, 31> pulseMixerTable;
std::array<float, 203> tndMixerTable;

void powerOn()
{
    generateMixerTables();
    regSet(0x4015, 0); //Disables all channels
    regSet(0x4017, 0);
    for(uint16_t addr = 0x4000; addr < 0x4010; ++addr)
        regSet(addr, 0);
    frameHalfCycle = 0;
    noiseShiftRegister = 1;
}

void reset()
{
    
}

void step()
{
    if(frameReset && (cycle % 2) == 0) {
        frameHalfCycle = 0;
        frameReset = false;
    }
    switch(frameHalfCycle) {
        case 7459: //3728.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            break;
        case 14915: //7456.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            clockLengthCounters();
            clockSweep();
            break;
        case 22373: //11185.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            break;
        case 29830: //14914 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
            }
            break;
        case 29831: //14914.5 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
                clockEnvelopes();
                clockLinearCounter();
                clockLengthCounters();
                clockSweep();
            }
            break;
        case 29832: //14915 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
                frameHalfCycle = 2;
            }
            break;
        case 37283: //18640.5 full cycles
            if(frameReg.frameMode > 0) {
                clockEnvelopes();
                clockLinearCounter();
                clockLengthCounters();
                clockSweep();
                frameHalfCycle = 1;
            }
            break;
        case 37289:
            clockEnvelopes();
            clockLinearCounter();
            frameHalfCycle = 7459;
            break;
    }
    ++frameHalfCycle;

    if(cycle % 2 == 0) { //On every other clock cycle
        stepPulse1();
        stepPulse2();
        stepNoise();
        stepDMC();
    }
    stepTriangle();

    mixOutput();

    if(frameInterruptRequest || DMCinterruptRequest)
        CPU::triggerIRQ();
    else
        CPU::unsetIRQ();
    
    cycle++;
}


void clockLengthCounters()
{
    if(pulse1_lenCntr) {
        if(controlReg.enableLCpulse1 == 0)
            pulse1_lenCntr = 0;
        else if(pulse1Reg0.loopDisableLC == 0)
            --pulse1_lenCntr;
    }
    if(pulse2_lenCntr) {
        if(controlReg.enableLCpulse2 == 0)
            pulse2_lenCntr = 0;
        else if(pulse2Reg0.loopDisableLC == 0)
            --pulse2_lenCntr;
    }
    if(noise_lenCntr) {
        if(controlReg.enableLCnoise == 0)
            noise_lenCntr = 0;
        else if(noiseReg0.loopDisableLC == 0)
            --noise_lenCntr;
    }
    if(triangle_lenCntr) {
        if(controlReg.enableLCtriangle == 0)
            triangle_lenCntr = 0;
        else if(triReg0.lenCtrDisable == 0)
            --triangle_lenCntr;
    }
}

void clockEnvelopes() {
    //Pulse 1
    if(pulse1StartEnv) {
        //Start flag set (due to $4003 write)
        pulse1StartEnv = false;
        pulse1EnvDecay = 15;
        pulse1EnvDivider = pulse1Reg0.volPeriod;
    }
    else {
        //Decrement divider
        if(pulse1EnvDivider > 0) {
            --pulse1EnvDivider;
        }
        else {
            //Divider at 0
            pulse1EnvDivider = pulse1Reg0.volPeriod;
            //Decrement decay counter
            if(pulse1EnvDecay > 0) {
                --pulse1EnvDecay;
            }
            else if(pulse1Reg0.loopDisableLC) {
                pulse1EnvDecay = 15;
            }
        }
    }
    if(pulse1Reg0.constVol) pulse1Volume = pulse1Reg0.volPeriod;
    else pulse1Volume = pulse1EnvDecay;


    //Pulse 2
    if(pulse2StartEnv) {
        //Start flag set (due to $4003 write)
        pulse2StartEnv = false;
        pulse2EnvDecay = 15;
        pulse2EnvDivider = pulse2Reg0.volPeriod;
    }
    else {
        //Decrement divider
        if(pulse2EnvDivider > 0) {
            --pulse2EnvDivider;
        }
        else {
            //Divider at 0
            pulse2EnvDivider = pulse2Reg0.volPeriod;
            //Decrement decay counter
            if(pulse2EnvDecay > 0) {
                --pulse2EnvDecay;
            }
            else if(pulse2Reg0.loopDisableLC) {
                pulse2EnvDecay = 15;
            }
        }
    }
    if(pulse2Reg0.constVol) pulse2Volume = pulse2Reg0.volPeriod;
    else pulse2Volume = pulse2EnvDecay;


    //Noise
    if(noiseStartEnv) {
        //Start flag set (due to $4003 write)
        noiseStartEnv = false;
        noiseEnvDecay = 15;
        noiseEnvDivider = noiseReg0.volPeriod;
    }
    else {
        //Decrement divider
        if(noiseEnvDivider > 0) {
            --noiseEnvDivider;
        }
        else {
            //Divider at 0
            noiseEnvDivider = noiseReg0.volPeriod;
            //Decrement decay counter
            if(noiseEnvDecay > 0) {
                --noiseEnvDecay;
            }
            else if(noiseReg0.loopDisableLC) {
                noiseEnvDecay = 15;
            }
        }
    }
    if(noiseReg0.constVol) noiseVolume = noiseReg0.volPeriod;
    else noiseVolume = noiseEnvDecay;
}

void clockSweep() {
    int16_t periodDelta;

    //Pulse 1
    if(pulse1Reg1.sweepEnable == 1 && pulse1SweepDivider == 0 && pulse1SweepMute == 0) {
        periodDelta = timerPeriodTargetPulse1 >> pulse1Reg1.sweepShiftCount;
        if(pulse1Reg1.sweepNegative == 1) timerPeriodTargetPulse1 -= periodDelta - 1;
        else timerPeriodTargetPulse1 += periodDelta;
        
        timerPeriodPulse1 = timerPeriodTargetPulse1;
    }

    if(pulse1SweepDivider == 0 || pulse1SweepReload) {
        pulse1SweepDivider = pulse1Reg1.sweepPeriod;
        pulse1SweepReload = false;
    }
    else
        --pulse1SweepDivider;
    if(timerPeriodTargetPulse1 > 0x7FF || timerPeriodPulse1 < 8)
        pulse1SweepMute = true;
    else
        pulse1SweepMute = false;
    
    //Pulse 2
    if(pulse2Reg1.sweepEnable == 1 && pulse2SweepDivider == 0 && pulse2SweepMute == 0) {
        periodDelta = timerPeriodTargetPulse2 >> pulse2Reg1.sweepShiftCount;
        if(pulse2Reg1.sweepNegative == 1) timerPeriodTargetPulse2 -= periodDelta - 1;
        else timerPeriodTargetPulse2 += periodDelta;
        
        timerPeriodPulse2 = timerPeriodTargetPulse2;
    }

    if(pulse2SweepDivider == 0 || pulse2SweepReload) {
        pulse2SweepDivider = pulse2Reg1.sweepPeriod;
        pulse2SweepReload = false;
    }
    else
        --pulse2SweepDivider;
    if(timerPeriodTargetPulse2 > 0x7FF || timerPeriodPulse2 < 8)
        pulse2SweepMute = true;
    else
        pulse2SweepMute = false;
}

void clockLinearCounter() {
    if(triangle_linearCntrReload)
        triangle_linearCntr = triReg0.linCtrReloadVal;
    else if(triangle_linearCntr > 0)
        --triangle_linearCntr;
    if(triReg0.lenCtrDisable == 0)
        triangle_linearCntrReload = false;
}

uint8_t regGet(uint16_t addr)
{   
    if(addr == 0x4015) {
        statusReg.LCpulse1 = ((pulse1_lenCntr>0) ? 1 : 0);
        statusReg.LCpulse2 = ((pulse2_lenCntr>0) ? 1 : 0);
        statusReg.LCtriangle = ((triangle_lenCntr>0) ? 1 : 0);
        statusReg.LCnoise = ((noise_lenCntr>0) ? 1 : 0);
        statusReg.DMCactive = ((dmcBytesRemaining > 0) ? 1 : 0);
        statusReg.frameIRQ = ((frameInterruptRequest) ? 1 : 0);
        statusReg.dmcIRQ = ((DMCinterruptRequest) ? 1 : 0);
        statusReg.openBus = CPU::busVal >> 5;
        frameInterruptRequest = false;
        return statusReg.value;
    }
    return CPU::busVal; //Open bus behavior
}

void regSet(uint16_t addr, uint8_t val)
{
    switch(addr) {
        case 0x4000:
            pulse1Reg0.value = val;
            dutyCyclePulse1 = pulseDutyCycleTable[pulse1Reg0.dutyCycleSel];
            if(pulse1Reg0.constVol) pulse1Volume = pulse1Reg0.volPeriod;
            else pulse1Volume = pulse1EnvDecay;
            break;
        case 0x4001:
            pulse1Reg1.value = val;
            break;
        case 0x4002:
            timerPeriodPulse1 = (timerPeriodPulse1 & 0x700) | val;
            break;
        case 0x4003:
            pulse1Reg3.value = val;
            timerPeriodPulse1 = (((uint16_t)pulse1Reg3.timerHigh) << 8) | (timerPeriodPulse1 & 0xFF);
            if(controlReg.enableLCpulse1) pulse1_lenCntr = lengthCounterArray[pulse1Reg3.lenCtrLoad];
            timerPulse1 = timerPeriodPulse1;
            timerPeriodTargetPulse1 = timerPeriodPulse1;
            dutyIdxPulse1 = 0;
            pulse1StartEnv = true;
            break;
        case 0x4004:
            pulse2Reg0.value = val;
            dutyCyclePulse2 = pulseDutyCycleTable[pulse2Reg0.dutyCycleSel];
            if(pulse2Reg0.constVol) pulse2Volume = pulse2Reg0.volPeriod;
            else pulse2Volume = pulse2EnvDecay;
            break;
        case 0x4005:
            pulse2Reg1.value = val;
            break;
        case 0x4006:
            timerPeriodPulse2 = (timerPeriodPulse2 & 0x700) | val;
            break;
        case 0x4007:
            pulse2Reg3.value = val;
            timerPeriodPulse2 = (((uint16_t)pulse2Reg3.timerHigh) << 8) | (timerPeriodPulse2 & 0xFF);
            if(controlReg.enableLCpulse2) pulse2_lenCntr = lengthCounterArray[pulse2Reg3.lenCtrLoad];
            timerPulse2 = timerPeriodPulse2;
            timerPeriodTargetPulse2 = timerPeriodPulse2;
            dutyIdxPulse2 = 0;
            pulse2StartEnv = true;
            break;
        case 0x4008:
            triReg0.value = val;
            break;
        case 0x400A:
            timerSetTriangle = (timerSetTriangle & 0x700) | val;
            break;
        case 0x400B:
            triReg2.value = val;
            timerSetTriangle = (((uint16_t)triReg2.timerHigh) << 8) | (timerSetTriangle & 0xFF);
            if(controlReg.enableLCtriangle) triangle_lenCntr = lengthCounterArray[triReg2.lenCtrLoad];
            timerTriangle = timerSetTriangle;
            triangle_linearCntrReload = true;
            break;
        case 0x400C:
            noiseReg0.value = val;
            if(noiseReg0.constVol) noiseVolume = noiseReg0.volPeriod;
            else noiseVolume = noiseEnvDecay;
            break;
        case 0x400E:
            noiseReg1.value = val;
            break;
        case 0x400F:
            noiseReg2.value = val;
            noiseStartEnv = true;
            if(controlReg.enableLCnoise) noise_lenCntr = lengthCounterArray[noiseReg2.lenCtrLoad];
            break;
        case 0x4010:
            dmcReg0.value = val;
            if(dmcReg0.IRQenable == 0) DMCinterruptRequest = false;
            break;
        case 0x4011:
            dmcReg1.value = val;
            outputDMC = dmcReg1.directLoad;
            break;
        case 0x4012:
            dmcTargetAddr = 0xC000 + (val * 64);
            break;
        case 0x4013:
            dmcTargetLen = (val * 16) + 1;
            break;
        case 0x4015:
            controlReg.value = val;
            if(controlReg.enableLCpulse1 == 0) pulse1_lenCntr = 0;
            if(controlReg.enableLCpulse2 == 0) pulse2_lenCntr = 0;
            if(controlReg.enableLCtriangle == 0) triangle_lenCntr = 0;
            if(controlReg.enableLCnoise == 0) noise_lenCntr = 0;
            if(controlReg.enableDMC == 0) dmcBytesRemaining = 0;
            else if(dmcBytesRemaining == 0) {
                dmcBytesRemaining = dmcTargetLen;
                dmcCurrAddr = dmcTargetAddr;
                loadDMC();
            }
            DMCinterruptRequest = false;
            break;
        case 0x4017:
            frameReg.value = val;
            if(frameReg.IRQinhibit) frameInterruptRequest = false;
            frameReset = true;
            if(frameReg.frameMode) { //Clock immediately
                clockEnvelopes();
                clockLinearCounter();
                clockLengthCounters();
                clockSweep();
            }
            break;
    }
}

void stepPulse1() {
    if(timerPulse1 > 0) {
        --timerPulse1;
    }
    else {
        timerPulse1 = timerPeriodPulse1;
        dutyIdxPulse1 = (dutyIdxPulse1 + 1) % 8;
    }

    if(pulse1_lenCntr > 0 && pulse1SweepMute == 0) {
        outputPulse1 = dutyCyclePulse1[dutyIdxPulse1] * pulse1Volume;
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
        timerPulse2 = timerPeriodPulse2;
        dutyIdxPulse2 = (dutyIdxPulse2 + 1) % 8;
    }

    if(pulse2_lenCntr > 0 && pulse2SweepMute == 0) {
        outputPulse2 = dutyCyclePulse2[dutyIdxPulse2] * pulse2Reg0.volPeriod;
    }
    else {
        outputPulse2 = 0;
    }
}

void stepTriangle() {
    if(timerTriangle > 0) {
        --timerTriangle;
    }
    else {
        timerTriangle = timerSetTriangle;
        if(triangle_lenCntr > 0 && triangle_linearCntr > 0) {
            ++triangleOutputArrayIdx;
            if(triangleOutputArrayIdx >= 32) triangleOutputArrayIdx = 0;
        }
    }
    outputTriangle = triangleOutputArray[triangleOutputArrayIdx];
}

void stepNoise() {
    uint16_t feedback;
    if(timerNoise > 0) {
        --timerNoise;
    }
    else {
        timerNoise = noiseTimerTable[noiseReg1.noisePeriodSel];
        //Calculate pseudorandom number
        if(noiseReg1.noiseMode == 0)
            feedback = (noiseShiftRegister & 1) ^ ((noiseShiftRegister >> 1) & 1);
        else
            feedback = (noiseShiftRegister & 1) ^ ((noiseShiftRegister >> 6) & 1);
        noiseShiftRegister >>= 1;
        noiseShiftRegister = (noiseShiftRegister & 0x3FFF) | (feedback << 14);
    }

    if(((noiseShiftRegister & 1) == 0) && noise_lenCntr > 0)
        outputNoise = noiseVolume;
    else
        outputNoise = 0;
}

void stepDMC() {
    if(timerDMC > 0) {
        --timerDMC;
    }
    else {
        timerDMC = dmcRateTable[dmcReg0.freqIdx];
        if(dmcSilence == 0) {
            if((dmcShiftRegister & 1) == 0) {
                if(outputDMC >= 2) outputDMC -= 2;
            }
            else {
                if(outputDMC <= 125) outputDMC += 2;
            }
        }
        //Always shift register
        dmcShiftRegister >>= 1;
        if(dmcBitsRemaining > 0) --dmcBitsRemaining;
    }
    if(dmcBitsRemaining == 0) {
        dmcBitsRemaining = 8;
        if(dmcBuffer == 0) {
            dmcSilence = true;
        }
        else {
            dmcSilence = false;
            dmcShiftRegister = dmcBuffer;
            dmcBuffer = 0;
            loadDMC();
        }
    }
}

void loadDMC() {
    if(dmcBuffer != 0) return;
    //TODO: stall CPU
    dmcBuffer = CPU::memGet(dmcCurrAddr);
    if(dmcCurrAddr == 0xFFFF) dmcCurrAddr = 0x8000;
    else ++dmcCurrAddr;
    --dmcBytesRemaining;

    if(dmcBytesRemaining == 0) {
        if(dmcReg0.loopSample) {
            dmcCurrAddr = dmcTargetAddr;
            dmcBytesRemaining = dmcTargetLen;
        }
        else if(dmcReg0.IRQenable) {
            DMCinterruptRequest = true;
        }
    }
}


void mixOutput() {
    //Output is 0-1
    float output;
    output = pulseMixerTable[outputPulse1 + outputPulse2];
    output += tndMixerTable[3 * outputTriangle + 2 * outputNoise + outputDMC];
    NES::rawAudio.buffer[NES::rawAudio.writeIdx] = output;
    NES::rawAudio.writeIdx = (NES::rawAudio.writeIdx+1) % NES::rawAudio.buffer.size();
}

void generateMixerTables() {
    //Using info from http://wiki.nesdev.com/w/index.php/APU_Mixer
    //Generates lookup tables to speed up processing time
    pulseMixerTable[0] = 0;
    tndMixerTable[0] = 0;
    for(unsigned int i=1; i<pulseMixerTable.size(); ++i) {
        pulseMixerTable[i] = (95.52f / (8128.0f / i + 100.0f));
    }
    for(unsigned int i=1; i<tndMixerTable.size(); ++i) {
        tndMixerTable[i] = (163.67f / (24329.0f / i + 100.0f));
    }
}


}