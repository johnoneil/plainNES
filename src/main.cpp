#include "emulator.h"
#include <iostream>
#include <string>
#include "cxxopts.hpp"

int main(int argc, char *argv[])
{
	cxxopts::Options options("plainNES", "NES Emulator");
	options.add_options()
		("f,file", "File name", cxxopts::value<std::string>())
		("PC", "Start program at specific memory address", cxxopts::value<uint16_t>())
		("log", "enable logging", cxxopts::value<bool>()->default_value("false"))
		("disableAudio", "Disables audio, unthrottling emulator", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Print usage")
		;

	options.parse_positional({"file"});

	auto vm = options.parse(argc, argv);

	//Parse command line options
	EMULATOR::StartOptions startOptions;

	if(vm.count("file")) startOptions.filename = vm["file"].as<std::string>();
	if(vm.count("PC")) {
		startOptions.startAtPC = true;
		startOptions.debugPC = vm["PC"].as<uint16_t>();
	}
	if(vm.count("log")) startOptions.log = true;
	if(vm.count("disableAudio")) startOptions.disableAudio = true;

	//Start program
	return EMULATOR::start(startOptions);
}
