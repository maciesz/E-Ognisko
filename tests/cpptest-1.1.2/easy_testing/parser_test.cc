#include <boost/cstdint.hpp>

#include <iostream>
#include <algorithm>
#include <string>

#include "../../../main/parsers/client_parser.hpp"
#include "../../../main/parsers/server_parser.hpp"
#include "../../../main/headers/structures.hpp"
#include "../../../main/headers/response.hpp"

int main(int argc, char** argv)
{
	program_parametres ps(argc, argv);

	/*
	Client parser test
	*/
	/*
	boost::uint16_t port;
	std::string server;
	boost::uint16_t retransmit;

	response res = client_parser::parse(port, server, retransmit, ps);
	
	std::cout << "Port: " << port << "\n";
	std::cout << "Server name: " + server + "\n";
	std::cout << "Retransmit limit: " << retransmit << "\n";
	

	*/
	/*
	Server parser test
	*/
	
	boost::uint16_t port;
	boost::uint16_t fifo_size;
	boost::uint16_t low_fifo_watermark;
	boost::uint16_t high_fifo_watermark;
	boost::uint16_t buffer_length;
	boost::uint16_t tx_interval;
	
	response res = server_parser::parse(
		port,
		fifo_size,
		low_fifo_watermark,
		high_fifo_watermark,
		buffer_length,
		tx_interval,
		ps
	);

	std::cout << "Port: " << port << "\n";
	std::cout << "Fifo size: " << fifo_size << "\n";
	std::cout << "Low fifo watermark: " << low_fifo_watermark << "\n";
	std::cout << "High fifo watermark: " << std::min(high_fifo_watermark, fifo_size) << "\n";
	std::cout << "Buffer length: " << buffer_length << "\n";
	std::cout << "Tx interval: " << tx_interval << "\n";

	
	return 0;
}
