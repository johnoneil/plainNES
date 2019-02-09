#include "emulator.h"
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

int main(int argc, char *argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Command line option help")
		("file", "ROM file to open")
		("PC", po::value<uint16_t>(), "Start program at specific memory address")
		("log", "enable logging")
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
