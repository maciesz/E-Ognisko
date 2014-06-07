#include <boost/asio.hpp>
#include <boost/cstdint.hpp>

#include <iostream>
#include <algorithm>
#include <string>

#include "../client_parser/client_parser.hh"
#include "../common/structures.hh"
#include "../common/response.hh"
#include "../client/client.hh"

int main(int argc, char** argv)
{
	program_parametres ps(argc, argv);

	size_t port;
	std::string server_name;
	size_t retransmit;

	response res = client_parser::parse(
		port,
		server_name,
		retransmit,
		ps
	);

	if (res == response::SUCCESS) {
		/*std::cout << "Port: " << port << "\n";
		std::cout << "Jestem przed portem\n";
		std::cout << "Server name:" << server_name;// << "\n";
		//std::cout << "Retransmit limit: " << retransmit << "\n";*/

		try {
			
			std::string port_s = boost::lexical_cast<std::string>(port);
			boost::asio::io_service io_service;
			client cl(
				io_service,
				server_name,
				port_s,
				retransmit, 
				100,
				500,
				1000
			);

			io_service.run();
			std::cerr << "Skończyłem robotę i sobie wychodzę. Narazie\n";
		} catch (std::exception& ex) {
			std::cerr << "Exception: " << ex.what() << "\n";
		}
	}
	
	return 0;
}
