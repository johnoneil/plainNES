#pragma once

#include <stdint.h>

namespace GUI {

extern bool quit;

int init();
void update(uint8_t *screenBuffer, uint8_t *controllerPtr);
void close();

}
