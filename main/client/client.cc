#include "client.hh"

client::client(
		boost::asio::io_service& io_service, 
		std::string& host, 
		std::string& port, 
		size_t retransmit_limit,
		size_t keepalive_period,
		size_t reconnect_period,
		size_t connection_period) 
	: 	
		reconnect_timer_(io_service),
		port_(port),
	  	address_(host),
		//=========================================//
		// TCP.                                    //
		//=========================================//
		tcp_socket_(io_service),
		tcp_resolver_(io_service),
		output_(io_service, ::dup(STDOUT_FILENO)),
	  	raport_buffer_(CLIENT_BUFFER_LEN),
	  	reconnect_period_(reconnect_period),
	  	connection_period_(connection_period),
	  	//=========================================//
	  	// UDP.                                    //
	  	//=========================================//
	  	udp_socket_(
	  		io_service,
	  		boost::asio::ip::udp::endpoint(
	  			boost::asio::ip::udp::v4(),
	  			0
	  		) 
	  	),
	  	udp_resolver_(io_service),
	  	input_(io_service, ::dup(STDIN_FILENO)),
	  	keepalive_period_(keepalive_period),
	  	retransmit_limit_(retransmit_limit),
	  	nr_expected_(0),
	  	actual_dgram_nr_(0),
	  	server_demand_ack_(0),
	  	nr_max_seen_(0),
	  	win_(CLIENT_BUFFER_LEN),
	  	last_header_title_(std::string("")),
	  	last_datagram_(std::string("")),
	  	keepalive_timer_(io_service),
	  	connection_timer_(io_service),
	  	factory_(),
	  	input_buffer_(CLIENT_BUFFER_LEN),
	  	buffer_(new char[CLIENT_BUFFER_LEN])
{
	//std::cerr << "Jestem w konstruktorze\n";
	//=======================================================================//
	// TCP.                                                                  //
	//=======================================================================//
	//std::cerr << "Jestem w konstruktorze\n";
	do_async_tcp_connect();
}

client::~client()
{
	// Zwolnij zasoby.
	free_resources();
	// Zamknij deskryptory.
	input_.close();
	output_.close();
}		
//===========================================================================//
//                                                                           //
// TCP.                                                                      //
//                                                                           //
//===========================================================================//
void client::handle_after_connect(const boost::system::error_code& error)
{
	if (!error) {
		do_read_header();
		//std::cerr << "Połączyłem się super\n";
	} else {
		std::cerr << "Handle TCP connection..\n";
		//std::cerr << "Try again: " << error << "\n";
		// Przystąp do wysyłania keepalive'ów:
		reconnect_timer_.expires_from_now(
			boost::posix_time::milliseconds(reconnect_period_)
		);
		reconnect_timer_.async_wait(
			[this](boost::system::error_code /*error*/) {
				// Niezależnie od rezultatu ponów połączenie.
				do_async_tcp_connect();
			}
		);
	}
}

void client::do_async_tcp_connect()
{
	//std::cerr << "A oto adres: " << address_ << "\n";
	boost::asio::ip::tcp::resolver::query query(address_, port_);
	boost::asio::ip::tcp::resolver::iterator tcp_endpoint = 
		tcp_resolver_.resolve(query);

	boost::asio::async_connect(
		tcp_socket_, 
		tcp_endpoint, 
		boost::bind(
			&client::handle_after_connect,
			this, 
			boost::asio::placeholders::error
		)
	);
}

void client::monitor_connection()
{
	connection_timer_.expires_from_now(
		boost::posix_time::milliseconds(1000)
	);

	connection_timer_.async_wait(
		[this](boost::system::error_code error) {
			// Niezależnie od rezultatu ponów połączenie.
			if (error == boost::asio::error::operation_aborted) {
				//std::cerr << "Jest dobrze!\n";
				monitor_connection();
			}
			else {
				std::cerr << "Nastąpiło przerwanie połączenia, bądź sekundowy brak reakcji ze strony serwera\n";
				do_tcp_reconnect();
			}
		}
	);
}

void client::do_read_header()
{
	//std::cerr << "Czytam nagłówek i co\n";
	
	boost::asio::async_read_until(
		tcp_socket_,
		raport_buffer_,
		'\n',
		[this](boost::system::error_code error, size_t bytes_transferred) {
			if (!error) {
				monitor_connection();
				//std::cerr << "Przeczytałem nagłówek\n";
				std::string header;
				std::istream is(&raport_buffer_);
				std::getline(is, header);
				//std::cerr << "Header: " << header << "\n";
				// Usuń dane z bufora
				raport_buffer_.consume(raport_buffer_.size());

				const int one = 49;
				const int nine = 57;
				const int header_begin = header[0];
				// Jeżeli nagłówek zaczyna się od cyfry nie-0 to znaczy,
				// że będziemy odczytywać raport.
				size_t bytes;
				if (header_begin >= one && header_begin <= nine) {
					try {
						bytes = boost::lexical_cast<size_t>(header);
						do_read_body(bytes);
					} catch (boost::bad_lexical_cast& ) {
						//std::cerr << "Bad lexical cast client.cc/do_read_header\n";
						//std::cerr << "Castowany: " << header << ", na: " << bytes << "\n";
					}
				}
				// W przeciwnym przypadku otrzymaliśmy od serwera 
				// komunikat z identyfikatorem .
				else {
					// Sprobuj skonwertować do odpowiedniego nagłówka.
					try {
						std::shared_ptr<client_header> c_header(
							dynamic_cast<client_header*>(
								factory_.match_header(
									headerline_parser::get_data(header)
								)
							)
	
						);
						clientid_ = c_header->_client_id;
						//std::cerr << "Przed połączeniem po UDP\n";
						do_async_udp_connect();
						do_read_header();
					} catch (invalid_header_exception& ex) {
						//std::cerr << "Invalid header format: " << ex.what() << "\n";
					}
				}
			} else {
				std::cerr << "Do read header..\n";
				// Nastąpiło przerwanie połączenia po TCP. Ponów!
				//std::cerr << "Do read header: " << error << "\n";
				//do_tcp_reconnect();
				//do_read_body();
			}
		}
	);
}

void client::do_read_body(const size_t bytes_to_be_transferred)
{
	//std::cerr << "Mam wczytać dokładnie: " << bytes_to_be_transferred << " bajtów.\n";
	boost::asio::async_read(
		tcp_socket_,
		raport_buffer_,
		boost::asio::transfer_at_least(bytes_to_be_transferred),
		[this](boost::system::error_code error, size_t bytes_transferred) {
			//std::cerr << "Wczytałem bajtów: " << bytes_transferred << "\n";
			if (!error) {
				/*std::istream is(&raport_buffer_);
				std::string msg;
				std::getline(is, msg);*/
				//std::cerr << "treść body: " << msg << "\n";
				do_write_to_STDOUT();
			} else {
				// Najprawdopodobniej nastąpiło przerwanie połączenia po TCP.
				// Wznawiamy.
				std::cerr << "Do read body: " << error << "\n";
				//do_tcp_reconnect(); Zegarek się o to zatroszczy.
			}
		}
	);
}

void client::do_write_to_STDOUT()
{
	std::string header;
	std::istream is(&raport_buffer_);
	std::getline(is, header);
	//std::cerr << "Rozmiar bufora z raportem: " << raport_buffer_.size() << "\n";
	std::cerr << boost::asio::buffer_cast<const char*>(raport_buffer_.data());
	//std::cerr << "Już nie żyję\n";
	/*int size = raport_buffer_.size();
	//std::shared_ptr<char*> data(boost::asio::buffer_cast<const char*>(raport_buffer_.data()));
	//raport_buffer_.consume(raport_buffer_.size());
	boost::asio::async_write(
		output_,
		boost::asio::buffer(raport_buffer_.data()),
		[this](boost::system::error_code error, size_t bytes_transferred) {
			if (!error) {
				raport_buffer_.consume(raport_buffer_.size());
				do_read_header();
			} else {
				// Nastąpił błąd w czasie wypisywania na strumień STDOUT.
				//std::cerr << "Do write to STDOUT: " << error << "\n";
			}
		}
	);*/
	raport_buffer_.consume(raport_buffer_.size());
	do_read_header();
}

void client::do_tcp_reconnect()
{
	// Ustaw długość kolejki na niezainicjalizowaną.
	win_ = CLIENT_BUFFER_LEN;
	// Zwlonij zasoby.
	udp_socket_.close();
	reconnect_timer_.expires_from_now(
		boost::posix_time::milliseconds(reconnect_period_)
	);
	reconnect_timer_.async_wait(
		[this](boost::system::error_code /*error*/) {
			// Niezależnie od rezultatu ponów połączenie.
			do_async_tcp_connect();
		}
	);
}

void client::free_resources()
{
	// Zwolnij miejsce w buforach.
	input_buffer_.consume(input_buffer_.size());
	// Cofnij wszystkie operacje związane z deskryptorami.
	input_.cancel();
	output_.cancel();
}


//===========================================================================//
//                                                                           //
// UDP.                                                                      //
//                                                                           //
//===========================================================================//
void client::do_async_udp_connect()
{
	// Znajdź endpoint do połączenia z serwerem po UDP.
	boost::asio::ip::udp::resolver::query udp_query(address_, port_);
	boost::asio::ip::udp::resolver::iterator endpoint_iterator =
		udp_resolver_.resolve(udp_query);
	/*boost::asio::ip::udp::endpoint remote_endpoint(
		boost::asio::ip::address::from_string(address_), 
		port_s
	);*/
	// Podejmij próbę połączenia z serwerem.
	udp_socket_.async_connect(
		*endpoint_iterator,
		[this](boost::system::error_code error) {

			if (!error) {
				do_send_clientid_datagram();
			} else {
				// Jeżeli nie udało się połączyć po UDP, 
				// to się nie poddajemy, tylko próbujemy usilnie się połączyć.
				std::cerr << "Do async udp connect: " << error << "\n";
				//do_async_udp_connect(); // po UDP łączymy się od razu.
			}
		}
	);
}

void client::do_send_clientid_datagram()
{
	//std::cerr << "Będę wysyłał id klienta do serwera.\n";
	std::string clientid_to_str = std::to_string(clientid_);
	// Skonstruuj poprawny nagłówek
	std::shared_ptr<std::string> header_s(
		new std::string("CLIENT " + clientid_to_str + "\n")
	);
	// Wyślij datagram z identyfikatorem do serwera.
	udp_socket_.async_send(
		boost::asio::buffer(*header_s),
		[this, header_s] (boost::system::error_code error, size_t bytes_transferred) {

			if (!error) {
				//std::cerr << "Wysłałem\n";
				do_handle_udp_request();
			} else {
				// Jeśli nie wypaliło to:
				// -> zwróć stosowną informację o błędzie,
				// -> spróbuj wysłać komunikat ponownie
				std::cerr << "Do send clientid datagram: " << error << "\n";
				do_send_clientid_datagram();
			}
		}
	);
	// Ponieważ jesteśmy podłączeni, to zacznij wysyłać KEEPALIVE'y.
	//do_send_keepalive_dgram();
}

void client::do_send_keepalive_dgram()
{
	// Skonstruuj poprawny datagram.
	std::shared_ptr<std::string> header_s(
		new std::string("KEEPALIVE\n")
	);
	// Wyślij powyższy datagram serwerowi.
	udp_socket_.async_send(
		boost::asio::buffer(*header_s),
		//remote_endpoint_,
		[this, header_s](boost::system::error_code error, size_t bytes_transferred) {

			if (!error) {
				// Świetnie! Komunikat został poprawnie wysłany.
				// Z tej okazji cieszymy się i nic nie robimy
				// ... przez najbliższe 100ms(kupa czasu).
			} else {
				// Jeżeli wystąpił błąd, to znaczy, 
				// że najprawdopodobniej połączenie po TCP się urwało
				// i w serwerze przestaliśmy słuchać tego klienta.
				// Podejmujemy więc próbę ponownego połączenia po TCP.
				std::cerr << "Send keepalive msg[Do send!]: " << error << "\n";
				//do_tcp_reconnect();
			}
		}
	);
	// Jeżęli komunikat został poprawnie wysłany,
	// to odmierz zegarkiem czas i ponownie wywołaj funkcję gdy nadejdzie czas.
	keepalive_timer_.expires_from_now(
		boost::posix_time::milliseconds(keepalive_period_)
	);
	keepalive_timer_.async_wait(
		[this](boost::system::error_code error) {

			if (!error) {
				do_send_keepalive_dgram();
			} else {
				// Jeśli nie udało się wysłać keepalive'a to znaczy,
				// że połączenie zostało przerwane i należy ponownie
				// połączyć się z serwerem po TCP.
				std::cerr << "Do send keepalive: " << error << "\n";
				//do_tcp_reconnect();
				monitor_connection();
			}
		}
	);
}

void client::do_handle_udp_request()
{
	// Odbieramy komunikat po wysłaniu inicjującego komunikatu postaci:
	// 
	// CLIENT [clientid]\n po UDP.
	udp_socket_.async_receive(
		boost::asio::buffer(read_buf_),
		[this](boost::system::error_code error, size_t bytes_transferred) {

			if (!error) {
				// Po odebraniu danych od serwera mamy jeden spójny datagram.
				// Jest on niepodzielony na nagłówek i ciało wiadomości.
				std::string msg;
				std::copy(
					read_buf_.begin(), 
					read_buf_.begin() + bytes_transferred,
					std::back_inserter(msg)
				);
				// Podziel wiadomość na nagłówek i część ,,body''.
				message_structure msg_structure = 
					message_converter::divide_msg_into_sections(
						msg, bytes_transferred
					);
				
				try {
					base_header* header = 
						factory_.match_header(
							headerline_parser::get_data(msg_structure._header)
						);

					//std::cerr << "Body.size(): " << msg_structure._body.size() << "\n";
					do_manage_msg(header, msg_structure._body);
				} catch (invalid_header_exception& e) {
					//std::cerr << "Do handle udp request: " << e.what() << "\n";
				}
			} else {
				// Jeżeli nie udało się odebrać danych z gniazda, 
				// to wróć do nasłuchiwania po UDP.
				std::cerr << "Do handle udp request\n";
				do_handle_udp_request();
			}
		}
	);
}

void client::do_write_mixed_data_to_stdout(std::shared_ptr<std::string> data)
{
	// Prześlij ,,body'' wiadomoścu na STDOUT.

	/*std::shared_ptr<std::string> data_ptr(data);
	std::cout << *data << "\n";
	do_read_from_stdin();
	do_handle_udp_request();*/
	boost::asio::async_write(
		output_,
		boost::asio::buffer(*data, data->size()),
		[this, data](boost::system::error_code error, size_t bytes_transferred) {
			
			// Niezależnie od powodzenia operacji rozpocznij:
			// -> czytanie z STDIN,
			// -> obsługę datagramów po UDP.
			if (!error) {
				//std::cerr << "bytes_transferred: " << bytes_transferred << "\n";
				do_read_from_stdin();
				do_handle_udp_request();
			} else {
				std::cerr << "Write mixed data to stdout..\n";
				//std::cerr << "Error\n";
			}
		}
	);
}

void client::do_manage_msg(base_header* header, std::string& body)
{
	//std::cerr << "Zarządzanie wiadomością\n";
	const std::string header_name = header->_header_name;
	if (header_name == DATA) {
		std::shared_ptr<data_header> d_header(
			dynamic_cast<data_header*>(header)
		);

		//if (d_header->_nr > nr_max_seen_) {
		/*std::cerr << "--------------------------------------------------------\n";
		std::cerr << "Datagram: DATA.\n";
		std::cerr << "Numer datagramu: " << d_header->_nr << "\n";
		std::cerr << "Oczekiwany pakiet ode mnie: " << d_header->_ack << "\n";
		std::cerr << "Rozmiar kolejki(win): " << d_header->_win << "\n";*/
		/*std::cerr << "Message: " << body << "\n"; 
		std::cerr << "\n";
		std::cerr << "Aktualnie ostatnio wysłany pakiet: " << actual_dgram_nr_ << "\n";
		std::cerr << "--------------------------------------------------------\n";*/
		//std::cerr << "Wiadomość: " << body << "\n";
		nr_max_seen_ = d_header->_nr;
		// Jeżeli otrzymaliśmy dwukrotnie datagram DATA
		// bez powtórzenia ostatnio wysłanego datagramu,
		// to ponawiamy wysyłanie ostatniego datagramu.
		std::shared_ptr<std::string> data_ptr(new std::string(body));
			
		if (last_header_title_ == DATA) {
	//		std::cerr << "Nie otrzymałem potwierdzenia poprzedniego UPLOADA\n";
			// Wyślij ostatni datagram UPLOAD raz jeszcze.
			if (!last_datagram_.empty()) {
				do_write_mixed_data_to_stdout(data_ptr);
				//do_resend_last_datagram();				
			}
		}
		// Jeżeli jest to pierwsza wiadomość od początku znajomości po UDP.
		if (win_ == CLIENT_BUFFER_LEN) {
	//		std::cerr << "Jest to pierwsza wiadomość od serwera\n";
			// Ustaw parametry klienta.
			win_ = d_header->_win + 1;
			nr_expected_ = d_header->_nr + 1;
			actual_dgram_nr_ = d_header->_ack;
			/*last_header_title_ = d_header->_header_name;
			last_datagram_ = "CLIENT " + std::to_string(clientid_) + ""*/
			// Wypisz ,,body'' wiadomości na STDOUT.
			do_write_mixed_data_to_stdout(data_ptr);
		}
		// Jeżeli klient otrzymał datagram z numerem większym
		// niż kolejny oczekiwany:
		else if (d_header->_nr > nr_expected_) {
	//		std::cerr << "Otrzymałem datagram z numerem większym niż kolejny oczekiwany.\n";
			if (nr_expected_ >= d_header->_nr - retransmit_limit_) {
				// Porzuć ten datagram i zaktualizuj wartość nr_expected_.
				//do_retransmit(nr_expected_); //póki co blocked TODO: Uruchomić to i przetestować.
				nr_expected_ = d_header->_nr + 1;
				win_ = d_header->_win;
				do_write_mixed_data_to_stdout(data_ptr);
			}
			// W przeciwnym przypadku przyjmij datagram oraz uznaj,
			// że poprzednich nie uda się już przeczytać.
			else {
				nr_expected_ = d_header->_nr + 1;
				win_ = d_header->_win;
				// Odczytaj maksymalnie win_ bajtów danych z STDIN
				do_write_mixed_data_to_stdout(data_ptr);
				//do_read_from_stdin();
				//do_handle_udp_request();
			}
		} else if (d_header->_nr < nr_expected_) {
	//		std::cerr << "Porzucam datagram\n";
			// Zwyczajnie porzuć ten datagram i czekaj na nowy.
			do_handle_udp_request();
		}
		// Jeżeli jest to któryś z rzędu komunikat od serwera typu DATA
		// oraz przekazany nr_datagramu jest zgodny z numerem oczekiwanym.
		else {
			// Zaktualizuj:
			// -> liczbę dostępnych bajtów w kolejce:
			win_ = d_header->_win;
			// -> numer kolejnego datagramu:
			nr_expected_++;
			// -> pozostałe:
			//server_demand_ack_ = d_header->_ack;
			last_header_title_ = d_header->_header_name;
			//last_datagram_ = last_header_title_ + body;
			// Wypisz rezultat na STDOUT.
			if (!last_datagram_.empty()) {
				//std::shared_ptr<std::string> data_ptr(new std::string(body));
				do_write_mixed_data_to_stdout(data_ptr);
			} else {
				do_write_mixed_data_to_stdout(data_ptr);
				//do_handle_udp_request();
			}
		}
		//nr_max_seen_ = std::max(nr_max_seen_, d_header->_nr);
	} else if (header_name == ACK) {
		//std::cerr << "Otrzymałem potwierdzenie.\n";
		std::shared_ptr<ack_header> a_header(
			dynamic_cast<ack_header*>(header)
		);
		/*std::cerr << "-----------------------------------------------\n";
		std::cerr << "Otrzymałem potwierdzenie ACK.\n";
		std::cerr << "Oczekiwany datagram ode mnie: " << a_header->_ack << "\n";
		std::cerr << "Rozmiar kolejki: " << a_header->_win << "\n";
		std::cerr << "\n";
		std::cerr << "Mój aktualny datagram: " << actual_dgram_nr_ << "\n";
		std::cerr << "-----------------------------------------------\n";*/
		// Jeżeli jest to pierwsza informacja zwrotna
		// od serwera po UDP w nowym połączeniu.
		if (win_ == CLIENT_BUFFER_LEN) {
			do_send_clientid_datagram();
			win_ = a_header->_win;
		} else {
			// Zaktualizuj:
			// -> tytuł ostatniego nagłówka:
			last_header_title_ = ACK;
			// -> ilość dostępnych bajtów w kolejce:
			win_ = a_header->_win;
			do_handle_udp_request();
			// Wczytaj nie więcej niż _win bajtów danych z STDIN
			//do_read_from_stdin();
		}
	}
}

void client::do_retransmit(const std::uint32_t nr)
{
	//std::cerr << "Małgorzata, ratatata.\n";
	std::string message(
		"RETRANSMIT " +
		std::to_string(std::max(nr_max_seen_, nr)) + 
		"\n"
	);

	udp_socket_.async_send(
		boost::asio::buffer(message),
		[this] (boost::system::error_code error, size_t bytes_transferred) {

			if (!error) {
				do_handle_udp_request();
			} else {
				std::cerr << "Error przy retransmicie: " << error << "\n";
				// Trudno.
			}
		}
	);
}

void client::do_read_from_stdin()
{
			//std::cerr << "Adsadas\n";
			if (win_ > 0) {
			//std::cerr << "win_: --------------------------------------------------------------------------------: " << win_ << "\n";
	input_.async_read_some(
			boost::asio::buffer(buffer_, std::min(win_, CLIENT_BUFFER_LEN)),
			//boost::asio::transfer_at_least(1),
			[this](boost::system::error_code error, size_t bytes_transferred) {

				if (!error) {
	//				std::cerr << "Wczytałem coś\n";
					//win_ -= bytes_transferred;
					// Wczytaj dane z bufora do stringa.
					/*std::istream is(&input_buffer_);
					std::string s_dgram;
					is >> s_dgram;*/
					//buffer_[bytes_transferred] = '\0;
					std::string s_dgram(buffer_, bytes_transferred);//std::begin(buffer_), std::begin(buffer_) + bytes_transferred);

					//std::cerr << "Wczytałem " << bytes_transferred << " bajtów, rozmiar kolejki(win): " << win_ << "\n";
					// Zuploaduj dane otrzymane z STDIN, ale zanim to zrobisz:
					// -> zaktualizuj datagram jako ostatnio nadany:
					std::string header(
						"UPLOAD " + 
						std::to_string(actual_dgram_nr_) +
						"\n"
					);
					// -> zaktualizuj aktualny numer datagramu do wysłania:
					actual_dgram_nr_++;
					last_datagram_ = header + s_dgram;

					input_buffer_.consume(input_buffer_.size());
					std::shared_ptr<std::string> message(
						new std::string(header + s_dgram)
					);
					// Zapisz do udp socket'a.
					do_write_msg_to_udp_socket(message);
				} else {
					//std::cerr << "Do read from STDIN: " << error << "\n";
				}
			}
		);
		} else {
			do_handle_udp_request();
		}
	//	std::cerr << "Jestem w czytelni z wejścia\n";
		// Zarezerwuj miejsce na win_ bajtów z STDIN.
		//boost::asio::streambuf::mutable_buffers_type bufs = 
		//	input_buffer_.prepare(win_);
		// Wczytaj nie więcej niż win_ bajtów danych z STDIN.
		
	/*} else {
		do_handle_udp_request();
	}*/
}

void client::do_write_msg_to_udp_socket(std::shared_ptr<std::string> message)
{
	//std::cerr << "Taka SYTUACJA: " << *message << "\n";
	udp_socket_.async_send(
		boost::asio::buffer(*message, CLIENT_BUFFER_LEN),
		[this, message](
			boost::system::error_code error, 
			size_t bytes_transferred
		) {
			if (error) {
				// Jeżeli zapisanie do socketa się nie powiodło,
				// to spróbuj raz jeszcze.
				std::cerr << "Do write msg to udp socket: " << error << "\n";
				//do_write_msg_to_udp_socket(message);
			}
			// Niezależnie od tego nasłuchuj.
			do_handle_udp_request();
		}
	);
}

void client::do_resend_last_datagram()
{
	/*std::cerr << "-------------------------------------------\n";
	std::cerr << "Ponawiam wysłanie ostatniego datagramu\n";
	std::cerr << "                                           \n";
	std::cerr << last_datagram_ << "\n";
	std::cerr << "-------------------------------------------\n";*/
	std::shared_ptr<std::string> resend_message(
		new std::string(last_datagram_)
	);
	// Wyślij ponownie
	udp_socket_.async_send(
		boost::asio::buffer(*resend_message),
		[this](
			boost::system::error_code error, 
			size_t bytes_transferred
		) {
			if (error) {
				std::cerr << "Do resend last datagram..\n";
			}
			// Niezależnie od pomyślności ponownego przesłania wiadomości:
			// -> czytaj ze standardowego wejścia.
			//do_read_from_stdin();
			// -> Jednocześnie czekawszy na wiadomość od serwera.
			//do_handle_udp_request();
		}
	);
}
