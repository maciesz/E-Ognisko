#ifndef MIXER_H
#define MIXER_H

#include <boost/cstdint.hpp>
/*#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>*/

#include "../common/structures.hpp"

class Mixer
{
public:
	void mixer(mixer_input* inputs, size_t n, void* output_buf, 
		size_t* output_size, unsigned long tx_interval_ms) const;
private:
	size_t get_total_bytes(const mixer_input* inputs, const int size) const;

	void init_consumed_bytes(mixer_input* inputs, const int size) const;

	const unsigned long MULTIPLIER = 176;
};

#endif
