#ifndef MIXER_HPP
#define MIXER_HPP

#include <boost/cstdint.hpp>

#include "../common/structures.hpp"

class mixer
{
public:
	static void mix(mixer_input* inputs, size_t n, void* output_buf, 
		size_t* output_size, unsigned long tx_interval_ms);
private:
	static size_t get_total_bytes(const mixer_input* inputs, const int size);

	static void init_consumed_bytes(mixer_input* inputs, const int size);

	static const unsigned long MULTIPLIER = 176;
};

#endif
