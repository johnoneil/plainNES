#pragma once

#include <string>

std::string int_to_hex(uint8_t i);
std::string int_to_hex(uint16_t i);

//Bitworker
template <unsigned int bit_n, unsigned int nbits=1, typename T=uint8_t>
struct BitWorker {
	T value;
	enum { mask = (1u << nbits) - 1u }; //Using "const T mask" doesn't work;
		
	template<typename T2>
	BitWorker& operator=(T2 newval)
	{
		value = (value & ~(mask << bit_n)) | ((newval & mask) << bit_n);
		return *this;
	}
    BitWorker& operator=(BitWorker bw)
    {
        value = (value & ~(mask << bit_n)) | ((bw & mask) << bit_n);
        return *this;
    }
	operator T() const { return (value >> bit_n) & mask; }
	BitWorker& operator++() { return *this = *this + 1; }
	T operator++(int) { T v = *this; ++*this; return v; }
};