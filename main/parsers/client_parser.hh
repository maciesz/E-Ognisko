#ifndef CLIENT_PARSER_HH
#define CLIENT_PARSER_HH

// Biblioteki standardowe
#include <cstdint>
#include <string>
// Biblioteki boost'owe
#include <boost/program_options.hpp>
// Własne biblioteki/struktury
#include "../common/structures.hh"
#include "../common/global_variables.hh"
#include "../common/response.hh"

class client_parser
{
public:
	static response parse(std::uint16_t& port, std::string& server_name, 
		std::uint16_t& retransmit, const program_parametres& params);
};
#endif
