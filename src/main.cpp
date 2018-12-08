#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
//#include <SDL2/SDL.h>

#include "cpu.h"
//#include "ppu.h"
//#include "io.h"
#include "gamepak.h"



int main(int argc, char *argv[])
{
	bool debug_PC_start_flag = false;
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
	
	if(GAMEPAK::loadROM(file) < 0) {
		std::cout << "Invalid file\n" << std::endl;
		return -1;
	}
	file.close();
	
	CPU::init();
	if(debug_PC_start_flag)
		CPU::setPC(debug_PC_start);

	std::cout << "Starting..." << std::endl;

	std::cout << std::hex << std::uppercase;
	try {
		for(int i=0; i<10000; ++i) {
			CPU::step();
		}
	}
	catch (int e) {
		if(e == 1) {
			std::cout << "Ending program due to invalid opcode" << std::endl;
		}
		else {
			std::cout << "Ending program due to unexpected error " << e << std::endl;
		}
	}
		
	return 0;
}
