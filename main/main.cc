#include <vector>

#include <stdio.h>
#include <stdint.h>

#include "mixer.h"
#include "structures.h"

void function(void* data)
{
	std::vector<int>* vec = static_cast<std::vector<int>*>(data);
	const int size = vec->size();
	int i = 0, k;
	while (i < size) {
		printf("%d\n", (*vec)[i]);
		//*((int*)data) = 1;
		++i;
	}
}

int main()
{
	std::vector<int> vec;
	vec.push_back(1);
	vec.push_back(4);
	vec.push_back(6);
	vec.push_back(1009);

	for (int i = 0; i< vec.size(); ++i)
		printf("%d, ", vec.size());

	function(static_cast<void*>(&vec));
	function(static_cast<void*>(&vec));

	Mixer mixer;
	std::vector<int16_t> output_buffer;
	output_buffer.push_back(1);
	output_buffer.push_back(3);

		//printf("%hd\n", output_buffer.size());
		//output_buffer.push_back((int16_t)0);

	size_t size_another = 4;
	std::vector<int16_t> packages[size_another];

	packages[0].push_back(2);
	packages[0].push_back(1929);
	packages[1].push_back(3);
	packages[1].push_back(100);
	packages[2].push_back(0);
	packages[2].push_back(4);
	packages[3].push_back(5);
	packages[3].push_back(20);

	mixer_input inputs[size_another];
	size_t* buffer_size;
	*buffer_size = 4;
	for (int i = 0; i< size_another; ++i) {
		inputs[i].len = 4;
		inputs[i].data = static_cast<void*>(&packages[i]);
	}

	mixer.mixer(&inputs[0], size_another, static_cast<void*>(&output_buffer), buffer_size, 5);
	
	//printf("%d\n", output_buffer.size());
	for (int i = 0; i< output_buffer.size(); ++i)
		printf("%hd ", output_buffer[i]);

	std::vector<int> wektor;

	for (int i = 0; i< output_buffer.size(); ++i) {
		printf("%d\n", output_buffer.size());
	}
	return 0;
}
