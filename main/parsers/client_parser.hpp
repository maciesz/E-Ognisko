#ifndef CLIENT_PARSER_HPP
#define CLIENT_PARSER_HPP

// Biblioteki boost'owe
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
// Biblioteki standardowe
#include <string>
// Własne biblioteki/struktury
#include "../headers/structures.hpp"
#include "../headers/global_variables.hpp"
#include "../headers/response.hpp"

class client_parser
{
public:
	static response parse(boost::uint16_t& port, std::string& server_name, 
		boost::uint16_t& retransmit, const program_parametres& params);
};
#endif