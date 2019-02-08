#pragma once

#include <string>

namespace EMULATOR {

struct StartOptions {
    std::string filename = "";
    bool startAtPC = false;
    uint16_t debugPC;
    bool log = false;
    bool disableAudio = false;
};


int start(StartOptions startOptions);


} //EMULATOR