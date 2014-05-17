#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <climits>

#include "../../../main/mixer/mixer.h"
#include "../../../main/headers/structures.h"
#include "../src/cpptest.h"

enum OutputType
{
	Compiler,
	Html,
	TextTerse,
	TextVerbose
};

static void
usage()
{
	std::cout << "usage: mytest [MODE]\n"
		 << "where MODE may be one of:\n"
		 << "  --compiler\n"
		 << "  --html\n"
		 << "  --text-terse (default)\n"
		 << "  --text-verbose\n";
	exit(0);
}

static std::auto_ptr<Test::Output>
cmdline(int argc, char** argv)
{
	if (argc > 2)
		usage(); // will not return
	
	Test::Output* output = 0;
	
	if (argc == 1)
		output = new Test::TextOutput(Test::TextOutput::Verbose);
	else
	{
		const char* arg = argv[1];
		if (strcmp(arg, "--compiler") == 0)
			output = new Test::CompilerOutput;
		else if (strcmp(arg, "--html") == 0)
			output =  new Test::HtmlOutput;
		else if (strcmp(arg, "--text-terse") == 0)
			output = new Test::TextOutput(Test::TextOutput::Terse);
		else if (strcmp(arg, "--text-verbose") == 0)
			output = new Test::TextOutput(Test::TextOutput::Verbose);
		else
		{
			std::cout << "invalid commandline argument: " << arg << std::endl;
			usage(); // will not return
		}
	}
	
	return std::auto_ptr<Test::Output>(output);
}

class MixerTestSuite: public Test::Suite
{
public:
	MixerTestSuite()
	{
		TEST_ADD(MixerTestSuite::regularly_distributed_single_mixer_call_test);
		TEST_ADD(MixerTestSuite::regularly_distributed_extreme_short_values_test);
		TEST_ADD(MixerTestSuite::irregularly_distributed_single_mixer_call_test);
	}

protected:
	virtual void setup(); // Inicjalizacja wspólnych struktur danych
	virtual void tear_down(); // Zwalnianie miejsca
private:
	// Metody testujące
	void regularly_distributed_single_mixer_call_test();
	void regularly_distributed_extreme_short_values_test();
	void irregularly_distributed_single_mixer_call_test();
	
	// Wspólne:
	// struktury danych:
	std::vector<int16_t> queues[13];
	mixer_input* inputs;
	std::vector<int16_t> buffer;
	size_t* buffer_size;
	size_t bytes;
	// mixer:
	Mixer mixer;
	// metody:
	void init_data(const int size_of_buffer, const int inputs_size);
	// stałe:
	static const int QUEUES; // Liczba kolejeka kolejki
	static const int TX_INTERVAL_MS; // Częstotliwość nadawania komunikatów
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

	// Zwolnij pamięć zajmowaną przez wskaźnik na domyślny rozmiar bufora
	//delete buffer_size;
}

// Testy
void MixerTestSuite::regularly_distributed_single_mixer_call_test()
{
	size_t size_of_buffer = 2 * 2;
	const int inputs_size = 4;

	buffer_size = &size_of_buffer;
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

	// Wywołanie metody mixer na.mixerze:
	mixer.mixer(&inputs[0], inputs_size , static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	// Sprawdź zawartość bufora
	TEST_ASSERT(buffer[0] == 1 + 5 + 2001 + 28);
	TEST_ASSERT(buffer[1] == 10 + 19 + 789 + 45);

	// Sprawdź, czy 'consumed' są poprawnie wyznaczone
	for (int i = 0; i< inputs_size; ++i) {
		TEST_ASSERT(inputs[i].consumed == 4);
	}
}

void MixerTestSuite::regularly_distributed_extreme_short_values_test()
{
	size_t size_of_buffer = 2 * 2;
	const int inputs_size = 3;

	buffer_size = &size_of_buffer;
	// Równomierna inicjalizacja niektórych kolejek w tablicy
	queues[0].push_back(15000);
	queues[0].push_back(-13000);

	queues[1].push_back(20000);
	queues[1].push_back(-15000);

	queues[5].push_back(4000);
	queues[5].push_back(-10000);

	// Inicjalizacja danych:
	init_data(size_of_buffer, inputs_size);

	// Wywołanie metody mixer na.mixerze:
	mixer.mixer(&inputs[0], inputs_size, static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	// Sprawdź zawartość bufora 
	TEST_ASSERT(buffer[0] == SHRT_MAX);
	TEST_ASSERT(buffer[1] == SHRT_MIN);

	// Sprawdź, czy 'consumed' są poprawnie wyznaczone
	for (int i = 0; i< inputs_size; ++i) {
		TEST_ASSERT(inputs[i].consumed == 4);
	}
}

void MixerTestSuite::irregularly_distributed_single_mixer_call_test()
{

	size_t size_of_buffer = 4 * 2;
	const int inputs_size = 3;

	buffer_size = &size_of_buffer;
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

	// Wywołanie metody mixer na.mixerze:
	mixer.mixer(&inputs[0], inputs_size, static_cast<void*>(&buffer), buffer_size, TX_INTERVAL_MS);

	// Sprawdź zawartość bufora
	TEST_ASSERT(buffer[0] == 1029 + 9 + 13);
	TEST_ASSERT(buffer[1] == -8129 + 9);
	TEST_ASSERT(buffer[2] == 1253);
	TEST_ASSERT(buffer[3] == 123);

	// Sprawdź, czy 'consumed' są poprawnie wyznaczone
	TEST_ASSERT(inputs[0].consumed == 8);
	TEST_ASSERT(inputs[1].consumed == 2);
	TEST_ASSERT(inputs[2].consumed == 4);
}

void MixerTestSuite::init_data(const int size_of_buffer, const int inputs_size)
{
	// Zarezerwuj wymaganą pamięć
	buffer.reserve(size_of_buffer);

	// Deklaracja struktury:
	inputs = new mixer_input[inputs_size];

	// Inicjalizacja inputsów:
	int next = 0;
	for (int i = 0; i< QUEUES; ++i) {
		if (queues[i].size() > 0) {
			inputs[next].len = 2 * queues[i].size();
			inputs[next].consumed = 0;
			inputs[next].data = static_cast<void*>(&queues[i]);
			next++;
		}
	}
}

const int MixerTestSuite::QUEUES = 13;

const int MixerTestSuite::TX_INTERVAL_MS = 5;

int main(int argc, char** argv)
{
	try
	{
		// Demonstrates the ability to use multiple test suites
		//
		Test::Suite ts;
		ts.add(std::auto_ptr<Test::Suite>(new MixerTestSuite));

		// Run the tests
		//
		std::auto_ptr<Test::Output> output(cmdline(argc, argv));
		ts.run(*output, true);

		Test::HtmlOutput* const html = dynamic_cast<Test::HtmlOutput*>(output.get());
		if (html)
			html->generate(std::cout, true, "MyTest");
	}
	catch (...)
	{
		std::cout << "unexpected exception encountered\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
