#ifndef SERVER_HH
#define SERVER_HH

// Biblioteki STL-owe i standardowe i C-owa:
#include <array>
#include <cmath>
#include <cstdint> // uint*_t
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator> // std::back_inserter?
#include <map>
#include <signal.h> // signals
#include <sstream> // std::ostringstream, std::istream
#include <string>
#include <utility> // std::pair

// Biblioteki boost'owe:
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

// Własne biblioteki/struktury:
// 1) Niezbędne do połączeń po TCP:
#include "../connection/connection.hh"
#include "../connection_manager/connection_manager.hh"
// 2) Struktury:
#include "../client_data_on_server/client_data.hh"
#include "../common/header_titles.hh"
#include "../common/structures.hh"
#include "../common/global_variables.hh"
// 3) Parsery:
#include "../message_converter/message_converter.hh"
#include "../common_parsers/headerline_parser.hh"
#include "../string_converter/string_converter.hh"
// 4) Fabryki:
#include "../factories/header_factory.hh"
// 5) Mikser:
#include "../mixer/mixer.hh"
// 6) Wyjątki:
#include "../exceptions/invalid_header_exception.hh"

class connection_manager;

class server
{
public:
	//=======================================================================//
	//                                                                       //
	// Tworzenie/niszczenie obiektu i rozrusznik IO-SERVICE                  //
	//                                                                       //
	//=======================================================================//
	/// Konstruktor serwera.
	explicit server(
		std::string& port,
		size_t fifo_size,
		size_t fifo_low_watermark,
		size_t fifo_high_watermark,
		size_t buf_len,
		size_t tx_interval
	);
	/// Destruktor.
	~server();
	/// Blokady możliwości kopiowania obiektu serwera.
	server(const server&) = delete;
	server& operator=(const server&) = delete;
	/// Odpal pętlę io_service.
	void run();
	/// Zwolnij zasoby dla wskazanego klienta.
	void free_resources(const size_t clientid);
private:
	/// IO-service umożliwiający wykonywanie asynchronicznych operacji.
	boost::asio::io_service io_service_;
	/// Sygnały rejestrujące zakończenie działania procesu.
	boost::asio::signal_set signals_;

	/// Czekaj na sygnał kończący pracę servera.
	void do_await_stop();


	//=======================================================================//
	//                                                                       //
	// TCP.                                                                  //
	//                                                                       //
	//=======================================================================//
	
	//=======================================================================//
	// Metody.                                                               //
	//=======================================================================//
	/// Wykonaj asynchroniczną operację akceptowania połączeń po TCP.
	void do_accept();
	/// Identyfikator dla kolejnego klienta.
	size_t get_next_clientid();
	/// Wyślij raport do wszystkich klientów po TCP.
	void send_raport();
	/// Odpal czasowstrzymywacz na wysyłanie raportów.
	void run_raport_timer();

	//=======================================================================//
	// Zmienne.                                                              //
	//=======================================================================//
	/// Akceptor używany do nasłuchiwania na nadchodzące połączenia TCP.
	boost::asio::ip::tcp::acceptor acceptor_;
	/// Manager połączeń TCP.
	connection_manager* connection_manager_;
	/// Kolejne gniazdo TCP do zaakceptowania.
	boost::asio::ip::tcp::socket tcp_socket_;
	/// Identyfikator klienta.
	size_t clientid_;
	/// Tekst raportu.
	std::string raport_;
	/// Zegarek na raporty.
	boost::asio::deadline_timer raport_timer_;
	/// Okres wywoływania zegarka raportowego.
	const size_t raport_timer_period_;


	//=======================================================================//
	//                                                                       //
	// UDP.                                                                  //
	//                                                                       //
	//=======================================================================//

	//=======================================================================//
	// Metody.                                                               //
	//                                                                       //
	//=======================================================================//
	/// Odbierz wiadomość od klienta.
	void do_receive();
	/// Zarządzaj wiadomością klienta w zależności od nagłówka.
	void do_manage_msg(base_header* header, std::string& body);
	/// Transmituj dane na życzenie klienta.
	void do_transmit_data(
		std::shared_ptr<boost::asio::ip::udp::endpoint> remote_endpoint,
		std::list<std::string>& dgram_list);
	/// Konwertuj endpoint na human-readable form.
	std::string convert_remote_tcp_endpoint_to_string(
		boost::asio::ip::tcp::endpoint remote_tcp_endpoint);
	/// Wymieszaj dane pochodzące od klientów.
	void mixer();
	/// Odpal zegarek do miksowania.
	void do_mixery();
	/// Prześlij potwierdzenie konkretnemu klientowi.
	void do_send_ack_datagram(const size_t ack, const size_t free_space,
		boost::asio::ip::udp::endpoint remote_endpoint);
	/// Odpal zegarek na datagramy od konkretnego klienta.
	void run_udp_dgram_timer(
		std::shared_ptr<boost::asio::ip::udp::endpoint> client_endpt);

	//=======================================================================//
	// Zmienne.                                                              //
	//                                                                       //
	//=======================================================================//	
	/// Gniazdo do komunikacji po UDP.
	boost::asio::ip::udp::socket udp_socket_;
	/// Resolver do UDP.
	boost::asio::ip::udp::resolver udp_resolver_;
	/// Numer aktualnie wysyłanego datagramu.
	size_t current_datagram_nr_;
	/// Port.
	const std::string port_;
	/// Parametry kolejki:
	const size_t fifo_size_;
	const size_t fifo_low_watermark_;
	const size_t fifo_high_watermark_;
	/// Długość kolejki zapamiętanych datagramów.
	const size_t buf_len_;
	/// Parametr częstości uruchamiania miksera.
	const size_t tx_interval_;
	/// Mikserowy zegarek.
	boost::asio::deadline_timer mixer_timer_;
	/// Bufor do odczytu danych po UDP.
	std::array<char, CLIENT_BUFFER_LEN> read_buf_;
	/// Endpoint ostatniego klienta połączonego po UDP.
	boost::asio::ip::udp::endpoint sender_endpoint_;
	/// Fabryka nagłówków.
	header_factory factory_;
	/// Bufor serwera przechowujący zmiksowane dane.
	char* write_buf_;
	/// Okres wywoływania zegarka odmierzającego czas
	/// od ostatniego dgramu nadanego przez konkretnego klienta.
	const size_t udp_dgram_timers_period_;

	//=======================================================================//
	// Mapy.                                                                 //
	//=======================================================================//
	/// Mapa endpointów UDP.
	// [klucz]: endpoint UDP klienta
	// [wartość]: identyfikator przypisany konkretnemu klientowi
	std::map<boost::asio::ip::udp::endpoint, size_t> client_map_;
	/// Mapa ze strukturami przypisanymi konkretnym klientom.
	// [klucz]: identyfikator klienta
	// [wartość]: struktura client_data przechowująca:
	// -> kolejkę(bufor) klienta,
	// -> endpoint UDP w postaci stringa,
	// -> endpoint w postaci domyślnej,
	// -> statystyki:
	//    + minimalną liczbę bajtów w kolejce(od ostatniego raportu)
	//    + maksymalną liczbę bajtów w kolejce(od ostatniego raportu)
	//    + aktualny rozmiar kolejki
	// -> listę ostatnich datagramów przesłanych przez klienta
	std::map<size_t, client_data*> client_data_map_;
	/// Mapa zegarków dla dgramów przychodzących po UDP.
	// [klucz]: identyfikator klienta
	// [wartość]: zegarek odmierzający czas od ostatnio nadanego dgramu.
	std::map<size_t, boost::asio::deadline_timer*> udp_dgram_timers_;
};
#endif
