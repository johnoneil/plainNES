#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/chrono.hpp>

#include "gui.h"
#include "cpu.h"
#include "ppu.h"
#include "io.h"
#include "apu.h"
#include "gamepak.h"


int main(int argc, char *argv[])
{
	bool debug_PC_start_flag = false;
	uint16_t debug_PC_start;
	bool loggingFlag = false;
	bool fpsFlag = false;
	bool disableAudioFlag = false;
	bool debugPPUflag = false;

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
		//po::store(po::parse_command_line(argc, argv, desc), vm);
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

	std::ifstream file;
	if(vm.count("file")) {
		file.open(vm["file"].as<std::string>(),std::ios::binary);
		if(file.fail()) {
			std::cout << "Error opening file" << std::endl;
			return -1;
		}
	}
	else {
		std::cout << "Rom filename must be included as an argument" << std::endl;
		return -1;
	}

	if(vm.count("PC")) {
		debug_PC_start_flag = true;
		debug_PC_start = vm["PC"].as<uint16_t>();
	}

	if(vm.count("log")) loggingFlag = true;
	if(vm.count("debugPPU")) debugPPUflag = true;
	if(vm.count("FPS")) fpsFlag = true;
	if(vm.count("disableAudio")) disableAudioFlag = true;
	
	if(GAMEPAK::init(file) < 0) {
		std::cout << "Invalid file\n" << std::endl;
		return -1;
	}
	file.close();
	
	GUI::init(fpsFlag, disableAudioFlag, debugPPUflag);
	CPU::init(loggingFlag);
	PPU::init();
	APU::init();

	if(debug_PC_start_flag) {
		CPU::setPC(debug_PC_start);
	}

	int framenum = 0;
	const int avgWindow = 30;
	boost::chrono::high_resolution_clock::time_point frameStart, frameEnd;
	boost::chrono::nanoseconds avgFrameDuration_ns;

	frameStart = boost::chrono::high_resolution_clock::now();
	try {
		while(GUI::quit == 0 && CPU::alive)
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
			PPU::setframeReady(false);
			while(PPU::isframeReady() == 0) {
				CPU::step();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Standard exception: " << e.what() << std::endl;
	}
	catch (int e) {
		if(e == 1) {
			std::cout << "Ending program due to invalid opcode" << std::endl;
		}
		else {
			std::cout << "Ending program due to unexpected error " << e << std::endl;
		}
	}
		
	GUI::close();
	return 0;
}
