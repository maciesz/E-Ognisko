#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <iostream>
#include <algorithm>

#include "../common/queue_state.hpp"

class client_data
{
public:
	//=========================================================================
	//
	// Konstruktor i destruktory.
	//
	//=========================================================================
	client_data(size_t max_queue_length, const std::string& endpoint, 
		const boost::uint16_t fifo_high_watermark,
		const boost::uint16_t fifo_low_watermark);

	~client_data();


	//=========================================================================
	//
	// Metody wywoływane po przetworzeniu danych przez mikser.
	//
	//=========================================================================
	void actualize_content_after_mixery(boost::uint16_t bytes_transferred);

	void actualize_content_after_upload(
		std::vector<boost::int16_t>& upload_data);

	std::string print_statistics();

	queue_state rate_queue_state();
private:
	//=========================================================================
	//
	// Metody prywatne(podwykonawcy) wywoływane w czasie aktualizacji kontenera
	//
	//=========================================================================
	void pop_front_sequence(boost::uint16_t shift);

	void reset_statistics();


	//=========================================================================
	//
	// Zmienne prywatne.
	//
	//=========================================================================

	// Maksymalny rozmiar kolejki:
	const size_t max_queue_length_;
	// Endpoint klienta:
	const std::string endpoint_;
	// Warunki na stany kolejki:
	const boost::uint16_t fifo_high_watermark_;
	const boost::uint16_t fifo_low_watermark_;
	// Faktyczny rozmiar kolejki:
	boost::uint16_t queue_size_;
	// Kolejka:
	std::vector<boost::int16_t> queue_;
	// Minimalna ilość bajtów w kolejce od ostatniego raportu
	boost::uint16_t min_bytes_;
	// Maksymalna ilość bajtów w kolejce od ostatniego raportu
	boost::uint16_t max_bytes_;
};
#endif