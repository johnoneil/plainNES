#include "emulator.h"
#include "nes.h"
#include "gui.h"

namespace EMULATOR {

int start(StartOptions startOptions)
{
    if(startOptions.disableAudio) GUI::onEmuSpeedMax();
    if(startOptions.startAtPC) NES::setDebugPC(true, startOptions.debugPC);
    if(startOptions.log) NES::enableLogging();

	GUI::init();

    if(startOptions.filename != "") {
        if(NES::loadROM(startOptions.filename) == 0)
	        NES::powerOn();
    }
	
    while(GUI::quit == 0)
	{
		GUI::update();
		NES::frameStep();
	}

    return 0;
}

} //EMULATOR