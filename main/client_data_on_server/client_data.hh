#ifndef CLIENT_DATA_HH
#define CLIENT_DATA_HH

#include <cstdint>
#include <vector>
#include <list>
#include <iostream>
#include <algorithm>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "../common/queue_state.hh"
#include "../common/structures.hh"

class client_data
{
public:
	//=======================================================================//
	//                                                                       //
	// Konstruktor i destruktory.                                            //
	//                                                                       //
	//=======================================================================//
	client_data(
		size_t max_queue_length, 
		const std::string& endpoint,
		const boost::asio::ip::udp::endpoint udp_endpoint,
		size_t fifo_high_watermark, 
		size_t fifo_low_watermark,
		size_t buf_len);

	~client_data();


	//=======================================================================//
	//                                                                       //
	// Metody wywoływane po przetworzeniu danych przez mikser.               //
	//                                                                       //
	//=======================================================================//
	void actualize_content_after_mixery(size_t bytes_transferred);

	void actualize_content_after_upload(
		std::vector<std::int16_t>& upload_data);

	std::string get_statistics();

	queue_state rate_queue_state();

	std::vector<std::int16_t>& get_client_msg_queue();

	std::list<std::string> get_last_dgrams(
		const size_t inf_nr);

	size_t get_queue_size();

	size_t get_available_size();

	size_t get_last_dgram_nr();

	void add_to_dgrams_list(const std::string& dgram, const size_t dgram_nr);

	void actualize_last_dgram_nr();
	// Endpoint faktyczny:
	boost::asio::ip::udp::endpoint udp_endpoint_;
private:
	//=======================================================================//
	//                                                                       //
	// Metody wywoływane w czasie aktualizacji kontenera.                    //
	//                                                                       //
	//=======================================================================//
	void pop_front_sequence(size_t shift);

	void reset_statistics();


	//=======================================================================//
	//                                                                       //
	// Zmienne.                                                              //
	//                                                                       //
	//=======================================================================//

	// Maksymalny rozmiar kolejki:
	const size_t max_queue_length_;
	// Endpoint klienta:
	const std::string endpoint_;
	// Warunki na stany kolejki:
	const size_t fifo_high_watermark_;
	const size_t fifo_low_watermark_;
	// Długość listy ostatnich komunikatów:
	const size_t last_dgrams_max_size_;
	// Faktyczny rozmiar kolejki:
	size_t queue_size_;
	// Kolejka:
	std::vector<std::int16_t> queue_;
	// Lista ostatnich komunikatów:
	std::list<retransmit_dgram> last_dgrams_;
	// Minimalna ilość bajtów w kolejce od ostatniego raportu:
	size_t min_bytes_;
	// Maksymalna ilość bajtów w kolejce od ostatniego raportu:
	size_t max_bytes_;
	// Oczekiway numer datagramu klienta.
	size_t expected_client_datagram_nr_;
};
#endif
