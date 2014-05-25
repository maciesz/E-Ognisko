#include "client.hpp"
 
 client::client(boost::asio::io_service& io_service, const std::string& port, 
 	const std::string& server_name, boost::uint16_t retransmit_limit,
 	boost::uint16_t keepalive_period) 
 : 
	tcp_socket_(io_service), 
	udp_socket_(io_service),
	tcp_resolver_(io_service), 
	udp_resolver_(io_service),
	input_(io_service, ::dup(STDIN_FILENO)), 
	output_(io_service, ::dup(STDOUT_FILENO)), 
	header_buffer_(CLIENT_BUFFER_LEN),
	body_buffer_(CLIENT_BUFFER_LEN),
	raport_buf_(CLIENT_BUFFER_LEN),
	//read_buf_(CLIENT_BUFFER_LEN), 
	//write_buf_(CLIENT_BUFFER_LEN),
	input_buffer_(CLIENT_BUFFER_LEN),
	output_buffer_(CLIENT_BUFFER_LEN),
	port_(port), 
	server_name_(server_name),
	keepalive_period_(keepalive_period),
	retransmit_limit_(retransmit_limit),
	nr_expected_(static_cast<boost::int32_t>(-1)),
	actual_dgram_nr_(static_cast<boost::int32_t>(-1)),
	server_demand_ack_(static_cast<boost::int32_t>(0)),
	win_(static_cast<boost::int32_t>(-1)),
	factory_(),
	last_header_title_(std::string("")),
	last_datagram_(std::string("")),
	rerun_timer_(io_service),
	keepalive_timer_(io_service),
	header_str_(std::string("")),
	body_str_(std::string(""))
{
	// Znajdź endpoint do połączenia z serwerem po TCP
	boost::asio::ip::tcp::resolver::query tcp_query_(server_name, port);
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = 
		tcp_resolver_.resolve(tcp_query_);
	// Podejmij próbę połączenia z serwerem po TCP
	boost::asio::async_connect(tcp_socket_, endpoint_iterator,
		boost::bind(&client::handle_tcp_connect, this,
			boost::asio::placeholders::error));
}

client::~client()
{
}

void client::handle_tcp_connect(const boost::system::error_code& error)
{
	if (!error) {
		// Wczytaj dane do streambuffa
		boost::asio::async_read_until(tcp_socket_, header_buffer_, '\n',
			boost::bind(&client::handle_client_id, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle tcp connect\n";
	}
}

void client::handle_client_id(const boost::system::error_code& error)
{
	if (!error) {
		// Wczytaj dane z bufora do stringa
		std::istream is(&header_buffer_);
		std::string s_header;
		is >> s_header;
		try {
			boost::shared_ptr<client_header> header(
				dynamic_cast<client_header*>(
				factory_.match_header(headerline_parser::get_data(s_header))
			));
			
			header->_client_id;
		} catch (invalid_header_exception& ex) {
			std::cerr << "Invalid header format: client id from server\n";
			//close();
			return;
		}
		// Wyczyść bufor:
		const size_t header_buffer_size = header_buffer_.size();
		header_buffer_.consume(header_buffer_size);

		boost::asio::async_read(tcp_socket_, raport_buf_,
			boost::bind(&client::handle_write_tcp_raport, this,
				boost::asio::placeholders::error));

		create_udp_socket(s_header.size());
	} else {
		std::cerr << "Handle client id\n";
	}
}

void client::handle_write_tcp_raport(const boost::system::error_code& error)
{
	if (!error) {
		// Wypisz na standardowe wyjście zawartość bufora z raportami:
		boost::asio::async_write(output_, raport_buf_, 
			boost::bind(&client::handle_read_tcp_raport, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle write TCP raport\n";
	}
}

void client::handle_read_tcp_raport(const boost::system::error_code& error)
{
	if (!error) {
		// Zwolnij miejsce w buforze raportów po ostatnim transferze
		const size_t raport_buf_size = raport_buf_.size();
		raport_buf_.consume(raport_buf_size);
		boost::asio::async_read(tcp_socket_, raport_buf_,
			boost::bind(&client::handle_write_tcp_raport, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle read TCP raport\n";
	}
}

void client::create_udp_socket(const int bytes_transferred)
{
	// Znajdź endpoint do połączenia z serwerem po UDP
	boost::asio::ip::udp::resolver::query udp_query_(server_name_, port_);
	boost::asio::ip::udp::resolver::iterator endpoint_iterator = 
		udp_resolver_.resolve(udp_query_);

	// Podejmij próbę połączenia z serwerem po UDP
	boost::asio::async_connect(udp_socket_, endpoint_iterator,
		boost::bind(&client::handle_udp_connect, this,
			boost::asio::placeholders::error));
}

void client::handle_udp_connect(const boost::system::error_code& error)
{
	if (!error) {
		std::string client_id_to_str =
		boost::lexical_cast<std::string>(client_id_);		
		// Skonstruuj poprawny nagłówek
		boost::shared_ptr<std::string> header_s(
			new std::string("CLIENT " + client_id_to_str + "\n")
		);

		udp_socket_.async_send( 
			boost::asio::buffer(*header_s), //client_request,
			boost::bind(&client::handle_client_dgram, this,
				boost::asio::placeholders::error));

		// Przystąp do wysyłania keepalive'ów:
		keepalive_timer_.expires_from_now(
			boost::posix_time::milliseconds(keepalive_period_)
		);
		keepalive_timer_.async_wait(
			boost::bind(&client::send_keepalive_dgram, this)
		);

	} else {
		std::cerr << "Handle udp connect\n";
	}
}

void client::send_keepalive_dgram()
{
	// Skonstruuj poprawny nagłówek
	boost::shared_ptr<std::string> header_s(
		new std::string("KEEPALIVE\n")
	);
	// Zapamiętaj długość wiadomości:
	//const int header_s_size = header_s.size();
	/*// Stwórz strumień i zainicjuj go wskazanym stringiem
	std::stringstream binarydata_buf(header_s);
	// Zapisz dane do streambufa
	boost::asio::streambuf keepalive_request;
	std::ostream keepalive_stream(&keepalive_request);
	keepalive_stream.write(&binarydata_buf, sizeof(binarydata_buf));*/

	udp_socket_.async_send(
		boost::asio::buffer(*header_s),
		boost::bind(&client::wait_for_next_keepalive, this,
			boost::asio::placeholders::error));
}

void client::wait_for_next_keepalive(const boost::system::error_code& error)
{
	if (!error) {
		keepalive_timer_.expires_from_now(
				boost::posix_time::milliseconds(keepalive_period_)
		);
		keepalive_timer_.async_wait(
			boost::bind(&client::send_keepalive_dgram, this)
		);
	} else {
		std::cerr << "Wait for next keepalive\n";
	}
}

void client::handle_client_dgram(const boost::system::error_code& error)
{
	if (!error) {
		// Odbierz komunikat po wysłaniu inicjującego komunikatu:
		//
		// CLIENT [clientid] po UDP
		udp_socket_.async_receive(
			boost::asio::buffer(read_buf_), //'\n', // TODO: Możliwe, że nie jest to odpowiednik async_read_until
			boost::bind(&client::handle_read_datagram_header, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle client dgram\n";
	}
}

void client::handle_read_datagram_header(const boost::system::error_code& error, // TODO: kandydat na rename'a
	const size_t bytes_transferred)
{
	if (!error) {
		// Po odebraniu danych od serwera mamy jeden komunikat,
		// niepodzielony na nagłówek i ciało wiadomości:
		std::string whole_message;
		std::copy(read_buf_.begin(), read_buf_.begin() + bytes_transferred, 
			std::back_inserter(whole_message));
		
		const message_structure msg_structure = 
			message_converter::divide_msg_into_sections(message);
			
		try {
			base_header* header = 
				factory_.match_header(headerline_parser::get_data(
					msg_structure.header
					)
				);
			manage_read_header(header);
		} catch (invalid_header_exception& ex) {
			std::cerr << "Handle read datagram\n";
		}
	} else {
		std::cerr << "Handle read datagram\n";
	}
}

void client::handle_write_to_stdout()
{
	// Prześlij wiadomość z body_buffer na STDOUT
	boost::asio::async_write(output_, 
		boost::asio::buffer(body_str_), // TODO!: Czytasz z bufora po staremu aj ty ty!
		boost::bind(&client::handle_read_from_stdin, this,
			boost::asio::placeholders::error));
}

void client::manage_read_header(base_header* header)
{
	if (header->_header_name == DATA) {
			boost::shared_ptr<data_header> d_header(
				dynamic_cast<data_header*>(header)
			);

			// Jeżeli jest to pierwsza wiadomość od serwera
			if (win_ == -1) {
				// Ustaw parametry
				win_ = d_header->_win;
				nr_expected_= d_header->_nr + 1;
				server_demand_ack_ = d_header->_ack;
				last_header_title_ = d_header->_header_name;

				// Wypisz body wiadomości na STDOUT
				//std::cout << body_str_;
				handle_write_to_stdout();
				/*udp_socket_.async_receive(
					boost::asio::buffer(body_buffer_),
					boost::bind(&client::handle_write_to_stdout, this,
						boost::asio::placeholders::error));*/
			}
			// Jeżeli otrzymaliśmy dwukrotnie datagram DATA,
			// bez potwierdzenia ostatnio wysłanego datagramu,
			// to ponawiamy wysyłanie ostatniego datagramu 
			if (last_header_title_ == DATA) {
				// Wyślij ostatni datagram raz jeszcze
				resend_last_datagram();
			}
			// Jeżeli klient otrzyma datagram z numerem większym
			// niż kolejny oczekiwany:
			else if (d_header->_nr > nr_expected_) {
				if (nr_expected_ >= d_header->_nr - retransmit_limit_) {
					// Porzuca ten datagram i wysyła w odpowiedzi 
					nr_expected_ = d_header->_nr + 1;
					// datagram RETRANSMIT nr_expected_
				}
				// W przeciwnym wypadku przyjmuje datagram oraz uznaje,
				// że poprzednich już nie przeczyta
				else {
					nr_expected_ = d_header->_nr;
					win_ = d_header->_win;
					// Zarezerwuj miejsce na win bytów z STDIN:
					boost::asio::streambuf::mutable_buffers_type bufs =
						input_buffer_.prepare(win_);
					// Wczytaj nie więcej niż _win bajtów danych z STDIN
					boost::asio::async_read(input_, bufs,
						boost::bind(&client::handle_read_from_stdin, this,
							boost::asio::placeholders::error));
				}
			} else if (d_header->_nr < nr_expected_) {
				// Zwyczajnie porzuć ten datagram i tyle.
			} 
			// Jeżeli jest to któryś z rzędu komunikat od serwera typu DATA
			// oraz przekazany nr_datagramu jest zgodny z numerem oczekiwanego
			else {
				// Zaktualizuj:
				// -> liczbę dostępnych bajtów w kolejce:
				win_ = d_header->_win;
				// -> numer kolejnego datagramu
				nr_expected_++;
				// Zaktualizuj n
				server_demand_ack_ = d_header->_ack;
				last_header_title_ = d_header->_header_name;

				handle_write_to_stdout();
			}
	}	else if (header->_header_name == ACK) {
			boost::shared_ptr<ack_header> a_header(
				dynamic_cast<ack_header*>(header)
			);
			// Jeżeli jest to pierwsza informacja zwrotna 
			// od serwera po UDP w nowym połączeniu
			if (win_ == -1) {
				// Ponów wysłanie client_id
				std::string client_id_to_str =
				boost::lexical_cast<std::string>(client_id_);		
				// Skonstruuj poprawny nagłówek
				boost::shared_ptr<std::string> header_s(
					new std::string("CLIENT " + client_id_to_str + "\n")
				);
				// Zapamiętaj długość nagłówka:
				//const int header_s_size = header_s.size();
				/*// Stwórz strumień i zainicjuj go wskazanym stringiem
				std::stringstream binarydata_buf(header_s);
				// Zapisz dane do streambufa
				boost::asio::streambuf client_request;
				std::ostream client_stream(&client_request);
				client_stream.write(&binarydata_buf, sizeof(binarydata_buf));*/
				// Wyślij komunikat inicjujący
				udp_socket_.async_send(
					boost::asio::buffer(*header_s),
					boost::bind(&client::handle_client_dgram, this,
						boost::asio::placeholders::error));
			} else {
				// Zaktualizuj:
				//
				// -> tytuł ostatniego nagłówka
				last_header_title_ = ACK;
				// -> ilość dostępnych bajtów w kolejce:
				win_ = a_header->_win;
				// Wczytaj nie więcej niż _win bajtów danych z STDIN
				boost::asio::streambuf::mutable_buffers_type bufs =
						input_buffer_.prepare(win_);
				boost::asio::async_read(input_, bufs,
					boost::bind(&client::handle_read_from_stdin, this,
						boost::asio::placeholders::error));
			} 
	}
}

void client::handle_read_from_stdin(const boost::system::error_code& error)
{
	if (!error) {
		// Zwolnij bufor ciała komunikatu od serwera:
		//const size_t body_buffer_size = body_buffer_.size();
		//body_buffer_.consume(body_buffer_size);
		// Wczytaj dane z bufora do stringa
		std::istream is(&input_buffer_);
		std::string s_dgram;
		is >> s_dgram;
		// Zuploaduj dane otrzymane z STDIN, ale zanim to zrobisz:
		// -> zaktualizuj aktualny numer datagramu do wysłania:
		actual_dgram_nr_++;
		// -> zaktualizuj datagram jako ostatnio nadany:
		last_datagram_ = 
			"UPLOAD " + 
			boost::lexical_cast<std::string>(nr_expected_) +
			" " +
			boost::lexical_cast<std::string>(actual_dgram_nr_) +
			" " +
			boost::lexical_cast<std::string>(win_) +
			"\n" +
			s_dgram;

		boost::shared_ptr<std::string> message(
			new std::string(last_datagram_)
		);
		// Zapamiętaj rozmiar komunikatu:
		//const int last_datagram_size = last_datagram_.size();
		/*
		// Stwórz strumień i zainicjuj go wskazanym stringiem
		std::stringstream binarydata_buf(last_datagram_);
		// Zapisz dane do streambufa
		boost::asio::streambuf write_request;
		std::ostream write_stream(&write_request);
		write_stream.write(&binarydata_buf, sizeof(binarydata_buf));
		// 
		*/
		// Zapisz do udp_socket'a
		udp_socket_.async_send(
			boost::asio::buffer(*message),
			boost::bind(&client::handle_after_read_from_stdin, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle read from STDIN\n";
	}
}

void client::handle_after_read_from_stdin(const boost::system::error_code& error)
{
	if (!error) {
		// Czekaj na komunikat zwrotny od serwera
		// (ACK albo [nie daj Boże] na DATA bez uprzedniego ACK)
		udp_socket_.async_receive(
			boost::asio::buffer(read_buf_), //'\n', // TODO: Możliwe, że źle czytane
			boost::bind(&client::handle_read_datagram_header, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle upload datagram\n";
	}
}

void client::resend_last_datagram()
{
/*
	// Stwórz strumień i zainicjuj go wskazanym stringiem
	std::stringstream binarydata_buf(last_datagram_);
	// Zapisz dane do streambufa
	boost::asio::streambuf resend_request;
	std::ostream resend_stream(&resend_request);
	resend_stream.write(&binarydata_buf, sizeof(binarydata_buf));
	*/
	//const int last_datagram_size = last_datagram_.size();
	boost::shared_ptr<std::string> resend_message (
		new std::string(last_datagram_)
	);
	// Wyślij ponownie:
	udp_socket_.async_send(
		boost::asio::buffer(*resend_message),
		boost::bind(&client::handle_after_read_from_stdin, this,
			boost::asio::placeholders::error));
}
