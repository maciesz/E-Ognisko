#ifndef SERVER_HPP
#define SERVER_HPP

// Biblioteki boost'owe
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/cstdint.hpp>
// Biblioteki standardowe
#include <cstring>
#include <iostream>
#include <sstream> // std::ostringstream
#include <iterator>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <cmath>
// Własne biblioteki/struktury
#include "../common/header_titles.hpp"
#include "../common/structures.hpp"
#include "../common/global_variables.hpp"
#include "../factories/header_factory.hpp"
#include "../parsers/headerline_parser.hpp"
#include "../client_data_on_server/client_data.hpp"
#include "../message_converter/message_converter.hpp"
#include "../string_converter/string_converter.hpp"
#include "../mixer/mixer.hpp"
#include "../string_converter/string_converter.hpp"

class server
{
public:
	//=========================================================================
	//
	// Konstruktory i destruktory.
	//
	//=========================================================================
	server(
		boost::asio::io_service& io_service,
		boost::uint16_t port, 
		boost::uint16_t fifo_size,
		boost::uint16_t fifo_low_watermark, 
		boost::uint16_t fifo_high_watermark,
		boost::uint16_t buf_len,
		boost::uint16_t tx_interval
	);

	~server();
	//=========================================================================
	// Zablokuj możliwość kopiowania konkretnego serwera
	//=========================================================================
	server& operator=(const server&) = delete;
	server(const server&) = delete;
private:
	//=========================================================================
	//
	// TCP.
	//
	//=========================================================================
	void start_accepting_on_tcp(const boost::system::error_code& error);

	void pass_clientid(
		boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket,
		const boost::system::error_code& error);


	//=========================================================================
	//
	// UDP.
	//
	//=========================================================================
	void handle_udp_message(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void manage_read_header(base_header* header, std::string& body);

	void handle_receive_udp(const boost::system::error_code& error);

	void handle_data_transmission(
		boost::shared_ptr<boost::asio::ip::udp::endpoint> remote_endpoint,
		std::list<std::string>& dgram_list,
		const boost::system::error_code& error);

	std::string convert_remote_udp_endpoint_to_string();


	//=========================================================================
	//
	// Raport.
	//
	//=========================================================================
	void send_raport_info();

	void handle_after_send_raport(
		boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket,
		const boost::system::error_code& error);

	void restart_timer();

	//=========================================================================
	//
	// Mixernia.
	//
	//=========================================================================
	void mixer();

	void handle_next_mixery();


	//=========================================================================
	//
	// Funkcje na danych konkretnych klientów
	//
	//=========================================================================
	size_t get_next_clientid();


	//=========================================================================
	//
	// Zmienne prywatne
	//
	//=========================================================================

	// TCP Akceptor:
	boost::asio::ip::tcp::acceptor tcp_acceptor_;
	// Sockety:
	boost::asio::ip::udp::socket udp_socket_;
	// Resolvery:
	boost::asio::ip::tcp::resolver tcp_resolver_;
	boost::asio::ip::udp::resolver udp_resolver_;
	// Bufory do komunikacji po tcp:
	boost::asio::streambuf header_buf_;
	boost::asio::streambuf body_buf_;
	boost::asio::streambuf raport_buf_;
	// Bufory do komunikacji po udp:
	boost::array<char, CLIENT_BUFFER_LEN> read_buf_;
	//boost::array<char, CLIENT_BUFFER_LEN> write_buf_;
	std::vector<boost::int16_t> write_buf_;
	// Kolejny wolny identyfikator
	boost::uint32_t next_clientid_;
	// Numer aktualnie wysyłanego datagramu:
	boost::uint32_t current_datagram_nr_;
	// Bufor do miksera:
	std::vector<boost::int16_t> mixer_buf_;
	// Długość zmiksowanej wiadomości:
	const boost::uint16_t mixed_msg_size_;
	// Fabryka do nagłówków komunikatów:
	header_factory factory_;
	// Parametry kolejki:
	const boost::uint16_t fifo_size_;
	const boost::uint16_t fifo_low_watermark_;
	const boost::uint16_t fifo_high_watermark_;
	// Port:
	const boost::uint16_t port_;
	// Długość kolejki zapamiętanych datagramów:
	const boost::uint16_t buf_len_;
	// Parametr powtarzalności mixowania
	const boost::uint16_t tx_interval_;
	// Zegarki:
	boost::asio::deadline_timer mixer_timer_;
	boost::asio::deadline_timer raport_timer_;
	//=========================================================================
	// Mixer
	//=========================================================================
	// Jako klasa statyczna

	//=========================================================================
	// Mapy:
	//=========================================================================
	
	// [klucz]: endpoint UDP klienta jako human readable form,
	// [wartość]: identyfikator przypisany konkretnemu klientowi
	std::map<std::string, boost::uint32_t> client_map_;
	// [klucz]: identyfikator klienta
	// [wartość]: struktura client_data przechowująca
	// -> kolejkę(bufor) klienta
	// -> endpoint UDP w postaci stringa
	// -> endpoint w postaci klasycznej
	// -> statystyki:
	//      + minimalną liczbę bajtów w kolejce(od ostatniego raportu)
	//      + maksymalną liczbę bajtów w kolejce(od ostatniego raportu)
	//      + aktualny rozmiar kolejki
	// -> listę ostatnich datagramów przesłanych przez klienta
	std::map<boost::uint32_t, client_data*> client_data_map_;
	// [klucz]: endpoint TCP klienta w postaci klasycznej
	// [wartość]: identyfikator klienta
	std::map<boost::uint32_t, boost::asio::ip::tcp::endpoint> client_tcp_map_;
	// Endpoint UPD dla gniazda klienta:
	boost::asio::ip::udp::endpoint udp_remote_endpoint_;
};	
#endif