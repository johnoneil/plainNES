#include "utils.h"

#include <string>
#include <iomanip>
#include <sstream>

std::string int_to_hex(uint8_t i)
{
	std::stringstream stream;
	stream << std::setfill ('0') << std::hex << std::uppercase << std::setw(2) 
				 << std::hex << static_cast<int>(i);
	return stream.str();
}

std::string int_to_hex(uint16_t i)
{
	std::stringstream stream;
	stream << std::setfill ('0') << std::hex << std::uppercase << std::setw(4) 
				 << std::hex << static_cast<int>(i);
	return stream.str();
}