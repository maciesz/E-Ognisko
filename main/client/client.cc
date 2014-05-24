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
	header_buffer_(new char[CLIENT_BUFFER_LEN]),
	body_buffer_(new char[CLIENT_BUFFER_LEN]),
	raport_buf_(new char[CLIENT_BUFFER_LEN]),
	read_buf_(new char[CLIENT_BUFFER_LEN]), 
	write_buf_(new char[CLIENT_BUFFER_LEN]),
	input_buffer_(new char[CLIENT_BUFFER_LEN]),
	output_buffer_(new char[CLIENT_BUFFER_LEN]),
	port_(port), 
	server_name_(server_name),
	keepalive_period_(keepalive_period),
	retransmit_limit_(retransmit_limit),
	nr_expected_(static_cast<boost::int32_t>(-1)),
	actual_dgram_nr_(static_cast<boost::int32_t>(-1)),
	win_(static_cast<boost::int32_t>(-1)),
	factory_(),
	last_header_title_(std::string("")),
	last_datagram_(std::string("")),
	rerun_timer_(io_service),
	keepalive_timer_(io_service)
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

void client::handle_tcp_connect(const boost::system::error_code& error)
{
	if (!error) {
		boost::asio::read_until(tcp_socket_, boost::asio::buffer(header_buffer_, CLIENT_BUFFER_LEN), '\n',
			boost::bind(&client::handle_client_id, this,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle tcp connect\n";
	}
}

void client::handle_client_id(const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	if (!error) {
		std::string s_header(header_buffer_, bytes_transferred);
		try {
			boost::shared_ptr<client_header> header( 
				factory_.match_header(headerline_parser::get_data(s_header))
			);
			
			header->_client_id;
		} catch (invalid_header_exception& ex) {
			std::cerr << "Invalid header format: client id from server\n";
			//close();
			return;
		}
		boost::asio::async_read(tcp_socket_, 
			boost::asio::buffer(raport_buf_, CLIENT_BUFFER_LEN),
			boost::bind(&client::handle_write_tcp_raport, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		create_udp_socket(s_header.size());
	} else {
		std::cerr << "Handle client id\n";
	}
}

void client::handle_write_tcp_raport(const boost::system::error_code& error,
	const size_t bytes_transferred)
{
	if (!error) {
		boost::asio::async_write(output_, 
			boost::asio::buffer(raport_buf_, bytes_transferred),
			boost::bind(&client::handle_read_tcp_raport, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle write TCP raport\n";
	}
}

void client::handle_read_tcp_raport(const boost::system::error_code& error)
{
	if (!error) {
		boost::asio::async_read(tcp_socket_,
			boost::asio::buffer(raport_buf_, CLIENT_BUFFER_LEN),
			boost::bind(&client::handle_write_tcp_raport, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
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
			boost::asio::placeholders::error, bytes_transferred));
}

void client::handle_udp_connect(const boost::system::error_code& error, 
	const int bytes_transferred)
{
	if (!error) {
		std::string client_id_to_str =
		boost::lexical_cast<std::string>(client_id_);

		boost::shared_array<char> client_id_header(new char[
			6 /* Dł. tytułu komunikatu */ + 
			1 /* Odstęp */ +
			client_id_to_str.size() /* Dł. id klienta */ +
			1 /* Znak nowej linii */
		]);

		std::string header_s("CLIENT " + client_id_to_str + "\n");
		client_id_header = header_s.c_str();

		// Wyślij komunikat inicjujący
		boost::asio::async_write(udp_socket_, client_id_header
			boost::bind(&client::handle_client_dgram, this,
				boost::asio::placeholders::error));

		// Przystąp do wysyłania keepalive'ów:
		wait_for_next_keepalive(nullptr);

	} else {
		std::cerr << "Handle udp connect\n";
	}
}

void client::send_keepalive_dgram()
{
	boost::shared_array<char> keepalive(new char[
		9 /* Długość tytułu keepalive'a */ +
		1 /* Znak nowej linii */
	]);
	boost::asio::async_write(udp_socket_, keepalive,
		boost::bind(&client::wait_for_next_keepalive, this,
			boost::asio::placeholders::error_code));
}

void client::wait_for_next_keepalive(const boost::system::error_code& error)
{
	if (!error) {
		keepalive_timer_.expires_from_now(
				boost::posix_time::milliseconds(keepalive_period_)
		);
		keepalive_period_.async_wait(
			boost::bind(&client::send_keepalive_dgram, this)
		);
	} else {
		std::cerr << "Wait for next keepalive\n";
	}
}

void client::handle_client_dgram(const boost::system::error_code& error)
{
	if (!error) {
		boost::asio::async_read_until(udp_socket_, header_buffer_, '\n',
			boost::bind(&client::handle_read_datagram_header, this,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle client dgram\n";
	}
}

void client::handle_read_datagram_header(const boost::system::error_code& error, 
	const size_t bytes_transferred)
{
	if (!error) {
		const std::string& header_s(header_buffer_);//, bytes_transferred);
		try {
			boost::shared_ptr<base_header> header(
				factory_.match_header(headerline_parser::get_data(header_s)));
			manage_read_header(header);
		} catch (invalid_header_exception& ex) {
			std::cerr << "Handle read datagram\n";
		}
	} else {
		std::cerr << "Handle read datagram\n";
	}
}

void client::handle_write_to_stdout(const boost::system::error_code& error,
	const size_t bytes_transferred)
{
	if (!error) {
		// Prześlij wiadomość z body_buffer na STDOUT
		boost::asio::async_write(output_, 
			boost::asio::buffer(body_buffer_, bytes_transferred),
			boost::bind(&client::handle_read_from_stdin, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		// Po wszystkim przejdź do obsługi upload'u
	} else {
		std::cerr << "Handle read upload body\n";
	}
}

void client::manage_read_header(boost::shared_ptr<base_header> header)
{
	switch (header->_header_name) {
		case DATA:
			boost::shared_ptr<data_header> d_header(
				dynamic_cast<boost::shared_ptr<data_header>>(header)
			);

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
				if (nr_expected_ >= d_header - retransmit_limit_) {
					// Porzuca ten datagram i wysyła w odpowiedzi 
					nr_expected_ = d_header->_nr;
					// datagram RETRANSMIT nr_expected_
				}
				// W przeciwnym wypadku przyjmuje datagram oraz uznaje,
				// że poprzednich już nie przeczyta
				else {
					nr_expected_ = d_header->_nr;
					win_ = d_header->_win;
					// Wczytaj nie więcej niż _win bajtów danych z STDIN
					boost::asio::async_read(input_, 
						boost::asio::buffer(input_buffer_, win_),
						boost::bind(&client::handle_read_from_stdin, this,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));
				}
			} 
			// Jeżeli jest to pierwsza wiadomość od serwera
			else {
				// Ustaw parametry
				win_ = d_header->_win;
				nr_expected_= d_header->_nr;
				actual_dgram_nr_ = d_header->_ack;
				last_header_title_ = d_header->_header_name;

				boost::asio::async_read(udp_socket_, body_buffer_,
					boost::bind(&client::handle_write_to_stdout, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}

			break;
		case ACK:
			boost::shared_ptr<ack_header> a_header(
				dynamic_cast<boost::shared_ptr<ack_header>>(header)
			);
			// Jeżeli jest to pierwsza informacja zwrotna 
			// od serwera po UDP w nowym połączeniu
			if (win_ == -1) {
				// Ponów wysłanie client_id
				const std::string client_id_to_str =
					boost::lexical_cast<std::string>(client_id_);

				boost::shared_array<char> client_id_header(new char[
					6 /* Dł. tytułu komunikatu */ + 
					1 /* Odstęp */ +
					client_id_to_str.size() /* Dł. id klienta */ +
					1 /* Znak nowej linii */
				]);

				client_id_header = "CLIENT " + client_id_to_str + "\n";

				boost::asio::async_write(udp_socket_, client_id_header
						boost::bind(&client::handle_client_dgram, this,
						boost::asio::placeholders::error));
			} else {
				// Zaktualizuj tytuł ostatniego nagłówka
				last_header_title_ = ACK;
				// Wczytaj nie więcej niż _win bajtów danych z STDIN
				boost::asio::async_read(input_, 
					boost::asio::buffer(input_buffer_, win_),
					boost::bind(&client::handle_read_from_stdin, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			} 
	}
}

void client::handle_read_from_stdin(const boost::system::error_code& error,
	const size_t bytes_transferred)
{
	if (!error) {
		// Zuploaduj dane otrzymane z STDIN, ale zanim to zrobisz:
		//
		// -> zaktualizuj datagram jako ostatnio nadany:
				// Zapisz do ostatnio otrzymanego
		last_datagram_ = 
			"UPLOAD " + 
			boost::lexical_cast<std::string>(nr_expected_) +
			" " +
			boost::lexical_cast<std::string>(actual_dgram_nr_) +
			" " +
			boost::lexical_cast<std::string>(win_) +
			"\n" +
			std::string(input_buffer_, bytes_transferred);

		const int datagram_size = last_datagram_.size();
		// -> Stwórz shared_array'a, który będzie mergem nagłówka z danymi z STDIN:
		boost::shared_array<char> dgram_buffer(new char[datagram_size]);
		// -> Przepisz zawartość datagramu do powyższego bufora
		const std::string copy_of_last_datagram(last_datagram_);
		dgram_buffer = copy_of_last_datagram.c_str();
		// Zapisz do udp_socket'a
		boost::asio::async_write(udp_socket_,
			boost::asio::buffer(dgram_buffer, datagram_size, // TODO: Tutaj dodać nagłówek!!
			boost::bind(&client::handle_after_read_from_stdin, this,
				boost::asio::placeholders::error,
				bytes_transferred);
	} else {
		std::cerr << "Handle read from STDIN\n";
	}
}

void client::handle_after_read_from_stdin(const boost::system::error_code& error,
	const size_t bytes_transferred)
{
	if (!error) {
		// Czekaj na komunikat zwrotny od serwera
		// (ACK albo [nie daj Boże] na DATA bez uprzedniego ACK)
		boost::asio::async_read_until(udp_socket_, header_buffer_, '\n',
			boost::bind(&client::handle_read_datagram_header, this,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle upload datagram\n";
	}
}

void client::resend_last_datagram()
{
	const int datagram_size = last_datagram_.size();
	// -> Stwórz shared_array'a, który będzie mergem nagłówka z danymi z STDIN:
	boost::shared_array<char> dgram_buffer(new char[datagram_size]);
	// -> Przepisz zawartość datagramu do powyższego bufora:
	const std::string copy_of_last_datagram(last_datagram_);
	dgram_buffer = copy_of_last_datagram.c_str();
	// Wyślij ponownie:
	boost::asio::async_write(udp_socket_,
		boost::asio::buffer(dgram_buffer, datagram_size,
		boost::bind(&client::handle_after_read_from_stdin, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred);
}
