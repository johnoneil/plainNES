#pragma once

#include <stdint.h>

namespace GUI {

extern bool quit;

int init();
int initMainWindow();
int initPPUWindow();
void update();
void updateMainWindow();
void updatePPUWindow();
void close();

}
