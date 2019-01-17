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

	float targetFPS = 60;
	//float avgFPS = targetFPS;
	//float cmdFPS = targetFPS;
	const int avgWindow = 10;
	float FPSAdj = 0.1;
	
	boost::chrono::nanoseconds targetFrameDuration_ns((long long)(1000000000.0f / (targetFPS)));
	boost::chrono::nanoseconds avgFrameDuration_ns = targetFrameDuration_ns;
	boost::chrono::nanoseconds commandedFrameDuration_ns = targetFrameDuration_ns;
	boost::chrono::nanoseconds durationAdj((long long)(1000000000.0f/targetFPS - (1000000000.0f/(targetFPS+FPSAdj))));
	boost::chrono::high_resolution_clock::time_point frame_start, frame_end;
	
	int frames = 0;
	boost::chrono::high_resolution_clock::time_point lastUpdate = boost::chrono::high_resolution_clock::now();
	try {
		while(GUI::quit == 0 && CPU::alive)
		{
			frame_start = boost::chrono::high_resolution_clock::now();
			++frames;
			if(frames >= avgWindow) {
				avgFrameDuration_ns = (boost::chrono::high_resolution_clock::now() - lastUpdate)/avgWindow;
				lastUpdate = boost::chrono::high_resolution_clock::now();
				frames = 0;
				//avgFPS = 1000000000.0f/(avgFrameDuration_ns.count());
				//std::cout << avgFPS << std::endl;
				//cmdFPS = 1000000000.0f/(commandedFrameDuration_ns.count());
			
				if(avgFrameDuration_ns > (targetFrameDuration_ns)) {
					commandedFrameDuration_ns -= durationAdj;
				}
				else if(avgFrameDuration_ns < (targetFrameDuration_ns)) {
					commandedFrameDuration_ns += durationAdj;
				}
			}

			GUI::update();
			PPU::setframeReady(false);
			while(PPU::isframeReady() == 0) {
				CPU::step();
				if(APU::audioBufferReady) GUI::updateAudio();
			}
			
			frame_end = boost::chrono::high_resolution_clock::now();
			boost::this_thread::sleep_until(frame_start + commandedFrameDuration_ns);
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
