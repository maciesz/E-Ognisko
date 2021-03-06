#ifndef MIXER_HH
#define MIXER_HH

#include <cstdint>
#include <climits>
#include <algorithm>
#include <set>

#include "../common/structures.hh"

class mixer
{
public:
	static void mix(mixer_input* inputs, size_t n, void* output_buf, 
		size_t* output_size, unsigned long tx_interval_ms);
private:
	static void init_consumed_bytes(mixer_input* inputs, const int size);

	static const unsigned long MULTIPLIER = 176;
};

#endif
