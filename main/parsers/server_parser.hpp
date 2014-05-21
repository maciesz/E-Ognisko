#ifndef SERVER_PARSER_HPP
#define SERVER_PARSER_HPP

//// Biblioteki boost'owe
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
// Biblioteki standardowe
#include <string>
#include <algorithm>
// Własne biblioteki/struktury
#include "../headers/structures.hpp"
#include "../headers/global_variables.hpp"
#include "../headers/response.hpp"

class server_parser
{
public:
	// Ostatecznie fhw należy po zwróceniu wyniku przez funkcję wyznaczyć jako:
	//
	// fhw = std::min(fhw, fifo_size);
	static response parse(boost::uint16_t& port, boost::uint16_t& fifo_size,
		boost::uint16_t& flw, boost::uint16_t& fhw, boost::uint16_t& buf_len,
		boost::uint16_t& tx_interval, const program_parametres& prms);
};
#endif