#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <list>
#include <iostream>
#include <algorithm>

#include "../common/queue_state.hpp"
#include "../common/structures.hpp"

class client_data
{
public:
	//=========================================================================
	//
	// Konstruktor i destruktory.
	//
	//=========================================================================
	client_data(
		size_t max_queue_length, 
		const std::string& endpoint,
		const boost::asio::ip::udp::endpoint udp_endpoint,
		boost::uint16_t fifo_high_watermark, 
		boost::uint16_t fifo_low_watermark,
		boost::uint16_t buf_len);

	~client_data();


	//=========================================================================
	//
	// Metody wywoływane po przetworzeniu danych przez mikser.
	//
	//=========================================================================
	void actualize_content_after_mixery(boost::uint16_t bytes_transferred);

	void actualize_content_after_upload(
		std::vector<boost::int16_t>& upload_data);

	std::string get_statistics();

	queue_state rate_queue_state();

	std::vector<boost::int16_t>& get_client_msg_queue();

	std::list<std::string> get_last_dgrams(
		const boost::uint32_t inf_nr);

	size_t get_queue_size();

	// Endpoint faktyczny:
	boost::asio::ip::udp::endpoint udp_endpoint_;
private:
	//=========================================================================
	//
	// Metody prywatne(podwykonawcy) wywoływane w czasie aktualizacji kontenera
	//
	//=========================================================================
	void pop_front_sequence(boost::uint16_t shift);

	void reset_statistics();

	void add_to_upload_list(
		const std::string& upload_msg, boost::uint32_t dgram_nr);


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
	// Długość listy ostatnich komunikatów:
	const boost::uint16_t last_uploads_max_size_;
	// Faktyczny rozmiar kolejki:
	boost::uint16_t queue_size_;
	// Kolejka:
	std::vector<boost::int16_t> queue_;
	// Lista ostatnich komunikatów:
	std::list<client_upload> last_uploads_;
	// Minimalna ilość bajtów w kolejce od ostatniego raportu:
	boost::uint16_t min_bytes_;
	// Maksymalna ilość bajtów w kolejce od ostatniego raportu:
	boost::uint16_t max_bytes_;
};
#endif