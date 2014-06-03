#include "client_parser.hh"

response client_parser::parse(std::uint16_t& port, std::string& server_name, 
		std::uint16_t& retransmit, const program_parametres& prms)
{
	namespace po = boost::program_options;
	try {
		po::options_description desc("Options");
		desc.add_options()
			(",p", po::value<std::uint16_t>(&port), "PORT")
			(",s", po::value<std::string>(&server_name)->required(), "SERVER_NAME")
			(",X", po::value<std::uint16_t>(&retransmit), "RETRANSMIT_LIMIT")
		;

		po::variables_map vm;

		try {
			po::store(po::parse_command_line(prms._argc, prms._argv, desc), vm);

			if (!vm.count("p"))
				port = PORT;

			if (!vm.count("X"))
				retransmit = RETRANSMIT_LIMIT;

			po::notify(vm);
		} catch (po::error& error) {
			std::cerr << "ERROR: " << error.what() << std::endl << std::endl;
			std::cerr << desc <<std::endl;
			return response::ERROR_IN_COMMAND_LINE;
		}
	} catch (std::exception& exception) {
		std::cerr << "Unhandled Exception reached the top of main: " 
			<< exception.what() << ", application will now exit" << std::endl;
		return response::ERROR_UNHANDLED_EXCEPTION;
	}

	return response::SUCCESS;
}
