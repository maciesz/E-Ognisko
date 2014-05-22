#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>

#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

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
		const std::string& server_name, boost::uint16_t retransmit_limit);

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

	void handle_client_id(const boost::system::error_code& error);

	void handle_tcp_raport(const boost::system::error_code& error);


	//=========================================================================
	//
	// Handlery UDP.
	//
	//=========================================================================
	void handle_udp_connect(const boost::system::error_code& error);

	void handle_read_datagram(const boost::system::error_code& error);

	void handle_write_datagram(const boost::system::error_code& error);
	
	void handle_client_dgram(const boost::system::error_code& error);

	void handle_upload_dgram(const boost::system::error_code& error);

	void handle_data_dgram(const boost::system::error_code& error);

	void handle_ack_dgram(const boost::system::error_code& error);

	void handle_retransmit_dgram(const boost::system::error_code& error);

	void handle_keepalive_dgram(const boost::system::error_code& error);


	//=========================================================================
	//
	// Close.
	//
	//=========================================================================
	void close();


	//=========================================================================
	//
	// Handlery I/O.
	//
	//=========================================================================
	void handle_read_from_stdin(const boost::system::error_code& error);

	void handle_write_to_stdout(const boost::system::error_code& error);


	//=========================================================================
	//
	// Funkcje.
	//
	//=========================================================================
	void create_udp_socket(const int bytes_transferred);


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
	boost::asio::streambuf header_buffer_;
	// Bufory wiadomości na wejście/wyjście
	char* raport_buf_;
	char* read_buf_;
	char* write_buf_;
	// Parametry serwera
	const std::string port_;
	const std::string server_name_;
	// Współczynnik retransmitowania
	const boost::uint16_t retransmit_limit_;
	// Identyfikator klienta w komunikacji z serwerem
	boost::uint16_t client_id;
	// Fabryka komunikatów
	const header_factory factory_;
};	
#endif