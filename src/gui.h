#pragma once

#include <stdint.h>

namespace GUI {

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCREEN_SCALE = 2;

const int AUDIO_SAMPLE_RATE = 48000;
const int AUDIO_BUFFER_SIZE = 1024;
const int AUDIO_CHANNELS = 1;
const int WANTED_AUDIO_LATENCY_MS = 200;

extern float avgFPS;

extern bool quit;

int init(bool fpsFlag = false, bool disableAudioFlag = false, bool debugPPUflag = false);
int initMainWindow();
int initPPUWindow();
int initAudio();
void update();
void updateMainWindow();
void updatePPUWindow();
void updateAudio();
void fill_audio_buffer(void *user_data, uint8_t *out, int byte_count);
void close();
void downsample(int16_t *output, float *input, int &size);

}
