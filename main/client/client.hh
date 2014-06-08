#ifndef CLIENT_HH
#define CLIENT_HH

// Biblioteki STL-owe i standardowe i C-owa:
#include <algorithm>
#include <array> // std::array<*, size>
#include <cmath>
#include <cstdint> // std::uint*_t
#include <cstdlib>
#include <cstring>
#include <iostream> // cerr, cout, cin, etc.
#include <iterator> // pewnie back_inserter
#include <sstream> // std::istream
#include <vector>

// Biblioteki boost'owe:
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

// Własne biblioteki/struktury:
// 1) Struktury:
#include "../common/structures.hh"
#include "../common/header_titles.hh"
#include "../common/global_variables.hh"
// 2) Parsery:
#include "../common_parsers/headerline_parser.hh"
#include "../message_converter/message_converter.hh"
// 3) Fabryki:
#include "../factories/header_factory.hh"
// 4) Wyjątki:
#include "../exceptions/invalid_header_exception.hh"


class client
{
public:
	//=======================================================================//
	//                                                                       //
	// Konstruktory i destruktory.                                           //
	//                                                                       //
	//=======================================================================//
	/// Konstruktor.
	client(
		boost::asio::io_service&, 
		std::string& port, 
		std::string& host,
		size_t retransmit_limit,
		size_t keepalive_period,
		size_t reconnect_period,
		size_t connection_period
	);
	/// Destruktor.
	~client();
	/// Blokady kopiowania konkretnego klienta.
	client& operator=(const client&) = delete;
	client(const client&) = delete;
private:
	
	/// Parametr częstości podejmowania prób nawiązania połączenia po TCP.
	const size_t reconnect_period_;
	/// Zegarek do ponownego łączenia z serwerem
	boost::asio::deadline_timer reconnect_timer_;

	//=======================================================================//
	// Parametry serwera.                                                    //
	//=======================================================================//
	/// Port.
	const std::string port_;
	/// Adres.
	const std::string address_;

	/// Zwolnij wszystkie zasoby klienta.
	void free_resources();
	/// Obserwuj połączenie z serwerem.
	void monitor_connection();

	//=======================================================================//
	//                                                                       //
	// TCP.                                                                  //
	//                                                                       //
	//=======================================================================//

	//=======================================================================//
	// Metody.                                                               //
	//                                                                       //
	//=======================================================================//
	/// Połącz się z konkretnym serwerem po TCP.
	void do_async_tcp_connect();
	/// Przeczytaj nagłówek wiadomości.
	void do_read_header();
	/// Przeczytaj ,,body'' wiadomości.
	void do_read_body(const size_t bytes_to_be_transferred);
	/// Wypisz wiadomość na standardowe wyjście.
	void do_write_to_STDOUT();
	/// Obsłuż po połączeniu.
	void handle_after_connect(const boost::system::error_code&);
	/// Ponów próbę półączenia się po TCP z serwerem.
	void do_tcp_reconnect();

	//=======================================================================//
	// Zmienne.                                                              //
	//                                                                       //
	//=======================================================================//
	/// Gniazdo do łączenia się po TCP.
	boost::asio::ip::tcp::socket tcp_socket_;
	/// Resolver.
	boost::asio::ip::tcp::resolver tcp_resolver_;
	/// Endpoint serwera.
	boost::asio::ip::tcp::endpoint tcp_endpoint_iterator_;
	/// Deskryptor wyjścia.
	boost::asio::posix::stream_descriptor output_;
	/// Bufor na raporty.
	boost::asio::streambuf raport_buffer_;
	/// Maksymalny czas, przez który nie otrzymaliśmy żadnego komunikatu po TCP.
	const size_t connection_period_;

	//=======================================================================//
	//                                                                       //
	// UDP.                                                                  //
	//                                                                       //
	//=======================================================================//

	//=======================================================================//
	// Metody.                                                               //
	//                                                                       //
	//=======================================================================//
	/// Połącz się asynchronicznie po UDP.
	void do_async_udp_connect();
	/// Prześlij identyfikator klienta serwerowi.
	void do_send_clientid_datagram();
	/// Wyślij keepalive'a.
	void do_send_keepalive_dgram();
	/// Odbierz wiadomość z gniazda UDP.
	void do_handle_udp_request();
	/// Wypisz zmiksowane dane na standardowe wyjście.
	void do_write_mixed_data_to_stdout(std::shared_ptr<std::string> data);
	/// Zarządzaj wiadomością na podstawie jej nagłówka.
	void do_manage_msg(base_header* header, std::string& body);
	/// Czytaj dane o określonej wielkości z STDIN.
	void do_read_from_stdin();
	/// Zapisz komunikat do gniazda UDP.
	void do_write_msg_to_udp_socket(std::shared_ptr<std::string> message);
	/// Podejmij próbę ponownego wysłania datagramu.
	void do_resend_last_datagram();
	/// Retransmituj datagramy o numerach większych niż 'nr'.
	void do_retransmit(size_t nr);
	//=======================================================================//
	// Zmienne.                                                              //
	//                                                                       //
	//=======================================================================//
	std::array<char, CLIENT_BUFFER_LEN> read_buf_;
	/// Gniazdo do łączenia się po UDP.
	boost::asio::ip::udp::socket udp_socket_;
	/// Resolver UDP.
	boost::asio::ip::udp::resolver udp_resolver_;
	/// Deskryptor wejścia.
	boost::asio::posix::stream_descriptor input_;
	/// Parametr powtarzalności keepalive'a.
	const size_t keepalive_period_;
	/// Współczynnik retransmisji.
	const size_t retransmit_limit_;
	/// Identyfikator klienta w komunikacji z serwerem.
	size_t clientid_; // może się zmienić w sytuacji utraty połączenia.
	/// Numer kolejnego dgramu(liczone od pierwszego przesłanego komunikatu).
	size_t nr_expected_;
	/// Numer ostatnio wysłanego datagramu przez klienta.
	size_t actual_dgram_nr_;
	/// Największy numer ostatnio otrzymanego datagramu od serwera.
	size_t nr_max_seen_;
	/// Liczba wolnych bajtów w FIFO.
	size_t win_;
	/// Tytuł ostatniego nagłówka.
	std::string last_header_title_;
	/// Ostatni datagram
	std::string last_datagram_;
	/// Zegarek do keepalive'ów.
	boost::asio::deadline_timer keepalive_timer_;
	/// Zegarek mierzący czas od ostatniego poprawnego połączenia po UDP.
	boost::asio::deadline_timer connection_timer_;
	/// Fabryka nagłówków.
	header_factory factory_;
	/// Bufor przejmujący dane z wejścia.
	char* input_buffer_;
};
#endif
