#include "client.hpp"
 
 client::client(boost::asio::io_service& io_service, const std::string& port, 
 	const std::string& server_name, boost::uint16_t retransmit_limit) 
 : 
	tcp_socket_(io_service), udp_socket_(io_service),
	tcp_resolver_(io_service), udp_resolver_(io_service),
	input_(io_service, ::dup(STDIN_FILENO)), 
	output_(io_service, ::dup(STDOUT_FILENO)), 
	header_buffer_(CLIENT_BUFFER_LEN),
	port_(port), server_name_(server_name), retransmit_limit_(retransmit_limit),
	read_buf_(new char[CLIENT_BUFFER_LEN]), 
	write_buf_(new char[CLIENT_BUFFER_LEN]),
	raport_buf_(new char[CLIENT_BUFFER_LEN])
{
	// Znajdź endpoint do połączenia z serwerem po TCP
	boost::asio::ip::tcp::resolver::query tcp_query_(server_name, port);
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = 
		tcp_resolver_.resolve(tcp_query_);
	// Podejmij próbę połączenia z serwerem po TCP
	boost::asio::async_connect(tcp_socket_, endpoint_iterator,
		boost::bind(&client::handle_tcp_connect, this,
			boost::asio::placeholders::error);
}

void client::handle_tcp_connect(const boost::system::error_code& error)
{
	if (!error) {
		boost::asio::read_until(tcp_socket_, header_buffer_, '\n',
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
		std::istream is(&header_buffer_);
		std::string s_header;
		std::getline(is, s_header);
		try {
			client_header* header = 
				factory.match_header(headerline_parser::get_data(s_header));
			client_id_ = header->_client_id;
			delete header;
		} catch (invalid_header_exception& ex) {
			std::cerr << "Invalid header format: client id from server\n";
			//close();
			return;
		}
		boost::asio::async_read(tcp_socket_, 
			boost::asio::buffer(raport_buf_, CLIENT_BUFFER_LEN),
			boost::bind(&client::handle_udp_connect, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		create_udp_socket(s_header.size());
	} else {
		std::cerr << "Handle client id\n";
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
			boost::asio::placeholders::error, bytes_transferred);
}

void client::handle_udp_connect(const boost::system::error_code& error, 
	const int bytes_transferred)
{
	if (!error) {
		boost::asio::async_write(udp_socket_, 
			boost::asio::buffer(header_buffer_, bytes_transferred),
			boost::bind(&client::handle_client_dgram, this,
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Handle udp connect\n";
	}
}

void client::handle_client_dgram(const boost::system::error_code& error)
{
	if (!error) {
		boost::asio::async_read_until(udp_socket_, header_buffer_, '\n',
			boost::bind(&client::handle_read_datagram, this,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
	} else {
		std::cerr << "Handle client dgram\n";
	}
}

void client::handle_read_datagram(const boost::system::error_code& error, 
	const size_t bytes_transferred)
{
	if (!error) {
		// TODO:
		// Teraz to fabryka rozpoznaje jaka to wiadomość i na tej podstawie
		// działamy odpowiednio i przechodzimy do handlera wywołującego czytanie datagramu
	} else {
		std::cerr << "Handle read datagram\n";
	}
}

// TODO: cała reszta ;-)
