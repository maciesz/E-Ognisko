#ifndef SERVER_PARSER_HH
#define SERVER_PARSER_HH

// Biblioteki standardowe
#include <cstdint>
#include <string>
#include <algorithm>
// Biblioteki boost'owe
#include <boost/program_options.hpp>
// Własne biblioteki/struktury
#include "../common/structures.hh"
#include "../common/global_variables.hh"
#include "../common/response.hh"

class server_parser
{
public:
	// Ostatecznie fhw należy po zwróceniu wyniku przez funkcję wyznaczyć jako:
	//
	// fhw = std::min(fhw, fifo_size);
	static response parse(
		std::uint16_t& port, 
		std::uint16_t& fifo_size,
		std::uint16_t& flw, 
		std::uint16_t& fhw, 
		std::uint16_t& buf_len,
		std::uint16_t& tx_interval, 
		const program_parametres& prms
	);
};
#endif
