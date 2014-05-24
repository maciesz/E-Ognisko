#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>

#include "../common/header_titles.hpp"
#include "../common/structures.hpp"
#include "../common/global_variables.hpp"
#include "../factories/header_factory.hpp"
#include "../parsers/client_parser.hpp"
#include "../exceptions/invalid_header_exception.hpp"

class client
{
public:
	//=========================================================================
	//
	// Konstruktory i destruktory.
	//
	//=========================================================================
	client(boost::asio::io_service& io_service, const std::string& port, 
		const std::string& server_name, boost::uint16_t retransmit_limit, 
		boost::uint16_t keepalive_period);

	~client();

	//=========================================================================
	// Zablokuj możliwość kopiowania konkretnego klienta
	//=========================================================================
	client& operator=(const client&) = delete;
	client(const client&) = delete;
private:
	//=========================================================================
	//
	// Handlery TCP.
	//
	//=========================================================================
	void handle_tcp_connect(const boost::system::error_code& error);

	void handle_client_id(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void handle_tcp_raport(const boost::system::error_code& error);

	void handle_write_tcp_raport(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void handle_read_tcp_raport(const boost::system::error_code& error);


	//=========================================================================
	//
	// Handlery i funkcje w UDP.
	//
	//=========================================================================
	void create_udp_socket(const int bytes_transferred);

	void handle_udp_connect(const boost::system::error_code& error,
		const int bytes_transferred);

	void handle_client_dgram(const boost::system::error_code& error);

	void handle_read_datagram_header(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void manage_read_header(boost::shared_ptr<base_header> header);

	void handle_write_datagram(const boost::system::error_code& error);
	
	void resend_last_datagram();

	void send_keepalive_dgram();

	// TODO: 
	// void handle_retransmit_dgram(const boost::system::error_code& error);


	//=========================================================================
	//
	// Funkcje zegarków
	//
	//=========================================================================
	void wait_for_next_keepalive(const boost::system::error_code& error);


	//=========================================================================
	//
	// Close.
	//
	//=========================================================================
	//void close_tcp();


	//=========================================================================
	//
	// Handlery I/O.
	//
	//=========================================================================
	void handle_read_from_stdin(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void handle_write_to_stdout(const boost::system::error_code& error,
		const size_t bytes_transferred);

	void handle_after_read_from_stdin(const boost::system::error_code& error,
		const size_t bytes_transferred);



	//=========================================================================
	//
	// Zmienne.
	//
	//=========================================================================

	// Sockety
	boost::asio::ip::tcp::socket tcp_socket_;
	boost::asio::ip::udp::socket udp_socket_;
	// Resolvery
	boost::asio::ip::tcp::resolver tcp_resolver_;
	boost::asio::ip::udp::resolver udp_resolver_;
	// Deskryptory wejścia/wyjścia
	boost::asio::posix::stream_descriptor input_;
	boost::asio::posix::stream_descriptor output_;
	// StreambufY
	//boost::asio::streambuf header_buffer_;
	// Bufory wiadomości na wejście/wyjście
	char* header_buffer_;
	char* body_buffer_;
	char* raport_buf_;
	char* read_buf_;
	char* write_buf_;
	char* input_buffer_;
	char* output_buffer_;

	// Parametry serwera
	const std::string port_;
	const std::string server_name_;
	// Parametr powtarzalności keepalive'a:
	const boost::uint16_t keepalive_period_;
	// Współczynnik retransmisji
	const boost::uint16_t retransmit_limit_;
	// Identyfikator klienta w komunikacji z serwerem
	boost::uint16_t client_id_;
	// Numer kolejnego dgramu(są liczone od pierwszego przesłanego komunikatu)
	boost::int32_t nr_expected_;
	// Numer ostatnio wysłanego datagramu przez konkretnego klienta
	boost::int32_t actual_dgram_nr_;
	// Liczba wolnych bajtów w FIFO
	boost::int32_t win_;
	// Fabryka komunikatów
	const header_factory factory_;
	// Ostatni tytuł nagłówka
	std::string last_header_title_;
	// Ostatni datagram
	std::string last_datagram_;
	// Zegarki
	boost::asio::deadline_timer rerun_timer_; // ponowne łączenie z serwerem
	boost::asio::deadline_timer keepalive_timer_; // zegarek do keepalive'ów
};	
#endif