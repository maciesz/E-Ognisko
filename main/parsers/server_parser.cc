#include "server_parser.hpp"

response server_parser::parse(boost::uint16_t& port, boost::uint16_t& fifo_size,
		boost::uint16_t& flw, boost::uint16_t& fhw, boost::uint16_t& buf_len,
		boost::uint16_t& tx_interval, const program_parametres& prms)
{
	namespace po = boost::program_options;
	try {
		po::options_description desc("Options");
		desc.add_options()
			(",p", po::value<boost::uint16_t>(&port), "PORT")
			(",F", po::value<boost::uint16_t>(&fifo_size), "FIFO_SIZE")
			(",L", po::value<boost::uint16_t>(&flw), "FIFO_LOW_WATERMARK")
			(",H", po::value<boost::uint16_t>(&fhw), "FIFO_HIGH_WATERMARK")
			(",X", po::value<boost::uint16_t>(&buf_len), "BUF_LEN")
			(",i", po::value<boost::uint16_t>(&tx_interval), "TX_INTERVAL")
		;

		po::variables_map vm;

		try {
			po::store(po::parse_command_line(prms._argc, prms._argv, desc), vm);
			
			if (!vm.count("p"))
				port = PORT;

			if (!vm.count("F"))
				fifo_size = FIFO_SIZE;

			if (!vm.count("L"))
				flw = FIFO_LOW_WATERMARK;

			if (!vm.count("H"))
				fhw = std::min(FIFO_SIZE, FIFO_HIGH_WATERMARK);

			if (!vm.count("X"))
				buf_len = BUF_LEN;

			if (!vm.count("i"))
				tx_interval = TX_INTERVAL;

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

	fhw = std::min(fhw, fifo_size);
	response::SUCCESS;
}