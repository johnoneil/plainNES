#pragma once

#include <stdint.h>

namespace GUI {

extern bool quit;

int init();
int initMainWindow();
int initPPUWindow();
int initAudio();
void update();
void updateMainWindow();
void updatePPUWindow();
void updateAudio();
void close();

}
