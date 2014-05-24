#include <boost/cstdint.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <algorithm>
#include <string>

#include "parsers/client_parser.hpp"
#include "common/structures.hpp"
#include "common/response.hpp"
#include "client/client.hpp"

int main(int argc, char** argv)
{
	program_parametres ps(argc, argv);

	boost::uint16_t port;
	std::string server_name;
	boost::uint16_t retransmit;

	response res = client_parser::parse(
		port,
		server_name,
		retransmit,
		ps
	);

	if (res == response::SUCCESS) {
		std::cout << "Port: " << port << "\n";
		std::cout << "Server name: " << server_name << "\n";
		std::cout << "Retransmit limit: " << retransmit << "\n";

		try {

			boost::asio::io_service io_service;
			client cl(
				io_service, 
				boost::lexical_cast<std::string>(port), 
				server_name, 
				retransmit, 
				100
			);

			io_service.run();
		} catch (std::exception& ex) {
			std::cerr << "Exception: " << ex.what() << "\n";
		}
	}
	
	return 0;
}