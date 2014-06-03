#include <iostream>
#include <cstdint>
#include <algorithm>

#include <boost/asio.hpp>

#include "parsers/server_parser.hh"
#include "common/structures.hh"
#include "common/response.hh"
#include "server/server.hh"

int main(int argc, char** argv)
{
	program_parametres ps(argc, argv);

	std::uint16_t port;
	std::uint16_t fifo_size;
	std::uint16_t fifo_low_watermark;
	std::uint16_t fifo_high_watermark;
	std::uint16_t buf_len;
	std::uint16_t tx_interval;

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
