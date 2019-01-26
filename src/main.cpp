#include "nes.h"
#include "gui.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/chrono.hpp>

int main(int argc, char *argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Command line option help")
		("file", "ROM file to open")
		("PC", po::value<uint16_t>(), "Start program at specific memory address")
		("FPS", "show FPS counter")
		("log", "enable logging")
		("debugPPU", "Show PPU debug window")
		("disableAudio", "Disables audio, unthrottling emulator");
	po::variables_map vm;

	po::positional_options_description p;
	p.add("file", 1);

	try
	{
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);

		if(vm.count("help")) {
			std::cout << "plainNES - A simple NES emulator\n";
			std::cout << desc << std::endl;
			return 0;
		}
	}
	catch(po::error& e)
	{
		std::cerr << "ERROR: " << e.what() << "\n" << std::endl;
		std::cerr << desc << std::endl;
		return 1;
	}

	int guiOptions = 0;
	int emuOptions = 0;

	if(vm.count("file") == 0) {
		std::cout << "ROM filename must be included as an argument" << std::endl;
		return 1;
	}

	if(vm.count("PC")) {
		emuOptions |= NES::SET_PC_START;
		NES::PC_debug_start = vm["PC"].as<uint16_t>();
	}

	if(vm.count("log")) emuOptions |= NES::LOGGING;
	if(vm.count("debugPPU")) guiOptions |= GUI::PPU_DEBUG;
	if(vm.count("FPS")) guiOptions |= GUI::DISPLAY_FPS;
	if(vm.count("disableAudio")) guiOptions |= GUI::DISABLE_AUDIO;

	NES::setOptions(emuOptions);
	GUI::setOptions(guiOptions);

	GUI::init();
	NES::loadROM(vm["file"].as<std::string>());
	NES::powerOn();

	int framenum = 0;
	const int avgWindow = 30;
	boost::chrono::high_resolution_clock::time_point frameStart, frameEnd;
	boost::chrono::nanoseconds avgFrameDuration_ns;

	frameStart = boost::chrono::high_resolution_clock::now();

	try{
	while(GUI::quit == 0 && NES::running)
	{
		if(framenum >= avgWindow) {
			frameEnd = boost::chrono::high_resolution_clock::now();
			avgFrameDuration_ns = (frameEnd - frameStart)/avgWindow;
			frameStart = frameEnd;
			GUI::avgFPS = 1000000000.0f/(avgFrameDuration_ns.count());
			framenum = 0;
		}
		++framenum;

		GUI::update();
		NES::frameStep();
	}
	}
	catch(const std::out_of_range& oor)
	{
		std::cerr << "Out of Range error: " << oor.what() << std::endl;
	}
	catch(...)
    {
        std::exception_ptr p = std::current_exception();
        std::cout <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
    }
		
	GUI::close();
	return 0;
}
