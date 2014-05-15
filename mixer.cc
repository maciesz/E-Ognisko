#include <climits>
#include <algorithm>
#include <set>

#include "mixer.h"

void Mixer::mixer(mixer_input* inputs, size_t n, 
		void* output_buf, size_t* output_size, unsigned long tx_interval_ms) const
{
	// Wyznacz rozmiar danych, który zostanie zapisany do bufora(w bajtach)
	const size_t requested_length = MULTIPLIER * tx_interval_ms;
	const size_t total_bytes = get_total_bytes(inputs, n);
	const size_t out_size = std::min(requested_length, total_bytes);

	// Struktura przechowująca numery kolejek, 
	// w których w poszczególnych iteracjach są jeszcze bajty danych
	std::set<size_t> set;

	// Inicjalizacja struktury:
	for (int i = 0; i< n; ++i)
		set.insert(i);

	// Struktura przechowująca kolejki FIFO:
	std::vector<int16_t>* data[n + 1];

	// Inicjalizacja struktury:
	for (int i = 0; i< n; ++i)
		if (inputs[i].len > 1)
			data[i] = static_cast<std::vector<int16_t>*>(inputs[i].data);

	// Bufor liczb 16-bitowych ze znakiem:
	int16_t* buffer = new int16_t[n];

	// Zmienne pomocnicze:
	size_t written_shorts = 0;
	size_t iteration = 0;
	size_t shorts_sum;
	size_t idx;

	// Uzupełnianie bufora wynikowego:
	while (written_shorts < out_size) {
		shorts_sum = 0;
		// Przejdź po wszystkich niepustych kolejkach:
		for (auto it = set.begin(); it != set.end();) {
			
			// Wyznacz indeks i dodaj shorta do sumy:
			idx = *it;
			shorts_sum += (*data)[idx][iteration];

			// Jeżeli był to ostatni element w kolejce, 
			// to wyrzuć id_kolejki ze struktury:
			if (iteration == inputs[idx].len - 2)
				set.erase(it++);
			else
				++it;

			// Zaktualizuj liczbę pobranych bajtów w strukturze mixer_input:
			inputs[idx].consumed += 2;
			// oraz w written_shorts:
			written_shorts += 2;
		}

		const int16_t short_min = static_cast<int16_t>(std::max(SHRT_MIN, static_cast<int>(shorts_sum)));
		const int16_t short_max = static_cast<int16_t>(std::min(SHRT_MIN, static_cast<int>(shorts_sum)));
		const int16_t final_short = (shorts_sum < 0) ? short_min : short_max;

		// Wpisz kolejną liczbę 16-bitową na kolejną pozycję
		// TODO:
		//*(static_cast<int16_t*>(&output_buf)) = final_short;
	}

	// Usuń ze strukruty numery tych kolejek, 
	// które nie mogły być przejrzane do końca 
	// ze względu na ograniczenie requested_length.
	set.clear();

	// Zaktualizuj rozmiar przesłanych danych (w bajtach)
	*output_size = out_size;
}

void Mixer::init_consumed_bytes(mixer_input* inputs, const int size) const
{
	// Zainicjuj liczbę skonsumowanych bajtów w każdej kolejce FIFO
	for (int i = 0; i< size; ++i) {
		inputs[i].consumed = 0;
	}
}

unsigned long Mixer::get_total_bytes(const mixer_input* inputs, const int size) const
{
	unsigned long total_bytes = 0;
	// Wyznacz sumaryczną liczbę bajtów możliwą do skonsumowania
	for (int i = 0; i< size; ++i)
		total_bytes += (inputs[i].len / 2) * 2;

	return total_bytes;
}
