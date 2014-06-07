#include <iostream>
#include <cstdint>
#include <algorithm>

#include <boost/asio.hpp>

#include "../server_parser/server_parser.hh"
#include "../common/structures.hh"
#include "../common/response.hh"
#include "../server/server.hh"

int main(int argc, char** argv)
{
	//std::ios_base::sync_with_stdio(false);
	program_parametres ps(argc, argv);

	size_t port;
	size_t fifo_size;
	size_t fifo_low_watermark;
	size_t fifo_high_watermark;
	size_t buf_len;
	size_t tx_interval;

	response res = server_parser::parse(
		port,
		fifo_size,
		fifo_low_watermark,
		fifo_high_watermark,
		buf_len,
		tx_interval,
		ps
	);

	if (res == response::SUCCESS) {
		std::cout << "Port: " << port << "\n";
		std::cout << "Fifo size: " << fifo_size << "\n";
		std::cout << "Fifo low watermark: " << fifo_low_watermark << "\n";
		std::cout << "Fifo high watermark: " << fifo_high_watermark << "\n";
		std::cout << "Server buffer length: " << buf_len << "\n";
		std::cout << "TX_INTERVAL: " << tx_interval << "\n";

		std::string port_str = std::to_string(port);
		try {
			boost::asio::io_service io_service;
			server serv(
				port_str,
				fifo_size,
				fifo_low_watermark,
				fifo_high_watermark,
				buf_len,
				tx_interval
			);

	//		io_service.run();
			serv.run();
		} catch (std::exception& ex) {
			std::cerr << "Exception: " << ex.what() << "\n";
		}
	}

	return 0;
}
