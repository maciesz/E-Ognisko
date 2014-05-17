#include <vector>

// Struktura mixer_input
#include "structures.h"

class MixerTestSuite: public Test::Suite
{
public:
	MixerTestSuite()
	{
		TEST_ADD(MixerTestSuite::single_mixer_call_test);
		TEST_ADD(MixerTestSuite::extreme_short_value_test);
		TEST_ADD(MixerTestSuite::irregular_single_mixer_call_test);
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

// Wspólne ustawienia
void MixerTestSuite::setup() 
{
}

void MixerTestSuite::tear_down()
{
	// Wyczyść kolejki
	for (int i = 0; i< QUEUES; ++i)
		queues[i].clear();

	// Wyczyść bufor
	buffer.clear();

	// Zwolnij bezpiecznie pamięć po tablicy struktur mixer_input
	delete[] inputs;
	inputs = nullptr;

	// Zwolnij pamięć zajmowaną przez wskaźnik na domyślny rozmiar bufora
	delete buffer_size;
	buffer_size = nullptr;
}

// Testy
void MixerTestSuite::regular_single_mixer_call_test()
{
	const int size_of_buffer = 2;
	const int inputs_size = 4;
	// Równomierna inicjalizacja niektórych kolejek w tablicy:
	queues[0].push_back(1);
	queues[0].push_back(10);

	queues[4].push_back(5);
	queues[4].push_back(19);

	queues[5].push_back(2001);
	queues[5].push_back(789);

	queues[9].push_back(28);
	queues[9].push_back(45);

	// Inicjalizacja danych:
	init_data(size_of_buffer, inputs_size);

	// Wywołanie metody mikser na mikserze:
	mikser.mikser(&inputs[0], size, static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	TEST_ASSERT(buffer[0] == 1 + 5 + 2001 + 28);
	TEST_ASSERT(buffer[1] == 10 + 19 + 789 + 45);
}

void MixerTestSuite::regularly_distributed_extreme_short_values_test()
{
	const int size_of_buffer = 2;
	const int inputs_size = 3;
	// Równomierna inicjalizacja niektórych kolejek w tablicy
	queues[0].push_back(15000);
	queues[0].push_back(-13000);

	queues[1].push_back(20000);
	queues[1].push_back(-15000);

	queues[5].push_back(4000);
	queues[5].push_back(-10000);

	// Inicjalizacja danych:
	init_data(size_of_buffer, inputs_size);

	// Wywołanie metody mikser na mikserze:
	mikser.mikser(&inputs[0], size, static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	TEST_ASSERT(buffer[0] == SHRT_MIN);
	TEST_ASSERT(buffer[1] == SHRT_MAX);
}

void MixerTestSuite::irregularly_distributed_single_mixer_call_test()
{
	const int size_of_buffer = 4;
	const int inputs_size = 3;

	// Nierównomierna inicjalizacja pewnych kolejek
	queues[3].push_back(1029);
	queues[3].push_back(-8129);
	queues[3].push_back(1253);
	queues[3].push_back(123);

	queues[9].push_back(9);

	queues[12].push_back(13);
	queues[12].push_back(9);

	// Inicjalizacja danych
	init_data(size_of_buffer, inputs_size);

	// Wywołanie metody mikser na mikserze:
	mikser.mikser(&inputs[0], size, static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	TEST_ASSERT(buffer[0] == 1029 + 9 + 13);
	TEST_ASSERT(buffer[1] == -8129 + 9);
	TEST_ASSERT(buffer[2] == 1253);
	TEST_ASSERT(buffer[3] == 123);
}

void init_data(const int size_of_buffer, const int inputs_size)
{
	// Zarezerwuj wymaganą pamięć
	buffer.reserve(size_of_buffer);
	*buffer_size = 2 * size_of_buffer;

	// Deklaracja struktury:
	inputs = new mixer_input[inputs_size];

	// Inicjalizacja inputsów:
	int next = 0;
	for (int i = 0; i< QUEUES; ++i) {
		if (queues[i].size() > 0) {
			inputs[next++].len = 2 * queues[i].size();
			inputs[next++].data = static_cast<void*>(&queues[i]);
		}
	}

}

int MixerTestSuite::QUEUES = 13;

int MixerTestSuite::TX_INTERVAL_MS = 5;
