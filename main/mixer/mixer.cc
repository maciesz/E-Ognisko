#include <climits>
#include <algorithm>
#include <set>

#include "mixer.hpp"

void Mixer::mixer(mixer_input* inputs, size_t n, 
		void* output_buf, size_t* output_size, unsigned long tx_interval_ms) const
{
	// Wyznacz rozmiar danych, który zostanie zapisany do bufora(w bajtach)
	const size_t requested_length = MULTIPLIER * tx_interval_ms;
	const size_t total_bytes = get_total_bytes(inputs, n);
	const size_t out_size = std::min(*output_size, std::min(requested_length, total_bytes));

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
		if (inputs[i].len > 1) {

			data[i] = static_cast<std::vector<int16_t>*>(inputs[i].data);

		}

	// Zmienne pomocnicze:
	size_t iteration = 0;
	int shorts_sum;
	size_t idx;

	// Castowanie bufora danych wyjściowych na typ domyślnie przyjęty w serwerze
	std::vector<int16_t>* output_data_buf = static_cast<std::vector<int16_t>*>(output_buf);


	// Uzupełnianie bufora wynikowego:
	while (2 * iteration < out_size) {
		shorts_sum = 0;

		// Przejdź po wszystkich niepustych kolejkach:
		for (auto it = set.begin(); it != set.end();) {
			
			// Wyznacz indeks i dodaj shorta do sumy:
			idx = *it;

			shorts_sum += data[idx]->at(iteration);

			// Jeżeli był to ostatni element w kolejce
			// lub po tym elemencie został jeden bajt, 
			// to wyrzuć id_kolejki ze struktury tymczasowej:
			if (2 * iteration >= inputs[idx].len - 2)
				set.erase(it++);
			else
				++it;

			// Zaktualizuj liczbę pobranych bajtów w strukturze mixer_input:
			inputs[idx].consumed += 2;
		}

		const int16_t short_min = static_cast<int16_t>(std::max(SHRT_MIN, static_cast<int>(shorts_sum)));
		const int16_t short_max = static_cast<int16_t>(std::min(SHRT_MAX, static_cast<int>(shorts_sum)));
		const int16_t final_short = (shorts_sum < 0) ? short_min : short_max;

		// Wpisz kolejną liczbę 16-bitową na kolejną pozycję
		(*output_data_buf)[iteration++] = final_short;
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

size_t Mixer::get_total_bytes(const mixer_input* inputs, const int size) const
{
	size_t total_bytes = 0;
	// Wyznacz sumaryczną liczbę bajtów możliwą do skonsumowania
	for (int i = 0; i< size; ++i)
		total_bytes = std::max((inputs[i].len / 2) * 2, total_bytes);

	return total_bytes;
}
