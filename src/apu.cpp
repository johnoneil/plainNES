#include "apu.h"
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
        BitWorker<5, 1> disableLenCtr;
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

uint16_t dmcSampleAddr;
uint16_t dmcSampleLen;

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
bool pulse1RestartEnv = false;
uint8_t pulse1EnvDivider = 0;
uint8_t pulse1SweepDivider = 0;
bool pulse1SweepMute = false;
bool pulse1SweepReload = false;
uint8_t pulse1_lenCntr;
uint16_t timerSetPulse1;
uint16_t timerPulse1;
uint8_t outputPulse1;
int dutyIdxPulse1;
std::array<int,8> dutyCyclePulse1;

//Pulse 2
uint8_t pulse2Volume = 0;
uint8_t pulse2EnvDecay = 0;
bool pulse2RestartEnv = false;
uint8_t pulse2EnvDivider = 0;
uint8_t pulse2SweepDivider = 0;
bool pulse2SweepMute = false;
bool pulse2SweepReload = false;
uint8_t pulse2_lenCntr;
uint16_t timerSetPulse2;
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
bool noiseRestartEnv = false;
uint8_t noiseEnvDivider = 0;
uint8_t noise_lenCntr;
uint16_t noiseShiftRegister;
uint16_t timerNoise;
uint8_t outputNoise;
std::array<uint16_t, 16> noiseTimerTable {0x04, 0x08, 0x10, 0x20, 0x40, 0x60, 0x80, 0xA0,
                                          0xCA, 0xFE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4};

//DMC
bool DMCinterruptRequest = false;
uint8_t outputDMC;

//Frame Counter
bool frameInterruptRequest = false;



unsigned int frameHalfCycle;
unsigned long long cycle = 0;

bool audioBufferReady = false;
int outputBufferIdx = 0;
int outputBufferSel = 0;
uint16_t* outputBufferPtr;
std::array<std::array<uint16_t, OUTPUT_AUDIO_BUFFER_SIZE>,2> outputBuffers;

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
    noiseShiftRegister = 1;
}

void step()
{
    switch(frameHalfCycle) {
        case 7457: //3728.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            break;
        case 14913: //7456.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            clockLengthCounters();
            clockSweep();
            break;
        case 22371: //11185.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            break;
        case 29828: //14914 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
            }
            break;
        case 29829: //14914.5 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
                clockEnvelopes();
                clockLinearCounter();
                clockLengthCounters();
                clockSweep();
            }
            break;
        case 29830: //14915 full cycles
            if(frameReg.frameMode == 0) {
                if(frameReg.IRQinhibit == 0) frameInterruptRequest = true;
                frameHalfCycle = 0;
            }
            break;
        case 37281: //18640.5 full cycles
            clockEnvelopes();
            clockLinearCounter();
            clockLengthCounters();
            clockSweep();
            break;
        case 37282: //18641 full cycles
            frameHalfCycle = 0;
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

    if(cycle % (int)(1789772.7272 / OUTPUT_AUDIO_FREQ) == 0) { //Downsample
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
        else if(noiseReg0.disableLenCtr == 0)
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
    if(pulse1RestartEnv) {
        pulse1RestartEnv = false;
        pulse1EnvDecay = 15;
        pulse1EnvDivider = pulse1Reg0.volPeriod + 1;
    }
    else {
        if(pulse1EnvDivider == 0) {
            pulse1EnvDivider = pulse1Reg0.volPeriod + 1;
            if(pulse1EnvDecay == 0 && pulse1Reg0.loopDisableLC) {
                pulse1EnvDecay = 15;
            }
            else if(pulse1EnvDecay > 0) {
                --pulse1EnvDecay;
            }
        }
        else {
            --pulse1EnvDivider;
        }
    }
    if(pulse1Reg0.constVol == 0) pulse1Volume = pulse1EnvDecay;

    //Pulse 2
    if(pulse2RestartEnv) {
        pulse2RestartEnv = false;
        pulse2EnvDecay = 15;
        pulse2EnvDivider = pulse2Reg0.volPeriod + 1;
    }
    else {
        if(pulse2EnvDivider == 0) {
            pulse2EnvDivider = pulse2Reg0.volPeriod + 1;
            if(pulse2EnvDecay == 0 && pulse2Reg0.loopDisableLC) {
                pulse2EnvDecay = 15;
            }
            else if(pulse2EnvDecay > 0) {
                --pulse2EnvDecay;
            }
        }
        else {
            --pulse2EnvDivider;
        }
    }
    if(pulse2Reg0.constVol == 0) pulse2Volume = pulse2EnvDecay;

    //Noise
    if(noiseRestartEnv) {
        noiseRestartEnv = false;
        noiseEnvDecay = 15;
        noiseEnvDivider = noiseReg0.volPeriod + 1;
    }
    else {
        if(noiseEnvDivider == 0) {
            noiseEnvDivider = noiseReg0.volPeriod + 1;
            if(noiseEnvDecay == 0 && noiseReg0.disableLenCtr) {
                noiseEnvDecay = 15;
            }
            else if(noiseEnvDecay > 0) {
                --noiseEnvDecay;
            }
        }
        else {
            --noiseEnvDivider;
        }
    }
    if(noiseReg0.constVol == 0) noiseVolume = noiseEnvDecay;
}

void clockSweep() {
    int16_t periodDelta;
    //Pulse 1
    if(pulse1SweepDivider == 0) {
        if(pulse1Reg1.sweepEnable && pulse1SweepMute == 0) {
            //Modify the period
            periodDelta = timerSetPulse1 >> pulse1Reg1.sweepShiftCount;
            if(pulse1Reg1.sweepNegative) periodDelta = -periodDelta - 1;
            timerSetPulse1 = timerPulse1 + periodDelta;
            timerSetPulse1 &= 0x7FF;
        }
    }
    if(pulse1SweepDivider == 0 || pulse1SweepReload) {
        pulse1SweepDivider = pulse1Reg1.sweepPeriod;
        pulse1SweepReload = false;
    }
    else {
        --pulse1SweepDivider;
    }
    //Check if channel should be muted
    if(timerPulse1 < 8)
        pulse1SweepMute = true;
    else if(pulse1Reg1.sweepNegative == 0 && (timerSetPulse1 + (timerSetPulse1 >> pulse1Reg1.sweepShiftCount) > 0x7FF))
        pulse1SweepMute = true;
    else
        pulse1SweepMute = false;
    
    //Pulse 2
    if(pulse2SweepDivider == 0) {
        if(pulse2Reg1.sweepEnable && pulse2SweepMute == 0) {
            //Modify the period
            periodDelta = timerSetPulse2 >> pulse2Reg1.sweepShiftCount;
            if(pulse2Reg1.sweepNegative) periodDelta = -periodDelta;
            timerSetPulse2 = timerPulse2 + periodDelta;
            timerSetPulse2 &= 0x7FF;
        }
    }
    if(pulse2SweepDivider == 0 || pulse2SweepReload) {
        pulse2SweepDivider = pulse2Reg1.sweepPeriod;
        pulse2SweepReload = false;
    }
    else {
        --pulse2SweepDivider;
    }
    //Check if channel should be muted
    if(timerPulse2 < 8)
        pulse2SweepMute = true;
    else if(pulse2Reg1.sweepNegative == 0 && (timerSetPulse2 + (timerSetPulse2 >> pulse2Reg1.sweepShiftCount) > 0x7FF))
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
        statusReg.LCpulse1 = ((pulse1_lenCntr>0) ? 1: 0);
        statusReg.LCpulse2 = ((pulse2_lenCntr>0) ? 1: 0);
        statusReg.LCtriangle = ((triangle_lenCntr>0) ? 1: 0);
        statusReg.LCnoise = ((noise_lenCntr>0) ? 1: 0);
        statusReg.DMCactive = 0; //TODO: check DMC bytes remaining
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
            timerSetPulse1 = (timerSetPulse1 & 0x700) | val;
            break;
        case 0x4003:
            pulse1Reg3.value = val;
            timerSetPulse1 = (((uint16_t)pulse1Reg3.timerHigh) << 8) | (timerSetPulse1 & 0xFF);
            if(controlReg.enableLCpulse1) pulse1_lenCntr = lengthCounterArray[pulse1Reg3.lenCtrLoad];
            timerPulse1 = timerSetPulse1;
            dutyIdxPulse1 = 0;
            pulse1RestartEnv = true;
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
            timerSetPulse2 = (timerSetPulse2 & 0x700) | val;
            break;
        case 0x4007:
            pulse2Reg3.value = val;
            timerSetPulse2 = (((uint16_t)pulse2Reg3.timerHigh) << 8) | (timerSetPulse2 & 0xFF);
            if(controlReg.enableLCpulse2) pulse2_lenCntr = lengthCounterArray[pulse2Reg3.lenCtrLoad];
            timerPulse2 = timerSetPulse2;
            dutyIdxPulse2 = 0;
            pulse2RestartEnv = true;
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
            break;
        case 0x400E:
            noiseReg1.value = val;
            break;
        case 0x400F:
            noiseReg2.value = val;
            noiseRestartEnv = true;
            break;
        case 0x4015:
            controlReg.value = val;
            if(controlReg.enableLCpulse1 == 0) pulse1_lenCntr = 0;
            if(controlReg.enableLCpulse2 == 0) pulse2_lenCntr = 0;
            if(controlReg.enableLCtriangle == 0) triangle_lenCntr = 0;
            if(controlReg.enableLCnoise == 0) noise_lenCntr = 0;
            //TODO: Set DMC disable/enable
            break;
        case 0x4017:
            frameReg.value = val;
            if(frameReg.IRQinhibit) frameInterruptRequest = false;
            frameHalfCycle = 0;
            if(frameReg.frameMode) { //Clock immediately
                //inc envelopes and trianble linear counter
                clockLengthCounters();
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
        timerPulse2 = timerSetPulse2;
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
        noiseShiftRegister = (noiseShiftRegister & !0x4000) | (feedback << 14);
    }
    if(((noiseShiftRegister & 1) == 0) && noise_lenCntr > 0)
        outputNoise = noiseVolume;
    else
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