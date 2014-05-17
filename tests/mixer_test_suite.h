#ifndef MIXER_TEST_SUIT_H
#define MIXER_TEST_SUIT_H

#include <vector>

// Struktura mixer_input
#include "structures.h"

class MixerTestSuit: public Test::Suite
{
public:
	MixerTestSuit()
	{
		TEST_ADD(MixerTestSuit::single_mixer_call_test);
		TEST_ADD(MixerTestSuit::extreme_short_value_test);
		TEST_ADD(MixerTestSuit::irregular_single_mixer_call_test);
	}

protected:
	virtual void setup(){} // Inicjalizacja wspólnych struktur danych
	virtual void tear_down() {} // Zwalnianie miejsca

private:
	// Metody testujące
	void regularly_distributed_single_mixer_call_test() {} 
	void regularly_distributed_extreme_short_values_test() {}
	void irregularly_distributed_single_mixer_call_test() {}

	// Wspólne:
	// metody:
	void init_data(const int size_of_buffer, const int inputs_size) {}
	// stałe:
	static const int QUEUES; // Liczba kolejeka kolejki
	static const int TX_INTERVAL_MS; // Częstotliwość nadawania komunikatów
	// struktury danych
	std::vector<int16_t> queues[QUEUES];
	mixer_input* inputs;
	std::vector<int16_t> buffer;
	size_t* buffer_size;
	// mixer:
	Mixer mixer;
};

bool run_tests()
{
	MixerTestSuit mts;
	Test::TextOutput output(Test::TextOutput::Terse);
	return mts.run(output, false);
}
#endif