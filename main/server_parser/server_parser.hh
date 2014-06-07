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
		size_t& port, 
		size_t& fifo_size,
		size_t& flw, 
		size_t& fhw, 
		size_t& buf_len,
		size_t& tx_interval, 
		const program_parametres& prms
	);
};
#endif
