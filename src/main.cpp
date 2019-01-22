#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <exception>
#include <boost/thread/thread.hpp>
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
	bool loggingFlag = false;
	uint16_t debug_PC_start;

	if(argc < 2) {
		std::cout << "Include filename as argument" << std::endl;
		return -1;
	}
	std::ifstream file;
	file.open(argv[1],std::ios::binary);
	if(file.fail()) {
		std::cout << "Error opening file" << std::endl;
		return -1;
	}

	for(int i=2; i < argc-1; ++i) {
		if(std::string(argv[i]) == "-PC") {
			debug_PC_start_flag = true;
			debug_PC_start = strtoul(argv[i+1], NULL, 16);
		}
	}

	for(int i=2; i<argc; ++i) {
		if(std::string(argv[i]) == "-log") {
			loggingFlag = true;
		}
	}
	
	if(GAMEPAK::init(file) < 0) {
		std::cout << "Invalid file\n" << std::endl;
		return -1;
	}
	file.close();
	
	GUI::init();
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
